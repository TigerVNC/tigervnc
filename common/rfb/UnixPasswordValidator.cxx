/* 
 * Copyright (C) 2006 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <security/pam_appl.h>

#include <core/Configuration.h>
#include <core/LogWriter.h>

#include <rfb/UnixPasswordValidator.h>

using namespace rfb;

static core::LogWriter vlog("UnixPasswordValidator");

static core::StringParameter pamService
  ("PAMService", "Service name for PAM password validation", "vnc");
core::AliasParameter pam_service("pam_service", "Alias for PAMService",
                                 &pamService);

std::string UnixPasswordValidator::displayName;

typedef struct
{
  const char *username;
  const char *password;
  std::string &msg;
} AuthData;

#if defined(__sun)
static int pam_callback(int count, struct pam_message **in,
                        struct pam_response **out, void *ptr)
#else
static int pam_callback(int count, const struct pam_message **in,
                        struct pam_response **out, void *ptr)
#endif
{
  int i;
  AuthData *auth = (AuthData *) ptr;
  struct pam_response *resp =
    (struct pam_response *) malloc (sizeof (struct pam_response) * count);

  if (!resp && count)
    return PAM_CONV_ERR;

  for (i = 0; i < count; i++) {
    resp[i].resp_retcode = PAM_SUCCESS;
    switch (in[i]->msg_style) {
    case PAM_TEXT_INFO:
      vlog.info("%s info: %s", (const char *) pamService, in[i]->msg);
      auth->msg = in[i]->msg;
      resp[i].resp = nullptr;
      break;
    case PAM_ERROR_MSG:
      vlog.error("%s error: %s", (const char *) pamService, in[i]->msg);
      auth->msg = in[i]->msg;
      resp[i].resp = nullptr;
      break;
    case PAM_PROMPT_ECHO_ON:	/* Send Username */
      resp[i].resp = strdup(auth->username);
      break;
    case PAM_PROMPT_ECHO_OFF:	/* Send Password */
      resp[i].resp = strdup(auth->password);
      break;
    default:
      free(resp);
      return PAM_CONV_ERR;
    }
  }

  *out = resp;
  return PAM_SUCCESS;
}

bool UnixPasswordValidator::validateInternal(SConnection * /* sc */,
					     const char *username,
					     const char *password,
					     std::string &msg)
{
  int ret;
  AuthData auth = { username, password, msg };
  struct pam_conv conv = {
    pam_callback,
    &auth
  };
  pam_handle_t *pamh = nullptr;
  ret = pam_start(pamService, username, &conv, &pamh);
  if (ret != PAM_SUCCESS) {
    /* Can't call pam_strerror() here because the content of pamh undefined */
    vlog.error("pam_start(%s) failed: %d", (const char *) pamService, ret);
    return false;
  }
#ifdef PAM_XDISPLAY
  /* At this point, displayName should never be empty */
  assert(displayName.length() > 0);
  /* Pass the display name to PAM modules but PAM_XDISPLAY may not be
   * recognized by modules built with old versions of PAM */
  ret = pam_set_item(pamh, PAM_XDISPLAY, displayName.c_str());
  if (ret != PAM_SUCCESS && ret != PAM_BAD_ITEM) {
    vlog.error("pam_set_item(PAM_XDISPLAY) failed: %d (%s)", ret, pam_strerror(pamh, ret));
    goto error;
  }
#endif
  ret = pam_authenticate(pamh, 0);
  if (ret != PAM_SUCCESS) {
    vlog.error("pam_authenticate() failed: %d (%s)", ret, pam_strerror(pamh, ret));
    goto error;
  }
  ret = pam_acct_mgmt(pamh, 0);
  if (ret != PAM_SUCCESS) {
    vlog.error("pam_acct_mgmt() failed: %d (%s)", ret, pam_strerror(pamh, ret));
    goto error;
  }
  return true;
error:
  pam_end(pamh, ret);
  return false;
}
