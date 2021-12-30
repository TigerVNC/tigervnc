/* Copyright (C) 2006 TightVNC Team.  All Rights Reserved.
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

#include <string.h>
#include <assert.h>
#include <math.h>

#include <rfb/Rect.h>
#include <rfb/ScaleFilters.h>

using namespace rfb;

//
// -=- 1-D filters functions
//

// Nearest neighbor filter function
double nearest_neighbor(double x) {
  if (x < -0.5) return 0.0;
  if (x < 0.5) return 1.0;
  return 0.0;
}

// Linear filter function
double linear(double x) {
  if (x < -1.0) return 0.0;
  if (x < 0.0) return 1.0+x;
  if (x < 1.0) return 1.0-x;
  return 0.0;
}

// Cubic filter functions
double cubic(double x) {
  double t;
  if (x < -2.0) return 0.0;
  if (x < -1.0) {t = 2.0+x; return t*t*t/6.0;}
  if (x < 0.0) return (4.0+x*x*(-6.0+x*-3.0))/6.0;
  if (x < 1.0) return (4.0+x*x*(-6.0+x*3.0))/6.0;
  if (x < 2.0) {t = 2.0-x; return t*t*t/6.0;}
  return 0.0;
}


//
// -=- ScaleFilters class
//

SFilter &ScaleFilters::operator[](unsigned int filter_id) {
  assert(filter_id <= scaleFilterMaxNumber);
  return filters[filter_id];
}

int ScaleFilters::getFilterIdByName(char *filterName) {
  for (unsigned int i = 0; i <= scaleFilterMaxNumber; i++) {
    if (strcasecmp(filters[i].name, filterName) == 0) return i;
  }
  return -1;
}

void ScaleFilters::initFilters() {
  filters[scaleFilterNearestNeighbor] = create("Nearest neighbor", 0.5, nearest_neighbor);
  filters[scaleFilterBilinear] = create("Bilinear", 1, linear);
  filters[scaleFilterBicubic] = create("Bicubic", 2, cubic);
}

SFilter ScaleFilters::create(const char *name_, double radius_, filter_func func_) {
  SFilter filter;
  strncpy(filter.name, name_, sizeof(filter.name)-1); 
  filter.name[sizeof(filter.name)-1] = '\0';
  filter.radius = radius_;
  filter.func = func_;
  return filter;
}

void ScaleFilters::makeWeightTabs(int filter_id, int src_x, int dst_x, SFilterWeightTab **pWeightTabs) {
  double sxc;
  double offset = 0.5;
  double ratio = (double)dst_x / src_x;
  double sourceScale  = __rfbmax(1.0, 1.0/ratio);
  double sourceRadius = __rfbmax(0.5, sourceScale * filters[filter_id].radius);
  double sum, nc;
  int i, ci;

  SFilter sFilter = filters[filter_id];
  
  *pWeightTabs = new SFilterWeightTab[dst_x];
  SFilterWeightTab *weightTabs = *pWeightTabs;

  // Make the weight tab for the each dest x position
  for (int x = 0; x < dst_x; x++) {
    sxc = (double(x)+offset) / ratio;

    // Calculate the scale filter interval, [i0, i1)
    int i0 = int(__rfbmax(sxc-sourceRadius+0.5, 0));
    int i1 = int(__rfbmin(sxc+sourceRadius+0.5, src_x));

    weightTabs[x].i0 = i0; weightTabs[x].i1 = i1;
    weightTabs[x].weight = new short[i1-i0];

    // Calculate coeff to normalize the filter weights
    for (sum = 0, i = i0; i < i1; i++) sum += sFilter.func((double(i)-sxc+0.5)/sourceScale);
    if (sum == 0.) nc = (double)(WEIGHT_OF_ONE); else nc = (double)(WEIGHT_OF_ONE)/sum;


    // Calculate the weight coeffs on the scale filter interval
    for (ci = 0, i = i0; i < i1; i++) {
      weightTabs[x].weight[ci++] = (short)floor((sFilter.func((double(i)-sxc+0.5)/sourceScale) * nc) + 0.5);
    }
  }
}
