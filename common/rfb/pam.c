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

#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>

#include <rfb/pam.h>

typedef struct
{
  const char *username;
  const char *password;
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
    case PAM_ERROR_MSG:
      resp[i].resp = 0;
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


int do_pam_auth(const char *service, const char *username, const char *password)
{
  int ret;
  AuthData auth = { username, password };
  struct pam_conv conv = {
    pam_callback,
    &auth
  };
  pam_handle_t *h = 0;
  ret = pam_start(service, username, &conv, &h);
  if (ret == PAM_SUCCESS)
    ret = pam_authenticate(h, 0);
  if (ret == PAM_SUCCESS)
    ret = pam_acct_mgmt(h, 0);
  pam_end(h, ret);

  return ret == PAM_SUCCESS ? 1 : 0;
}

