/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 TigerVNC Team
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

#include <rdr/Exception.h>

#include <rfb/LogWriter.h>
#include <rfb/Security.h>

#include <rfb_win32/resource.h>
#include <rfb_win32/SecurityPage.h>

#include <list>

using namespace rdr;
using namespace rfb;
using namespace rfb::win32;
using namespace std;

static LogWriter vlog("AuthDialog");

/* XXX: This class contains bunch of similar code to unix/vncviewer/CConn.cxx */
SecurityPage::SecurityPage(Security *security_)
  : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SECURITY)),
    security(security_) {
}

void
SecurityPage::initDialog()
{
  list<U8> secTypes;
  list<U8>::iterator i;

  if (isItemChecked(IDC_ENC_X509))
    enableX509Dialogs();
  else
    disableX509Dialogs();

  secTypes = security->GetEnabledSecTypes();

  /* Process non-VeNCrypt sectypes */
  for (i = secTypes.begin(); i != secTypes.end(); i++) {
    switch (*i) {
    case secTypeNone:
      enableAuthMethod(IDC_ENC_NONE, IDC_AUTH_NONE);
      break;
    case secTypeVncAuth:
      enableAuthMethod(IDC_ENC_NONE, IDC_AUTH_VNC);
      break;
    }
  }

  list<U32> secTypesExt;
  list<U32>::iterator iext;

  secTypesExt = security->GetEnabledExtSecTypes();

  /* Process VeNCrypt subtypes */
  for (iext = secTypesExt.begin(); iext != secTypesExt.end(); iext++) {
    switch (*iext) {
    case secTypePlain:
      enableAuthMethod(IDC_ENC_NONE, IDC_AUTH_PLAIN);
      break;
    case secTypeTLSNone:
      enableAuthMethod(IDC_ENC_TLS, IDC_AUTH_NONE);
      break;
    case secTypeTLSVnc:
      enableAuthMethod(IDC_ENC_TLS, IDC_AUTH_VNC);
      break;
    case secTypeTLSPlain:
      enableAuthMethod(IDC_ENC_TLS, IDC_AUTH_PLAIN);
      break;
    case secTypeX509None:
      enableAuthMethod(IDC_ENC_X509, IDC_AUTH_NONE);
      enableX509Dialogs();
      break;
    case secTypeX509Vnc:
      enableAuthMethod(IDC_ENC_X509, IDC_AUTH_VNC);
      enableX509Dialogs();
      break;
    case secTypeX509Plain:
      enableAuthMethod(IDC_ENC_X509, IDC_AUTH_PLAIN);
      enableX509Dialogs();
      break;
    }
  }
}

bool
SecurityPage::onCommand(int id, int cmd)
{
  if (id == IDC_ENC_X509) {
    if (isItemChecked(IDC_ENC_X509))
      enableX509Dialogs();
    else
      disableX509Dialogs();
  }

  return true;
}

bool
SecurityPage::onOk() {
#ifdef HAVE_GNUTLS
  bool x509_loaded = false;
#endif
  bool vnc_loaded = false;
  list<U32> secTypes;

  /* Keep same priorities as in common/rfb/SecurityClient::secTypes */
  secTypes.push_back(secTypeVeNCrypt);

#ifdef HAVE_GNUTLS
  /* X509Plain */
  if (authMethodEnabled(IDC_ENC_X509, IDC_AUTH_PLAIN)) {
    loadX509Certs(x509_loaded);
    secTypes.push_back(secTypeX509Plain);
  }

  /* TLSPlain */
  if (authMethodEnabled(IDC_ENC_TLS, IDC_AUTH_PLAIN))
    secTypes.push_back(secTypeTLSPlain);

  /* X509Vnc */
  if (authMethodEnabled(IDC_ENC_X509, IDC_AUTH_VNC)) {
    loadX509Certs(x509_loaded);
    loadVncPasswd(vnc_loaded);
    secTypes.push_back(secTypeX509Vnc);
  }

  /* TLSVnc */
  if (authMethodEnabled(IDC_ENC_TLS, IDC_AUTH_VNC)) {
    loadVncPasswd(vnc_loaded);
    secTypes.push_back(secTypeTLSVnc);
  }

  /* X509None */
  if (authMethodEnabled(IDC_ENC_X509, IDC_AUTH_NONE)) {
    loadX509Certs(x509_loaded);
    secTypes.push_back(secTypeX509None);
  }

  /* TLSNone */
  if (authMethodEnabled(IDC_ENC_TLS, IDC_AUTH_NONE))
    secTypes.push_back(secTypeTLSNone);
#endif

  /* VncAuth */
  if (authMethodEnabled(IDC_ENC_NONE, IDC_AUTH_VNC)) {
    loadVncPasswd(vnc_loaded);
    secTypes.push_back(secTypeVncAuth);
  }

  /* None */
  if (authMethodEnabled(IDC_ENC_NONE, IDC_AUTH_NONE))
    secTypes.push_back(secTypeNone);

  security->SetSecTypes(secTypes);

  return true;
}

inline void
SecurityPage::disableFeature(int id)
{
  enableItem(id, false);
  setItemChecked(id, false);
}

inline void
SecurityPage::enableAuthMethod(int encid, int authid)
{
  setItemChecked(encid, true);
  setItemChecked(authid, true);
}

inline bool
SecurityPage::authMethodEnabled(int encid, int authid)
{
  return isItemChecked(encid) && isItemChecked(authid);
}

inline void
SecurityPage::loadX509Certs(bool &loaded)
{
  if (!loaded)
    loadX509Certs();
  loaded = true;
}

inline void
SecurityPage::loadVncPasswd(bool &loaded)
{
  if (!loaded)
    loadVncPasswd();
  loaded = true;
}

