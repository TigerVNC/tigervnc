/* Copyright 2021 Hugo Lundin <huglu@cendio.se> for Cendio AB.
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

#ifndef __MONITOR_INDEX_PARAMETER_H
#define __MONITOR_INDEX_PARAMETER_H

#include <set>
#include <vector>

#include <rfb/Configuration.h>

class MonitorIndicesParameter: public rfb::StringParameter {
public:
    MonitorIndicesParameter(const char* name_, const char* desc_, const char* v);
    std::set<int> getParam();
    bool setParam(std::set<int> indices);
    bool setParam(const char* value);
private:
    typedef struct {
        int x, y, w, h;
        int fltkIndex;
    } Monitor;

    bool parseIndices(const char* value, std::set<int> *indices,
                      bool complain=false);
    std::vector<MonitorIndicesParameter::Monitor> fetchMonitors();
    static int compare(const void*, const void*);
};

#endif // __MONITOR_INDEX_PARAMETER_H
