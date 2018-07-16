/* 
 * Copyright 2018 Pierre Ossman for Cendio AB
 *    
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <security/pam_appl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

// PAM service name
const char *SERVICE_NAME = "tigervnc";

// Main script PID
volatile static pid_t script = -1;

static void
sighandler(int sig)
{
    if (script > 0) {
        kill(script, SIGTERM);
    }
}

static void
setup_signals(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
}

static int
conv(int num_msg,
     const struct pam_message **msg,
     struct pam_response **resp, void *appdata_ptr)
{
    /* Opening a session should not require a conversation */
    return PAM_CONV_ERR;
}

static pam_handle_t *
run_pam(int *pamret, const char *username, const char *display)
{
    pam_handle_t *pamh;

    /* Say hello to PAM */
    struct pam_conv pconv;
    pconv.conv = conv;
    pconv.appdata_ptr = NULL;
    *pamret = pam_start(SERVICE_NAME, username, &pconv, &pamh);
    if (*pamret != PAM_SUCCESS) {
        /* pam_strerror requires a pamh argument, but if pam_start
           fails, pamh is invalid. In practice, at least the Linux
           implementation of pam_strerror does not use the pamh
           argument, but let's take care - avoid pam_strerror here. */
        fprintf(stderr, "pam_start failed: %d\n", *pamret);
        return NULL;
    }

    /* ConsoleKit and systemd (and possibly others) uses this to
       determine if the session is local or not. It needs to be set to
       something that can't be interpreted as localhost. We don't know
       what the client's address is though, and that might change on
       reconnects. We also don't want to set it to some text string as
       that results in a DNS lookup with e.g. libaudit. Let's use a
       fake IPv4 address from the documentation range. */
    /* FIXME: This might throw an error on a IPv6-only host */
    *pamret = pam_set_item(pamh, PAM_RHOST, "203.0.113.20");
    if (*pamret != PAM_SUCCESS) {
        fprintf(stderr, "pam_set_item(PAM_RHOST) failed: %d (%s)\n",
                *pamret, pam_strerror(pamh, *pamret));
        return pamh;
    }

#ifdef PAM_XDISPLAY
    /* Let PAM modules use this to tag the session as a graphical one */
    *pamret = pam_set_item(pamh, PAM_XDISPLAY, display);
    /* Note: PAM_XDISPLAY is only supported by modern versions of PAM */
    if (*pamret != PAM_BAD_ITEM && *pamret != PAM_SUCCESS) {
        fprintf(stderr, "pam_set_item(PAM_XDISPLAY) failed: %d (%s)\n",
                *pamret, pam_strerror(pamh, *pamret));
        return pamh;
    }
#endif

    /* Open session */
    *pamret = pam_open_session(pamh, PAM_SILENT);
    if (*pamret != PAM_SUCCESS) {
        fprintf(stderr, "pam_open_session failed: %d (%s)\n",
                *pamret, pam_strerror(pamh, *pamret));
        return pamh;
    }

    return pamh;
}

static int
stop_pam(pam_handle_t * pamh, int pamret)
{
    /* Close session */
    if (pamret == PAM_SUCCESS) {
        pamret = pam_close_session(pamh, PAM_SILENT);
        if (pamret != PAM_SUCCESS) {
            fprintf(stderr, "pam_close_session failed: %d (%s)\n",
                    pamret, pam_strerror(pamh, pamret));
        }
    }

    /* If PAM was OK and we are running on a SELinux system, new
       processes images will be executed in the root context. */

    /* Say goodbye */
    pamret = pam_end(pamh, pamret);
    if (pamret != PAM_SUCCESS) {
        /* avoid pam_strerror - we have no pamh. */
        fprintf(stderr, "pam_end failed: %d\n", pamret);
        return EX_OSERR;
    }
    return pamret;
}

static char **
prepare_environ(pam_handle_t * pamh)
{
    char **pam_env, **child_env, **entry;
    int orig_count, pam_count;

    /* This function merges the normal environment with PAM's changes */

    pam_env = pam_getenvlist(pamh);
    if (pam_env == NULL)
        return NULL;

    /*
     * Worst case scenario is that PAM only adds variables, so allocate
     * based on that assumption.
     */
    orig_count = 0;
    for (entry = environ; *entry != NULL; entry++)
        orig_count++;
    pam_count = 0;
    for (entry = pam_env; *entry != NULL; entry++)
        pam_count++;

    child_env = calloc(orig_count + pam_count + 1, sizeof(char *));
    if (child_env == NULL)
        return NULL;

    memcpy(child_env, environ, sizeof(char *) * orig_count);
    for (entry = child_env; *entry != NULL; entry++) {
        *entry = strdup(*entry);
        if (*entry == NULL)     // FIXME: cleanup
            return NULL;
    }

    for (entry = pam_env; *entry != NULL; entry++) {
        size_t varlen;
        char **orig_entry;

        varlen = strcspn(*entry, "=") + 1;

        /* Check for overwrite */
        for (orig_entry = child_env; *orig_entry != NULL; orig_entry++) {
            if (strncmp(*entry, *orig_entry, varlen) != 0)
                continue;

            free(*orig_entry);
            *orig_entry = *entry;
            break;
        }

        /* New variable? */
        if (*orig_entry == NULL) {
            /*
             * orig_entry will be pointing at the terminating entry,
             * so we can just tack it on here. The new NULL was already
             * prepared by calloc().
             */
            *orig_entry = *entry;
        }
    }

    return child_env;
}

static void
switch_user(const char *username, uid_t uid, gid_t gid)
{
    // We must change group stuff first, because only root can do that.
    if (setgid(gid) < 0) {
        perror(": setgid");
        _exit(EX_OSERR);
    }

    // Supplementary groups.
    if (initgroups(username, gid) < 0) {
        perror("initgroups");
        _exit(EX_OSERR);
    }

    // Set euid, ruid and suid
    if (setuid(uid) < 0) {
        perror("setuid");
        _exit(EX_OSERR);
    }
}

static void
redir_stdio(const char *homedir, const char *display)
{
    int fd;
    char hostname[HOST_NAME_MAX+1];
    char logfile[PATH_MAX];

    fd = open("/dev/null", O_RDONLY);
    if (fd == -1) {
        perror("open");
        _exit(EX_OSERR);
    }
    if (dup2(fd, 0) == -1) {
        perror("dup2");
        _exit(EX_OSERR);
    }
    close(fd);

    snprintf(logfile, sizeof(logfile), "%s/.vnc", homedir);
    if (mkdir(logfile, 0755) == -1) {
        if (errno != EEXIST) {
            perror("mkdir");
            _exit(EX_OSERR);
        }
    }

    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("gethostname");
        _exit(EX_OSERR);
    }

    snprintf(logfile, sizeof(logfile), "%s/.vnc/%s%s.log",
             homedir, hostname, display);
    fd = open(logfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        _exit(EX_OSERR);
    }
    if ((dup2(fd, 1) == -1) || (dup2(fd, 2) == -1)) {
        perror("dup2");
        _exit(EX_OSERR);
    }
    close(fd);
}

static void
close_fds(void)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/proc/self/fd");
    if (dir == NULL) {
        perror("opendir");
        _exit(EX_OSERR);
    }

    while ((entry = readdir(dir)) != NULL) {
        int fd;
        fd = atoi(entry->d_name);
        if (fd < 3)
            continue;
        close(fd);
    }

    closedir(dir);
}

static pid_t
run_script(const char *username, const char *display, char **envp)
{
    struct passwd *pwent;
    pid_t pid;
    const char *child_argv[3];

    pwent = getpwnam(username);
    if (pwent == NULL) {
        perror("getpwnam");
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        perror("subprocess: fork");
        return pid;
    }

    /* two processes now */

    if (pid > 0)
        return pid;

    /* child */

    switch_user(pwent->pw_name, pwent->pw_uid, pwent->pw_gid);

    close_fds();

    redir_stdio(pwent->pw_dir, display);

    // execvpe() is not POSIX and is missing from older glibc
    // First clear out everything
    while ((environ != NULL) && (*environ != NULL)) {
        char *var, *eq;
        var = strdup(*environ);
        eq = strchr(var, '=');
        if (eq)
            *eq = '\0';
        unsetenv(var);
        free(var);
    }

    // Then copy over the desired environment
    for (; *envp != NULL; envp++)
        putenv(*envp);

    // Set up some basic environment for the script
    setenv("HOME", pwent->pw_dir, 1);
    setenv("SHELL", pwent->pw_shell, 1);
    setenv("LOGNAME", pwent->pw_name, 1);
    setenv("USER", pwent->pw_name, 1);
    setenv("USERNAME", pwent->pw_name, 1);

    child_argv[0] = LIBEXEC_DIR "/vncserver";
    child_argv[1] = display;
    child_argv[2] = NULL;

    execvp(child_argv[0], (char*const*)child_argv);

    // execvp failed
    perror("execvp");

    _exit(EX_OSERR);
}

int
main(int argc, char **argv)
{
    const char *username, *display;

    if ((argc != 3) || (argv[2][0] != ':')) {
        fprintf(stderr, "Syntax:\n");
        fprintf(stderr, "    %s <username> <display>\n", argv[0]);
        return EX_USAGE;
    }

    username = argv[1];
    display = argv[2];

    /* Indicate that this is a graphical user session. We need to do
       this here before PAM as pam_systemd.so looks at these. */
    if ((putenv("XDG_SESSION_CLASS=user") < 0) ||
        (putenv("XDG_SESSION_TYPE=x11") < 0)) {
        perror("putenv");
        return EX_OSERR;
    }

    /* Init PAM */
    int pamret;
    pam_handle_t *pamh = run_pam(&pamret, username, display);
    if (!pamh) {
        return EX_OSERR;
    }
    if (pamret != PAM_SUCCESS) {
        stop_pam(pamh, pamret);
        return EX_OSERR;
    }

    char **child_env;
    child_env = prepare_environ(pamh);
    if (child_env == NULL) {
        fprintf(stderr, "Failure creating child process environment\n");
        return EX_OSERR;
    }

    setup_signals();

    script = run_script(username, display, child_env);
    if (script == -1) {
        fprintf(stderr, "Failure starting vncserver script\n");
        return EX_OSERR;
    }

    while (1) {
        int status;
        pid_t gotpid = waitpid(script, &status, 0);
        if (gotpid < 0) {
            if (errno != EINTR) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            continue;
        }
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                fprintf(stderr,
                        "vncsession: vncserver exited with status=%d\n",
                        WEXITSTATUS(status));
            }
            break;
        }
        else if (WIFSIGNALED(status)) {
            fprintf(stderr,
                    "vncsession: vncserver was terminated by signal %d\n",
                    WTERMSIG(status));
            break;
        }
    }

    stop_pam(pamh, pamret);

    return 0;
}
