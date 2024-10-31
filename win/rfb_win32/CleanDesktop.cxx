/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <wininet.h>
#include <shlobj.h>

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rfb_win32/CleanDesktop.h>
#include <rfb_win32/CurrentUser.h>
#include <rfb_win32/Registry.h>

#include <set>

using namespace rfb;
using namespace rfb::win32;

static core::LogWriter vlog("CleanDesktop");


struct ActiveDesktop {
  ActiveDesktop() : handle(nullptr) {
    // - Contact Active Desktop
    HRESULT result = CoCreateInstance(CLSID_ActiveDesktop, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IActiveDesktop, (PVOID*)&handle);
    if (result != S_OK)
      throw core::win32_error("Failed to contact Active Desktop", HRESULT_CODE(result));
  }
  ~ActiveDesktop() {
    if (handle)
      handle->Release();
  }

  // enableItem
  //   enables or disables the Nth Active Desktop item
  bool enableItem(int i, bool enable_) {
    COMPONENT item;
    memset(&item, 0, sizeof(item));
    item.dwSize = sizeof(item);

    HRESULT hr = handle->GetDesktopItem(i, &item, 0);
    if (hr != S_OK) {
      vlog.error("Unable to GetDesktopItem %d: %ld", i, hr);
      return false;
    }
    item.fChecked = enable_;

    hr = handle->ModifyDesktopItem(&item, COMP_ELEM_CHECKED);
    return hr == S_OK;
  }
  
  // enable
  //   Attempts to enable/disable Active Desktop, returns true if the setting changed,
  //   false otherwise.
  //   If Active Desktop *can* be enabled/disabled then that is done.
  //   If Active Desktop is always on (XP/2K3) then instead the individual items are
  //   disabled, and true is returned to indicate that they need to be restored later.
  bool enable(bool enable_) {
    bool modifyComponents = false;

    vlog.debug("ActiveDesktop::enable");

    // - Firstly, try to disable Active Desktop entirely
    HRESULT hr;
    COMPONENTSOPT adOptions;
    memset(&adOptions, 0, sizeof(adOptions));
    adOptions.dwSize = sizeof(adOptions);

    // Attempt to actually disable/enable AD
    hr = handle->GetDesktopItemOptions(&adOptions, 0);
    if (hr == S_OK) {
      // If Active Desktop is already in the desired state then return false (no change)
      // NB: If AD is enabled AND restoreItems is set then we regard it as disabled...
      if (((adOptions.fActiveDesktop==0) && restoreItems.empty()) == (enable_==false))
        return false;
      adOptions.fActiveDesktop = enable_;
      hr = handle->SetDesktopItemOptions(&adOptions, 0);
    }
    // Apply the change, then test whether it actually took effect
    if (hr == S_OK)
      hr = handle->ApplyChanges(AD_APPLY_REFRESH);
    if (hr == S_OK)
      hr = handle->GetDesktopItemOptions(&adOptions, 0);
    if (hr == S_OK)
      modifyComponents = (adOptions.fActiveDesktop==0) != (enable_==false);
    if (hr != S_OK) {
      vlog.error("Failed to get/set Active Desktop options: %ld", hr);
      return false;
    }

    if (enable_) {
      // - We are re-enabling Active Desktop.  If there are components in restoreItems
      //   then restore them!
      std::set<int>::const_iterator i;
      for (i=restoreItems.begin(); i!=restoreItems.end(); i++) {
        enableItem(*i, true);
      }
      restoreItems.clear();
    } else if (modifyComponents) {
      // - Disable all currently enabled items, and add the disabled ones to restoreItems
      int itemCount = 0;
      hr = handle->GetDesktopItemCount(&itemCount, 0);
      if (hr != S_OK) {
        vlog.error("Failed to get desktop item count: %ld", hr);
        return false;
      }
      for (int i=0; i<itemCount; i++) {
        if (enableItem(i, false))
          restoreItems.insert(i);
      }
    }

    // - Apply whatever changes we have made, but DON'T save them!
    hr = handle->ApplyChanges(AD_APPLY_REFRESH);
    return hr == S_OK;
  }
  IActiveDesktop* handle;
  std::set<int> restoreItems;
};


DWORD SysParamsInfo(UINT action, UINT param, PVOID ptr, UINT ini) {
  DWORD r = ERROR_SUCCESS;
  if (!SystemParametersInfo(action, param, ptr, ini)) {
    r = GetLastError();
    vlog.info("SPI error: %lu", r);
  }
  return r;
}


CleanDesktop::CleanDesktop() : restoreActiveDesktop(false),
                               restoreWallpaper(false),
                               restoreEffects(false) {
  CoInitialize(nullptr);
}

CleanDesktop::~CleanDesktop() {
  enableEffects();
  enableWallpaper();
  CoUninitialize();
}

void CleanDesktop::disableWallpaper() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("Disable desktop wallpaper/Active Desktop");

    // -=- First attempt to remove the wallpaper using Active Desktop
    try {
      ActiveDesktop ad;
      if (ad.enable(false))
        restoreActiveDesktop = true;
    } catch (std::exception& e) {
      vlog.error("%s", e.what());
    }

    // -=- Switch of normal wallpaper and notify apps
    SysParamsInfo(SPI_SETDESKWALLPAPER, 0, (PVOID) "", SPIF_SENDCHANGE);
    restoreWallpaper = true;

  } catch (std::exception& e) {
    vlog.info("%s", e.what());
  }
}

void CleanDesktop::enableWallpaper() {
  try {
    ImpersonateCurrentUser icu;

    if (restoreActiveDesktop) {
      vlog.debug("Restore Active Desktop");

      // -=- First attempt to re-enable Active Desktop
      try {
        ActiveDesktop ad;
        ad.enable(true);
        restoreActiveDesktop = false;
      } catch (std::exception& e) {
        vlog.error("%s", e.what());
      }
    }

    if (restoreWallpaper) {
      vlog.debug("Restore desktop wallpaper");

      // -=- Then restore the standard wallpaper if required
	    SysParamsInfo(SPI_SETDESKWALLPAPER, 0, nullptr, SPIF_SENDCHANGE);
      restoreWallpaper = false;
    }

  } catch (std::exception& e) {
    vlog.info("%s", e.what());
  }
}


void CleanDesktop::disableEffects() {
  try {
    ImpersonateCurrentUser icu;

    vlog.debug("Disable desktop effects");

    SysParamsInfo(SPI_SETFONTSMOOTHING, FALSE, nullptr, SPIF_SENDCHANGE);
    if (SysParamsInfo(SPI_GETUIEFFECTS, 0, &uiEffects, 0) == ERROR_CALL_NOT_IMPLEMENTED) {
      SysParamsInfo(SPI_GETCOMBOBOXANIMATION, 0, &comboBoxAnim, 0);
      SysParamsInfo(SPI_GETGRADIENTCAPTIONS, 0, &gradientCaptions, 0);
      SysParamsInfo(SPI_GETHOTTRACKING, 0, &hotTracking, 0);
      SysParamsInfo(SPI_GETLISTBOXSMOOTHSCROLLING, 0, &listBoxSmoothScroll, 0);
      SysParamsInfo(SPI_GETMENUANIMATION, 0, &menuAnim, 0);
      SysParamsInfo(SPI_SETCOMBOBOXANIMATION, 0, (PVOID)FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETGRADIENTCAPTIONS, 0, (PVOID)FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETHOTTRACKING, 0, (PVOID)FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, (PVOID)FALSE, SPIF_SENDCHANGE);
      SysParamsInfo(SPI_SETMENUANIMATION, 0, (PVOID)FALSE, SPIF_SENDCHANGE);
    } else {
      SysParamsInfo(SPI_SETUIEFFECTS, 0, (PVOID)FALSE, SPIF_SENDCHANGE);

      // We *always* restore UI effects overall, since there is no Windows GUI to do it
      uiEffects = TRUE;
    }
    restoreEffects = true;

  } catch (std::exception& e) {
    vlog.info("%s", e.what());
  }
}

void CleanDesktop::enableEffects() {
  try {
    if (restoreEffects) {
      ImpersonateCurrentUser icu;

      vlog.debug("Restore desktop effects");

      RegKey desktopCfg;
      desktopCfg.openKey(HKEY_CURRENT_USER, "Control Panel\\Desktop");
      SysParamsInfo(SPI_SETFONTSMOOTHING, desktopCfg.getInt("FontSmoothing", 0) != 0, nullptr, SPIF_SENDCHANGE);
      if (SysParamsInfo(SPI_SETUIEFFECTS, 0, (void*)(intptr_t)uiEffects, SPIF_SENDCHANGE) == ERROR_CALL_NOT_IMPLEMENTED) {
        SysParamsInfo(SPI_SETCOMBOBOXANIMATION, 0, (void*)(intptr_t)comboBoxAnim, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETGRADIENTCAPTIONS, 0, (void*)(intptr_t)gradientCaptions, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETHOTTRACKING, 0, (void*)(intptr_t)hotTracking, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETLISTBOXSMOOTHSCROLLING, 0, (void*)(intptr_t)listBoxSmoothScroll, SPIF_SENDCHANGE);
        SysParamsInfo(SPI_SETMENUANIMATION, 0, (void*)(intptr_t)menuAnim, SPIF_SENDCHANGE);
      }
      restoreEffects = false;
    }

  } catch (std::exception& e) {
    vlog.info("%s", e.what());
  }
}
