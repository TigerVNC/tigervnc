/* Copyright 2009-2026 Pierre Ossman for Cendio AB
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

#include <stdio.h>
#include <string.h>

#include <set>

#include <rfb/ScreenSet.h>

using namespace rfb;

void ScreenSet::add_screen(const Screen screen)
{
  screens.push_back(screen);
}

void ScreenSet::remove_screen(uint32_t id)
{
  std::list<Screen>::iterator iter, nextiter;
  for (iter = screens.begin();iter != screens.end();iter = nextiter) {
    nextiter = iter; nextiter++;
    if (iter->id == id)
        screens.erase(iter);
  }
}

bool ScreenSet::validate(int fb_width, int fb_height) const
{
  std::list<Screen>::const_iterator iter;
  std::set<uint32_t> seen_ids;
  core::Rect fb_rect;

  if (screens.empty())
    return false;
  if (num_screens() > 255)
    return false;

  fb_rect.setXYWH(0, 0, fb_width, fb_height);

  for (iter = screens.begin();iter != screens.end();++iter) {
    if (iter->dimensions.is_empty())
      return false;
    if (!iter->dimensions.enclosed_by(fb_rect))
      return false;
    if (seen_ids.find(iter->id) != seen_ids.end())
      return false;
    seen_ids.insert(iter->id);
  }

  return true;
};

void ScreenSet::print(char* str, size_t len) const
{
  char buffer[128];
  std::list<Screen>::const_iterator iter;
  snprintf(buffer, sizeof(buffer), "%d screen(s)\n", num_screens());
  str[0] = '\0';
  strncat(str, buffer, len - 1 - strlen(str));
  for (iter = screens.begin();iter != screens.end();++iter) {
    snprintf(buffer, sizeof(buffer),
             "    %10d (0x%08x): %dx%d+%d+%d (flags 0x%08x)\n",
             (int)iter->id, (unsigned)iter->id,
             iter->dimensions.width(), iter->dimensions.height(),
             iter->dimensions.tl.x, iter->dimensions.tl.y,
             (unsigned)iter->flags);
    strncat(str, buffer, len - 1 - strlen(str));
  }
};

static inline bool compare_screen(const Screen& first,
                                  const Screen& second)
{
  return first.id < second.id;
}

bool ScreenSet::operator==(const ScreenSet& r) const
{
  std::list<Screen> a = screens;
  a.sort(compare_screen);
  std::list<Screen> b = r.screens;
  b.sort(compare_screen);
  return a == b;
};

bool ScreenSet::operator!=(const ScreenSet& r) const
{
  return !operator==(r);
}
