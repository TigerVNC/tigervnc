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

namespace rfb {

  #define SCALE_ERROR (1e-7)
  #define BITS_OF_CHANEL 8
  #define BITS_OF_WEIGHT 14
  #define FINALSHIFT (2 * BITS_OF_WEIGHT - BITS_OF_CHANEL)
  #define WEIGHT_OF_ONE (1 << BITS_OF_WEIGHT)

  typedef double (*filter_func)(double x);

  const double pi = 3.14159265358979;

  const unsigned int scaleFilterNearestNeighbor = 0;
  const unsigned int scaleFilterBilinear = 1;
  const unsigned int scaleFilterBicubic = 2;

  const unsigned int scaleFilterMaxNumber = 2;
  const unsigned int defaultScaleFilter = scaleFilterBilinear;

  //
  // -=- Scale filters structures and routines
  //

  // Scale filter stuct
  typedef struct {
    char name[30];     // Filter name
    double radius;     // Radius where filter function is nonzero
    filter_func func;  // Pointer to filter function
  } SFilter;

  // Scale filter weight table
  typedef struct {
    short i0, i1;  // Filter function interval, [i0..i1)
    short *weight;    // Weight coefficients on the filter function interval
  } SFilterWeightTab;


  // ScaleFilters class helps us using a set of 1-d scale filters.
  class ScaleFilters {
  public:
    ScaleFilters() { initFilters(); };

    SFilter &operator[](unsigned int filter_id);

    int getFilterIdByName(char *filterName);

    void makeWeightTabs(int filter, int src_x, int dst_x, SFilterWeightTab **weightTabs);

  protected:
    void initFilters();

    SFilter create(const char *name_, double radius_, filter_func func_);

    SFilter filters[scaleFilterMaxNumber+1];
  };

};
