/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2015 Pierre Ossman for Cendio AB
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
/*  This is the xf86 module code for the vnc extension.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "opaque.h"
#include "randrstr.h"

#include "xorg-version.h"

#include "xf86.h"
#include "xf86Module.h"

#include "vncExtInit.h"
#include "RFBGlue.h"
#include "XorgGlue.h"
#include "RandrGlue.h"

static void vncModuleInit(INITARGS);

static MODULESETUPPROTO(vncSetup);

ExtensionModule vncExt =
{
    vncModuleInit,
    "VNC",
    NULL
};

static XF86ModuleVersionInfo vncVersRec =
{
    "vnc",
    "TigerVNC Project",
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_EXTENSION,         /* needs the server extension ABI */
    ABI_EXTENSION_VERSION,
    MOD_CLASS_EXTENSION,
    {0,0,0,0}
};

_X_EXPORT XF86ModuleData vncModuleData = { &vncVersRec, vncSetup, NULL };

static void *
vncSetup(void * module, void * opts, int *errmaj, int *errmin) {
    LoadExtensionList(&vncExt, 1, FALSE);
    /* Need a non-NULL return value to indicate success */
    return (void *)1;
}

static void vncModuleInit(INITARGS)
{
  static char once = 0;

  if (!once) {
    once++;

    vncInitRFB();

    for (int scr = 0; scr < screenInfo.numScreens; scr++) {
      ScrnInfoPtr pScrn;
      XF86OptionPtr option;

      pScrn = xf86Screens[scr];
      option = pScrn->options;
      while (option != NULL) {
        vncSetParam(xf86OptionName(option), xf86OptionValue(option));
        option = xf86NextOption(option);
      }
    }
  }

  vncExtensionInit();
}

void vncClientGone(int fd)
{
}

int vncRandRCanCreateScreenOutputs(int scrIdx, int extraOutputs)
{
  return 0;
}

int vncRandRCreateScreenOutputs(int scrIdx, int extraOutputs)
{
  return 0;
}

int vncRandRCanCreateModes(void)
{
  return 0;
}

void* vncRandRCreateMode(void* output, int width, int height)
{
  return 0;
}

void* vncRandRSetPreferredMode(void* output, void* mode)
{
  /*
   * We're not going to change which modes are preferred,
   * so just return the incoming mode.
   */
  return mode;
}
