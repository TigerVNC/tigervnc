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

#include <config.h>

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
#include <syslog.h>
#include <security/pam_appl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef HAVE_SELINUX
#include <selinux/selinux.h>
#include <selinux/restorecon.h>
#endif

extern char **environ;

// PAM service name
const char *SERVICE_NAME = "tigervnc";

// Main script PID
volatile static pid_t script = -1;

// Daemon completion pipe
int daemon_pipe_fd = -1;

static int
begin_daemon(void)
{
    int devnull, fds[2];
    pid_t pid;

    /* Pipe to report startup success */
    if (pipe(fds) < 0) {
        perror("pipe");
        return -1;
    }

    /* First fork */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid != 0) {
        ssize_t len;
        char buf[1];

        close(fds[1]);

        /* Wait for child to finish startup */
        len = read(fds[0], buf, 1);
        if (len != 1) {
            fprintf(stderr, "Failed to start session\n");
            _exit(EX_OSERR);
        }

        _exit(0);
    }

    close(fds[0]);
    daemon_pipe_fd = fds[1];

    /* Detach from terminal */
    if (setsid() < 0) {
        perror("setsid");
        return -1;
    }

    /* Another fork required to fully detach */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid != 0)
        _exit(0);

    /* A safe working directory */
    if (chdir("/") < 0) {
        perror("chdir");
        return -1;
    }

    /* Send all stdio to /dev/null */
    devnull = open("/dev/null", O_RDWR);
    if (devnull < 0) {
        fprintf(stderr, "Failed to open /dev/null: %s\n", strerror(errno));
        return -1;
    }
    if ((dup2(devnull, 0) < 0) ||
        (dup2(devnull, 1) < 0) ||
        (dup2(devnull, 2) < 0)) {
        perror("dup2");
        return -1;
    }
    if (devnull > 2)
        close(devnull);

    return 0;
}

static void
finish_daemon(void)
{
    write(daemon_pipe_fd, "+", 1);
    close(daemon_pipe_fd);
    daemon_pipe_fd = -1;
}

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
        syslog(LOG_CRIT, "pam_start failed: %d", *pamret);
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
        syslog(LOG_CRIT, "pam_set_item(PAM_RHOST) failed: %d (%s)",
               *pamret, pam_strerror(pamh, *pamret));
        return pamh;
    }

#ifdef PAM_XDISPLAY
    /* Let PAM modules use this to tag the session as a graphical one */
    *pamret = pam_set_item(pamh, PAM_XDISPLAY, display);
    /* Note: PAM_XDISPLAY is only supported by modern versions of PAM */
    if (*pamret != PAM_BAD_ITEM && *pamret != PAM_SUCCESS) {
        syslog(LOG_CRIT, "pam_set_item(PAM_XDISPLAY) failed: %d (%s)",
               *pamret, pam_strerror(pamh, *pamret));
        return pamh;
    }
#endif

    /* Open session */
    *pamret = pam_open_session(pamh, PAM_SILENT);
    if (*pamret != PAM_SUCCESS) {
        syslog(LOG_CRIT, "pam_open_session failed: %d (%s)",
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
            syslog(LOG_ERR, "pam_close_session failed: %d (%s)",
                   pamret, pam_strerror(pamh, pamret));
        }
    }

    /* If PAM was OK and we are running on a SELinux system, new
       processes images will be executed in the root context. */

    /* Say goodbye */
    pamret = pam_end(pamh, pamret);
    if (pamret != PAM_SUCCESS) {
        /* avoid pam_strerror - we have no pamh. */
        syslog(LOG_ERR, "pam_end failed: %d", pamret);
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
        syslog(LOG_CRIT, "setgid: %s", strerror(errno));
        _exit(EX_OSERR);
    }

    // Supplementary groups.
    if (initgroups(username, gid) < 0) {
        syslog(LOG_CRIT, "initgroups: %s", strerror(errno));
        _exit(EX_OSERR);
    }

    // Set euid, ruid and suid
    if (setuid(uid) < 0) {
        syslog(LOG_CRIT, "setuid: %s", strerror(errno));
        _exit(EX_OSERR);
    }
}

static void
redir_stdio(const char *homedir, const char *display)
{
    int fd;
    size_t hostlen;
    char* hostname = NULL;
    char logfile[PATH_MAX];

    fd = open("/dev/null", O_RDONLY);
    if (fd == -1) {
        syslog(LOG_CRIT, "Failure redirecting stdin: open: %s", strerror(errno));
        _exit(EX_OSERR);
    }
    if (dup2(fd, 0) == -1) {
        syslog(LOG_CRIT, "Failure redirecting stdin: dup2: %s", strerror(errno));
        _exit(EX_OSERR);
    }
    close(fd);

    snprintf(logfile, sizeof(logfile), "%s/.vnc", homedir);
    if (mkdir(logfile, 0755) == -1) {
        if (errno != EEXIST) {
            syslog(LOG_CRIT, "Failure creating \"%s\": %s", logfile, strerror(errno));
            _exit(EX_OSERR);
        }

#ifdef HAVE_SELINUX
        int result;
        if (selinux_file_context_verify(logfile, 0) == 0) {
            result = selinux_restorecon(logfile, SELINUX_RESTORECON_RECURSE);

            if (result < 0) {
                syslog(LOG_WARNING, "Failure restoring SELinux context for \"%s\": %s", logfile, strerror(errno));
            }
        }
#endif
    }

    hostlen = sysconf(_SC_HOST_NAME_MAX);
    if (hostlen < 0) {
      syslog(LOG_CRIT, "sysconf(_SC_HOST_NAME_MAX): %s", strerror(errno));
      _exit(EX_OSERR);
    }
    hostname = malloc(hostlen + 1);
    if (gethostname(hostname, hostlen + 1) == -1) {
        syslog(LOG_CRIT, "gethostname: %s", strerror(errno));
        free(hostname);
        _exit(EX_OSERR);
    }

    snprintf(logfile, sizeof(logfile), "%s/.vnc/%s%s.log",
             homedir, hostname, display);
    free(hostname);
    fd = open(logfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        syslog(LOG_CRIT, "Failure creating log file \"%s\": %s", logfile, strerror(errno));
        _exit(EX_OSERR);
    }
    if ((dup2(fd, 1) == -1) || (dup2(fd, 2) == -1)) {
        syslog(LOG_CRIT, "Failure redirecting stdout or stderr: %s", strerror(errno));
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
        syslog(LOG_CRIT, "opendir: %s", strerror(errno));
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
        syslog(LOG_CRIT, "getpwnam: %s", strerror(errno));
        return -1;
    }

    pid = fork();
    if (pid < 0) {
        syslog(LOG_CRIT, "fork: %s", strerror(errno));
        return pid;
    }

    /* two processes now */

    if (pid > 0)
        return pid;

    /* child */

    switch_user(pwent->pw_name, pwent->pw_uid, pwent->pw_gid);

    if (chdir(pwent->pw_dir) == -1)
        chdir("/");

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

    child_argv[0] = CMAKE_INSTALL_FULL_LIBEXECDIR "/vncserver";
    child_argv[1] = display;
    child_argv[2] = NULL;

    execvp(child_argv[0], (char*const*)child_argv);

    // execvp failed
    syslog(LOG_CRIT, "execvp: %s", strerror(errno));

    _exit(EX_OSERR);
}

int
main(int argc, char **argv)
{
    char pid_file[PATH_MAX];
    FILE *f;

    const char *username, *display;

    if ((argc != 3) || (argv[2][0] != ':')) {
        fprintf(stderr, "Syntax:\n");
        fprintf(stderr, "    %s <username> <display>\n", argv[0]);
        return EX_USAGE;
    }

    username = argv[1];
    display = argv[2];

    if (geteuid() != 0) {
        fprintf(stderr, "This program needs to be run as root!\n");
        return EX_USAGE;
    }

    if (getpwnam(username) == NULL) {
        if (errno == 0)
          fprintf(stderr, "User \"%s\" does not exist\n", username);
        else
          fprintf(stderr, "Cannot look up user \"%s\": %s\n",
                  username, strerror(errno));
        return EX_OSERR;
    }

    if (begin_daemon() == -1)
        return EX_OSERR;

    openlog("vncsession", LOG_PID, LOG_AUTH);

    /* Indicate that this is a graphical user session. We need to do
       this here before PAM as pam_systemd.so looks at these. */
    if ((putenv("XDG_SESSION_CLASS=user") < 0) ||
        (putenv("XDG_SESSION_TYPE=x11") < 0)) {
        syslog(LOG_CRIT, "putenv: %s", strerror(errno));
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
        syslog(LOG_CRIT, "Failure creating child process environment");
        stop_pam(pamh, pamret);
        return EX_OSERR;
    }

    setup_signals();

    script = run_script(username, display, child_env);
    if (script == -1) {
        syslog(LOG_CRIT, "Failure starting vncserver script");
        stop_pam(pamh, pamret);
        return EX_OSERR;
    }

    snprintf(pid_file, sizeof(pid_file),
             "/run/vncsession-%s.pid", display);
    f = fopen(pid_file, "w");
    if (f == NULL) {
        syslog(LOG_ERR, "Failure creating pid file \"%s\": %s",
               pid_file, strerror(errno));
    } else {
        fprintf(f, "%ld\n", (long)getpid());
        fclose(f);
    }

    finish_daemon();

    while (1) {
        int status;
        pid_t gotpid = waitpid(script, &status, 0);
        if (gotpid < 0) {
            if (errno != EINTR) {
                syslog(LOG_CRIT, "waitpid: %s", strerror(errno));
                exit(EXIT_FAILURE);
            }
            continue;
        }
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) != 0) {
                syslog(LOG_WARNING,
                        "vncsession: vncserver exited with status=%d",
                        WEXITSTATUS(status));
            }
            break;
        }
        else if (WIFSIGNALED(status)) {
            syslog(LOG_WARNING,
                    "vncsession: vncserver was terminated by signal %d",
                    WTERMSIG(status));
            break;
        }
    }

    unlink(pid_file);

    stop_pam(pamh, pamret);

    return 0;
}
