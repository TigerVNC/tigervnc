/* Copyright 2026 Adam Halim for Cendio AB
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

#ifndef __FADING_PIXELBUFFER_H__
#define __FADING_PIXELBUFFER_H__

#include <pixman.h>

#include <rfb/PixelBuffer.h>

namespace rfb {
  class FadingPixelBuffer : public PixelBuffer {
  public:
    FadingPixelBuffer(const PixelBuffer* parent);
    virtual ~FadingPixelBuffer();

    float getFadeLevel() const { return fadeLevel; }
    void setFadeLevel(float level);

    // Get a pointer into the faded buffer
    virtual const uint8_t* getBuffer(const core::Rect& r, int* stride) const override;

    // Sync the faded buffer with the parent for the given region
    void syncBuffers(const core::Region& r);

  private:
    // Slow fallback in case pixman fails
    void syncBuffersFallback(const core::Region& r);

    // Convert from rfb::PixelFormat to pixman_format_code_t
    pixman_format_code_t rfbToPixmanFormat(PixelFormat format);

  private:
    const PixelBuffer* parent;
    float fadeLevel;
    uint8_t* fadedBuffer;
  };

};

#endif // __FADING_PIXELBUFFER_H__
