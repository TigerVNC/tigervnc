/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

// -=- CleanDesktop.cxx

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>
#include <shlobj.h>

#include <rfb_win32/CleanDesktop.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Registry.h>
#include <rfb/LogWriter.h>
#include <rdr/Exception.h>

using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("CleanDesktop");


struct ActiveDesktop {
  ActiveDesktop() : handle(0) {
    // - Contact Active Desktop
    HRESULT result = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
                                      IID_IActiveDesktop, (PVOID*)&handle);
    if (result != S_OK)
      throw rdr::SystemException("failed to contact Active Desktop", result);
  }
  ~ActiveDesktop() {
    if (handle)
      handle->Release();
  }
  bool enable(bool enable_) {
    // - Get the current Active Desktop options
    COMPONENTSOPT adOptions;
    memset(&adOptions, 0, sizeof(adOptions));
    adOptions.dwSize = sizeof(adOptions);

    HRESULT result = handle->GetDesktopItemOptions(&adOptions, 0);
    if (result != S_OK) {
      vlog.error("failed to get Active Desktop options: %d", result);
      return false;
    }

    // - If Active Desktop is active, disable it
    if ((adOptions.fActiveDesktop==0) != (enable_==0)) {
      if (enable_)
        vlog.debug("enabling Active Desktop");
      else
        vlog.debug("disabling Active Desktop");

      adOptions.fActiveDesktop = enable_;
      result = handle->SetDesktopItemOptions(&adOptions, 0);
      if (result != S_OK) {
        vlog.error("failed to disable ActiveDesktop: %d", result);
        return false;
      }
      handle->ApplyChanges(AD_APPLY_REFRESH);
      return true;
    }
    return false;
  }
  IActiveDesktop* handle;
};


DWORD SysParamsInfo(UINT action, UINT param, PVOID ptr, UINT ini) {
  DWORD r = ERROR_SUCCESS;
  if (!SystemParametersInfo(action, param, ptr, ini)) {
    r = GetLastError();
    vlog.info("SPI error: %d", r);
  }
  return r;
}


CleanDesktop::CleanDesktop() : restoreActiveDesktop(false), restoreWallpaper(false), restoreEffects(false) {
  CoInitialize(0);
}

CleanDesktop::~CleanDesktop() {
  enableEffects();
  enablePattern();
  enableWallpaper();
  CoUninitialize();
}

void CleanDesktop::disableWallpaper() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("disable desktop wallpaper/Active Desktop");

    // -=- First attempt to remove the wallpaper using Active Desktop
    try {
      ActiveDesktop ad;
      if (ad.enable(false))
        restoreActiveDesktop = true;
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
    }

    // -=- Switch of normal wallpaper and notify apps
    SysParamsInfo(SPI_SETDESKWALLPAPER, 0, "", SPIF_SENDCHANGE);
    restoreWallpaper = true;

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}

void CleanDesktop::enableWallpaper() {
  try {
    ImpersonateCurrentUser icu;

    if (restoreActiveDesktop) {
      vlog.debug("restore Active Desktop");

      // -=- First attempt to re-enable Active Desktop
      try {
        ActiveDesktop ad;
        ad.enable(true);
        restoreActiveDesktop = false;
      } catch (rdr::Exception& e) {
        vlog.error(e.str());
      }
    }

    if (restoreWallpaper) {
      vlog.debug("restore desktop wallpaper");

      // -=- Then restore the standard wallpaper if required
	    SysParamsInfo(SPI_SETDESKWALLPAPER, 0, NULL, SPIF_SENDCHANGE);
      restoreWallpaper = false;
    }

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}


void CleanDesktop::disablePattern() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("disable desktop pattern");
    SysParamsInfo(SPI_SETDESKPATTERN, 0, "", SPIF_SENDCHANGE);
    restorePattern = true;

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}

void CleanDesktop::enablePattern() {
  try {
    if (restorePattern) {
      ImpersonateCurrentUser icu;

      vlog.debug("restoring pattern...");

      TCharArray pattern;
      if (osVersion.isPlatformWindows) {
        RegKey cfgKey;
        cfgKey.openKey(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"));
        pattern.buf = cfgKey.getString(_T("Pattern"));
      }
      SysParamsInfo(SPI_SETDESKPATTERN, 0, pattern.buf, SPIF_SENDCHANGE);
      restorePattern = false;
    }

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}


void CleanDesktop::disableEffects() {
#if (WINVER >= 0x500)
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("disable desktop effects");

    SysParamsInfo(SPI_SETFONTSMOOTHING, FALSE, 0, SPIF_SENDCHANGE);
    if (SysParamsInfo(SPI_GETUIEFFECTS, 0, &uiEffects, 0) == ERROR_CALL_NOT_IMPLEMENTED) {
      SysParamsInfo(SPI_GETCOMBOBOXANIMATION, 0, &comboBoxAnim, 0);
      SysParamsInfo(SPI_GETGRADIENTCAPTIONS, 0, &gradientCaptions, 0);
      SysParamsInfo(SPI_GETHOTTRACKING, 0, &hotTracking, 0);
      SysParamsInfo(SPI_GETLISTBOXSMOOTHSCROLLING, 0, &listBoxSmoothScroll, 0);
      SysParamsInfo(SPI_GETMENUANIMATION, 0, &menuAnim, 0);
      SysParamsInfo(SPI_SETCOMBOBOXANIMATION, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETGRADIENTCAPTIONS, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETHOTTRACKING, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETMENUANIMATION, 0, FALSE, SPIF_SENDCHANGE);
    } else {
      SysParamsInfo(SPI_SETUIEFFECTS, 0, FALSE, SPIF_SENDCHANGE);
    }
    restoreEffects = true;

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
#else
      vlog.info("disableffects not implemented");
#endif
}

void CleanDesktop::enableEffects() {
  try {
    if (restoreEffects) {
#if (WINVER >= 0x500)
      ImpersonateCurrentUser icu;

      vlog.debug("restore desktop effects");

      RegKey desktopCfg;
      desktopCfg.openKey(HKEY_CURRENT_USER, _T("Control Panel\\Desktop"));
      SysParamsInfo(SPI_SETFONTSMOOTHING, desktopCfg.getInt(_T("FontSmoothing"), 0) != 0, 0, SPIF_SENDCHANGE);
      if (SysParamsInfo(SPI_SETUIEFFECTS, 0, (void*)uiEffects, SPIF_SENDCHANGE) == ERROR_CALL_NOT_IMPLEMENTED) {
        SysParamsInfo(SPI_SETCOMBOBOXANIMATION, 0, (void*)comboBoxAnim, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETGRADIENTCAPTIONS, 0, (void*)gradientCaptions, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETHOTTRACKING, 0, (void*)hotTracking, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, (void*)listBoxSmoothScroll, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETMENUANIMATION, 0, (void*)menuAnim, SPIF_SENDCHANGE);
      }
      restoreEffects = false;
#else
      vlog.info("enableEffects not implemented");
#endif
    }

  } catch (rdr::Exception& e) {
    vlog.info(e.str());
  }
}
