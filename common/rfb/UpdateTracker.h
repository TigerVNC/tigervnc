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

#ifndef __RFB_UPDATETRACKER_INCLUDED__
#define __RFB_UPDATETRACKER_INCLUDED__

#include <core/Rect.h>
#include <core/Region.h>

namespace rfb {

  class UpdateInfo {
  public:
    core::Region changed;
    core::Region copied;
    core::Point copy_delta;
    bool is_empty() const {
      return copied.is_empty() && changed.is_empty();
    }
    // NOTE: We do not ever use UpdateInfo::numRects(), because Tight encoding
    //       complicates computing the number of rectangles.
    /*
    int numRects() const {
      return copied.numRects() + changed.numRects();
    }
    */
  };

  class UpdateTracker {
  public:
    UpdateTracker() {};
    virtual ~UpdateTracker() {};

    virtual void add_changed(const core::Region& region) = 0;
    virtual void add_copied(const core::Region& dest,
                            const core::Point& delta) = 0;
  };

  class ClippingUpdateTracker : public UpdateTracker {
  public:
    ClippingUpdateTracker() : ut(nullptr) {}
    ClippingUpdateTracker(UpdateTracker* ut_, const core::Rect& r={}) : ut(ut_), clipRect(r) {}
    
    void setUpdateTracker(UpdateTracker* ut_) {ut = ut_;}
    void setClipRect(const core::Rect& cr) {clipRect = cr;}

    void add_changed(const core::Region& region) override;
    void add_copied(const core::Region& dest,
                    const core::Point& delta) override;
  protected:
    UpdateTracker* ut;
    core::Rect clipRect;
  };

  class SimpleUpdateTracker : public UpdateTracker {
  public:
    SimpleUpdateTracker();
    virtual ~SimpleUpdateTracker();

    void add_changed(const core::Region& region) override;
    void add_copied(const core::Region& dest,
                    const core::Point& delta) override;
    virtual void subtract(const core::Region& region);

    // Fill the supplied UpdateInfo structure with update information
    // FIXME: Provide getUpdateInfo() with no clipping, for better efficiency.
    virtual void getUpdateInfo(UpdateInfo* info,
                               const core::Region& cliprgn);

    // Copy the contained updates to another tracker
    virtual void copyTo(UpdateTracker* to) const;

    // Move the entire update region by an offset
    void translate(const core::Point& p) {changed.translate(p); copied.translate(p);}

    virtual bool is_empty() const {return changed.is_empty() && copied.is_empty();}

    virtual void clear() {changed.clear(); copied.clear();};
  protected:
    core::Region changed;
    core::Region copied;
    core::Point copy_delta;
  };

}

#endif // __RFB_UPDATETRACKER_INCLUDED__
