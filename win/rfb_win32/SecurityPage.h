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

#ifndef __RFB_WIN32_SECURITYPAGE_H__
#define __RFB_WIN32_SECURITYPAGE_H__

#include <rdr/types.h>

#include <rfb/Security.h>
#include <rfb_win32/Dialog.h>

#include <list>

namespace rfb {
namespace win32 {

class SecurityPage: public PropSheetPage
{
public:
  SecurityPage(Security *security_);

  virtual void loadX509Certs(void) = 0;
  virtual void enableX509Dialogs(void) = 0;
  virtual void disableX509Dialogs(void) = 0;
  virtual void loadVncPasswd(void) = 0;

  virtual void initDialog();
  virtual bool onCommand(int id, int cmd);
  virtual bool onOk();

protected:
  Security *security;

private:
  inline void enableVeNCryptFeatures(bool enable);
  inline void disableFeature(int id);
  inline void enableAuthMethod(int encid, int authid);
  inline bool authMethodEnabled(int encid, int authid);
  inline void loadX509Certs(bool &loaded);
  inline void loadVncPasswd(bool &loaded);
};
    
};
};

#endif
