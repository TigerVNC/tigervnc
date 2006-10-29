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

#include <string.h>
#include <assert.h>
#include <math.h>

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

// Sinc filter function
double sinc(double x) {
  if (x == 0.0) return 1.0;
  else return sin(pi*x)/(pi*x);
}


//
// -=- ScaleFilters class
//

SFilter &ScaleFilters::operator[](unsigned int filter_id) {
  assert(filter_id < scaleFilterMaxNumber);
  return filters[filter_id];
}

void ScaleFilters::initFilters() {
  filters[scaleFilterNearestNeighbor] = create("Nearest neighbor", 0.5, nearest_neighbor);
  filters[scaleFilterBilinear] = create("Bilinear", 1, linear);
  filters[scaleFilterBicubic] = create("Bicubic", 2, cubic);
  filters[scaleFilterSinc]  = create("Sinc", 4, sinc);
}

SFilter ScaleFilters::create(char *name_, double radius_, filter_func func_) {
  SFilter filter;
  strncpy(filter.name, name_, sizeof(filter.name)-1); 
  filter.name[sizeof(filter.name)-1] = '\0';
  filter.radius = radius_;
  filter.func = func_;
  return filter;
}