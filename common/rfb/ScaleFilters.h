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

// -=- ScaleFilters.h
//
// Definitions of the 1-D filters and routines using for the image scaling.
//  
// 

#include <math.h>

namespace rfb {

  typedef double (*filter_func)(double x);

  const double pi = 3.14159265358979;

  const unsigned int scaleFilterNearestNeighbor = 0;
  const unsigned int scaleFilterBilinear = 1;
  const unsigned int scaleFilterBicubic = 2;
  const unsigned int scaleFilterSinc = 3;

  const unsigned int scaleFiltersMax = 10;

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

  class ScaleFilter {
  public:
    char name[30];
    int radius;
    filter_func func;
  };

};
