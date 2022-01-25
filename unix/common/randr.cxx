/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2017 Pierre Ossman for Cendio AB
 * Copyright 2018 Peter Astrand <astrand@cendio.se> for Cendio AB
 * Copyright 2014 Brian P. Hinz
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

#include <unixcommon.h>
#include <rfb/screenTypes.h>
#include <rfb/LogWriter.h>
#include <RandrGlue.h>
static rfb::LogWriter vlog("RandR");

static int ResizeScreen(bool dryrun, int fb_width, int fb_height,
                        std::set<unsigned int>* disabledOutputs)
{
  vlog.debug("Resizing screen framebuffer to %dx%d", fb_width, fb_height);

  /*
   * Disable outputs which are larger than the target size
   */
  for (int i = 0;i < vncRandRGetOutputCount();i++) {
    int x, y, width, height;
    if (vncRandRGetOutputDimensions(i, &x, &y, &width, &height) == 0) {
      if (x + width > fb_width || y + height > fb_height) {
        char *name = vncRandRGetOutputName(i);
        vlog.debug("Temporarily disabling output '%s'", name);
        free(name);
        if (!dryrun) {
          /* Currently ignoring errors */
          /* FIXME: Save output rotation and restore when configuring output */
          vncRandRDisableOutput(i);
          disabledOutputs->insert(vncRandRGetOutputId(i));
        }
      }
    }
  }

  if (!vncRandRIsValidScreenSize(fb_width, fb_height))
    return 0;

  if (dryrun)
    return 1;

  return vncRandRResizeScreen(fb_width, fb_height);
}


/* Return output index of preferred output, -1 on failure */
int getPreferredScreenOutput(OutputIdMap *outputIdMap,
                             const std::set<unsigned int>& disabledOutputs)
{
  int firstDisabled = -1;
  int firstEnabled = -1;
  int firstConnected = -1;
  int firstUsable = -1;

  for (int i = 0;i < vncRandRGetOutputCount();i++) {
    unsigned int output = vncRandRGetOutputId(i);

    /* In use? */
    if (outputIdMap->count(output) == 1) {
      continue;
    }

    /* Can it be used? */
    if (!vncRandRIsOutputUsable(i)) {
      continue;
    }

    /* Temporarily disabled? */
    if (disabledOutputs.count(output)) {
      if (firstDisabled == -1) firstDisabled = i;
    }

    /* Enabled? */
    if (vncRandRIsOutputEnabled(i)) {
      if (firstEnabled == -1) firstEnabled = i;
    }

    /* Connected? */
    if (vncRandRIsOutputConnected(i)) {
      if (firstConnected == -1) firstConnected = i;
    }

    if (firstUsable == -1) firstUsable = i;
  }

  if (firstEnabled != -1) {
    return firstEnabled;
  } else if (firstDisabled != -1) {
    return firstDisabled;
  } else if (firstConnected != -1) {
    return firstConnected;
  } else {
    return firstUsable; /* Possibly -1 */
  }
}


rfb::ScreenSet computeScreenLayout(OutputIdMap *outputIdMap)
{
  rfb::ScreenSet layout;
  OutputIdMap newIdMap;

  for (int i = 0;i < vncRandRGetOutputCount();i++) {
    unsigned int outputId;
    int x, y, width, height;

    /* Disabled? */
    if (!vncRandRIsOutputEnabled(i))
      continue;

    outputId = vncRandRGetOutputId(i);

    /* Known output? */
    if (outputIdMap->count(outputId) == 1)
      newIdMap[outputId] = (*outputIdMap)[outputId];
    else {
      rdr::U32 id;
      OutputIdMap::const_iterator iter;

      while (true) {
        id = rand();
        for (iter = outputIdMap->begin();iter != outputIdMap->end();++iter) {
          if (iter->second == id)
            break;
        }
        if (iter == outputIdMap->end())
          break;
      }

      newIdMap[outputId] = id;
    }

    if (vncRandRGetOutputDimensions(i, &x, &y, &width, &height) == 0) {
      layout.add_screen(rfb::Screen(newIdMap[outputId], x, y, width, height, 0));
    }
  }

  /* Only keep the entries that are currently active */
  *outputIdMap = newIdMap;

  /*
   * Make sure we have something to display. Hopefully it's just temporary
   * that we have no active outputs...
   */
  if (layout.num_screens() == 0)
    layout.add_screen(rfb::Screen(0, 0, 0, vncGetScreenWidth(),
                                  vncGetScreenHeight(), 0));

  return layout;
}

static unsigned int _setScreenLayout(bool dryrun,
                                     int fb_width, int fb_height, const rfb::ScreenSet& layout,
                                     OutputIdMap *outputIdMap)
{
  int ret;
  int availableOutputs;
  std::set<unsigned int> disabledOutputs;
  /* Printing errors in the dryrun pass might be confusing */
  const bool logErrors = !dryrun || vlog.getLevel() >= vlog.LEVEL_DEBUG;

  // RandR support?
  if (vncRandRGetOutputCount() == 0)
    return rfb::resultProhibited;

  /*
   * First check that we don't have any active clone modes. That's just
   * too messy to deal with.
   */
  if (vncRandRHasOutputClones()) {
    if (logErrors) {
      vlog.error("Clone mode active. Refusing to touch screen layout.");
    }
    return rfb::resultInvalid;
  }

  /* Next count how many useful outputs we have... */
  availableOutputs = vncRandRGetAvailableOutputs();

  /* Try to create more outputs if needed... (only works on Xvnc) */
  if (layout.num_screens() > availableOutputs) {
    vlog.debug("Insufficient screens. Need to create %d more.",
               layout.num_screens() - availableOutputs);

    if (!vncRandRCanCreateOutputs(layout.num_screens() - availableOutputs)) {
      if (logErrors)
        vlog.error("Unable to create more screens, as needed by the new client layout.");
      return rfb::resultInvalid;
    }

    if (!dryrun) {
      ret = vncRandRCreateOutputs(layout.num_screens() - availableOutputs);
      if (!ret) {
        if (logErrors)
          vlog.error("Unable to create more screens, as needed by the new client layout.");
        return rfb::resultInvalid;
      }
    }
  }

  /* First we might need to resize the screen */
  if ((fb_width != vncGetScreenWidth()) ||
      (fb_height != vncGetScreenHeight())) {
    ret = ResizeScreen(dryrun, fb_width, fb_height, &disabledOutputs);
    if (!ret) {
      if (logErrors) {
        vlog.error("Failed to resize screen to %dx%d", fb_width, fb_height);
      }
      return rfb::resultInvalid;
    }
  }

  /* Next, reconfigure all known outputs */
  for (int i = 0;i < vncRandRGetOutputCount();i++) {
    unsigned int output;

    rfb::ScreenSet::const_iterator iter;

    output = vncRandRGetOutputId(i);

    /* Known? */
    if (outputIdMap->count(output) == 0)
      continue;

    /* Find the corresponding screen... */
    for (iter = layout.begin();iter != layout.end();++iter) {
      if (iter->id == (*outputIdMap)[output])
        break;
    }

    /* Missing? */
    if (iter == layout.end()) {
      outputIdMap->erase(output);
      continue;
    }

    /* Probably not needed, but let's be safe */
    if (!vncRandRIsOutputUsable(i)) {
      if (logErrors) {
        char *name = vncRandRGetOutputName(i);
        vlog.error("Required output '%s' cannot be used", name);
        free(name);
      }
      return rfb::resultInvalid;
    }

    /* Possible mode? */
    if (!vncRandRCheckOutputMode(i, iter->dimensions.width(),
                                 iter->dimensions.height())) {
      if (logErrors) {
        char *name = vncRandRGetOutputName(i);
        vlog.error("Output '%s' does not support required mode %dx%d", name,
                   iter->dimensions.width(), iter->dimensions.height());
        free(name);
      }
      return rfb::resultInvalid;
    }

    char *name = vncRandRGetOutputName(i);
    vlog.debug("Reconfiguring output '%s' to %dx%d+%d+%d", name,
               iter->dimensions.width(), iter->dimensions.height(),
               iter->dimensions.tl.x, iter->dimensions.tl.y);
    free(name);

    if (dryrun)
      continue;

    /* Reconfigure new mode and position */
    ret = vncRandRReconfigureOutput(i,
                                    iter->dimensions.tl.x,
                                    iter->dimensions.tl.y,
                                    iter->dimensions.width(),
                                    iter->dimensions.height());
    if (!ret) {
      if (logErrors) {
        char *name = vncRandRGetOutputName(i);
        vlog.error("Failed to reconfigure output '%s' to %dx%d+%d+%d", name,
                   iter->dimensions.width(), iter->dimensions.height(),
                   iter->dimensions.tl.x, iter->dimensions.tl.y);
        free(name);
      }
      return rfb::resultInvalid;
    }
  }

  /* Allocate new outputs for new screens */
  rfb::ScreenSet::const_iterator iter;
  for (iter = layout.begin();iter != layout.end();++iter) {
    OutputIdMap::const_iterator oi;
    unsigned int output;
    int i;

    /* Does this screen have an output already? */
    for (oi = outputIdMap->begin();oi != outputIdMap->end();++oi) {
      if (oi->second == iter->id)
        break;
    }

    if (oi != outputIdMap->end())
      continue;

    /* Find an unused output */
    i = getPreferredScreenOutput(outputIdMap, disabledOutputs);

    /* Shouldn't happen */
    if (i == -1) {
      if (logErrors)
        vlog.error("Cannot find an available output for new screen layout");
      return rfb::resultInvalid;
    }
    output = vncRandRGetOutputId(i);

    /*
     * Make sure we already have an entry for this, or
     * computeScreenLayout() will think it is a brand new output and
     * assign it a random id.
     */
    (*outputIdMap)[output] = iter->id;

    /* Probably not needed, but let's be safe */
    if (!vncRandRIsOutputUsable(i)) {
      if (logErrors) {
        char *name = vncRandRGetOutputName(i);
        vlog.error("Required new output '%s' cannot be used", name);
        free(name);
      }
      return rfb::resultInvalid;
    }

    /* Possible mode? */
    if (!vncRandRCheckOutputMode(i, iter->dimensions.width(),
                                 iter->dimensions.height())) {
      if (logErrors) {
        char *name = vncRandRGetOutputName(i);
        vlog.error("New output '%s' does not support required mode %dx%d", name,
                   iter->dimensions.width(), iter->dimensions.height());
        free(name);
      }
      return rfb::resultInvalid;
    }

    char *name = vncRandRGetOutputName(i);
    vlog.debug("Reconfiguring new output '%s' to %dx%d+%d+%d", name,
               iter->dimensions.width(), iter->dimensions.height(),
               iter->dimensions.tl.x, iter->dimensions.tl.y);
    free(name);

    if (dryrun)
      continue;

    /* Reconfigure new mode and position */
    ret = vncRandRReconfigureOutput(i,
                                    iter->dimensions.tl.x,
                                    iter->dimensions.tl.y,
                                    iter->dimensions.width(),
                                    iter->dimensions.height());
    if (!ret) {
      if (logErrors) {
        char *name = vncRandRGetOutputName(i);
        vlog.error("Failed to reconfigure new output '%s' to %dx%d+%d+%d", name,
                   iter->dimensions.width(), iter->dimensions.height(),
                   iter->dimensions.tl.x, iter->dimensions.tl.y);
        free(name);
      }
      return rfb::resultInvalid;
    }
  }

  /* Turn off unused outputs */
  for (int i = 0;i < vncRandRGetOutputCount();i++) {
    unsigned int output = vncRandRGetOutputId(i);

    /* Known? */
    if (outputIdMap->count(output) == 1)
      continue;

    /* Enabled? */
    if (!vncRandRIsOutputEnabled(i))
      continue;

    /* Disable and move on... */
    ret = vncRandRDisableOutput(i);
    char *name = vncRandRGetOutputName(i);
    if (ret) {
      vlog.debug("Disabled unused output '%s'", name);
    } else {
      if (logErrors) {
        vlog.error("Failed to disable unused output '%s'", name);
      }
      free(name);
      return rfb::resultInvalid;
    }
    free(name);
  }

  /*
   * Update timestamp for when screen layout was last changed.
   * This is normally done in the X11 request handlers, which is
   * why we have to deal with it manually here.
   */
  vncRandRUpdateSetTime();

  return rfb::resultSuccess;
}


unsigned int setScreenLayout(int fb_width, int fb_height, const rfb::ScreenSet& layout,
                             OutputIdMap *outputIdMap)
{
    return _setScreenLayout(false, fb_width, fb_height, layout, outputIdMap);
}


unsigned int tryScreenLayout(int fb_width, int fb_height, const rfb::ScreenSet& layout,
                             OutputIdMap *outputIdMap)
{
    OutputIdMap dryrunIdMap = *outputIdMap;
    return _setScreenLayout(true, fb_width, fb_height, layout, &dryrunIdMap);
}
