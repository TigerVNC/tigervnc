/* Copyright 2024 Pierre Ossman for Cendio AB
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "vncExtInit.h"
#include "vncPresent.h"

#include <present.h>

static RRCrtcPtr vncPresentGetCrtc(WindowPtr window)
{
  ScreenPtr pScreen = window->drawable.pScreen;
  rrScrPrivPtr rp = rrGetScrPriv(pScreen);

  /* All output is synchronized, so just pick the first active crtc */
  for (int c = 0; c < rp->numCrtcs; c++) {
    RRCrtcPtr crtc;

    crtc = rp->crtcs[c];
    if (crtc->mode == NULL)
      continue;

    return crtc;
  }

  return NULL;
}

static int vncPresentGetUstMsc(RRCrtcPtr crtc, CARD64 *ust, CARD64 *msc)
{
  *ust = GetTimeInMicros();
  *msc = vncGetMsc(crtc->pScreen->myNum);

  return Success;
}

static int vncPresentQueueVBlank(RRCrtcPtr crtc, uint64_t event_id,
                                 uint64_t msc)
{
  vncQueueMsc(crtc->pScreen->myNum, event_id, msc);
  return Success;
}

void vncPresentMscEvent(uint64_t id, uint64_t msc)
{
  present_event_notify(id, GetTimeInMicros(), msc);
}

static void vncPresentAbortVBlank(RRCrtcPtr crtc, uint64_t event_id,
                                  uint64_t msc)
{
  vncAbortMsc(crtc->pScreen->myNum, event_id);
}

static void vncPresentFlush(WindowPtr window)
{
}

static present_screen_info_rec vncPresentScreenInfo = {
  .version = PRESENT_SCREEN_INFO_VERSION,

  .get_crtc = vncPresentGetCrtc,
  .get_ust_msc = vncPresentGetUstMsc,
  .queue_vblank = vncPresentQueueVBlank,
  .abort_vblank = vncPresentAbortVBlank,
  .flush = vncPresentFlush,

  .capabilities = PresentCapabilityNone,
};

Bool
vncPresentInit(ScreenPtr screen)
{
  return present_screen_init(screen, &vncPresentScreenInfo);
}