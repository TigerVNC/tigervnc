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

#ifndef __RFB_COMPARINGUPDATETRACKER_H__
#define __RFB_COMPARINGUPDATETRACKER_H__

#include <rfb/UpdateTracker.h>

namespace rfb {

  class ComparingUpdateTracker : public SimpleUpdateTracker {
  public:
    ComparingUpdateTracker(PixelBuffer* buffer);
    ~ComparingUpdateTracker();

    // compare() does the comparison and reduces its changed and copied regions
    // as appropriate. Returns true if the regions were altered.

    virtual bool compare();

    // enable()/disable() turns the comparing functionality on/off. With it
    // disabled, the object will behave like a dumb update tracker (i.e.
    // compare() will be a no-op). It is harmless to repeatedly call these
    // methods.

    virtual void enable();
    virtual void disable();

    void logStats();

  private:
    void compareRect(const Rect& r, Region* newchanged);
    PixelBuffer* fb;
    ManagedPixelBuffer oldFb;
    bool firstCompare;
    bool enabled;

    unsigned long long totalPixels, missedPixels;
  };

}
#endif
