/* Copyright 2026 jose-pr
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

#include <core/LogWriter.h>
#include <core/i18n.h>

#include "AudioOutput.h"

#ifdef HAVE_AUDIO
#ifdef WIN32
#include "AudioOutputWin32.h"
typedef AudioOutputWin32 AudioOutputPlatform;
#else
#include "AudioOutputPulse.h"
typedef AudioOutputPulse AudioOutputPlatform;
#endif
#endif

static core::LogWriter vlog("AudioOutput");

AudioOutput* AudioOutput::create()
{
#ifdef HAVE_AUDIO
  AudioOutputPlatform* audio;

  audio = new AudioOutputPlatform();
  if (!audio->isAvailable()) {
    delete audio;
    vlog.info(_("No audio playback device available"));
    return nullptr;
  }

  return audio;
#else
  vlog.debug("No audio playback support for this platform");
  return nullptr;
#endif
}
