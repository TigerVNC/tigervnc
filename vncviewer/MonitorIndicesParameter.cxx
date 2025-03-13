/* Copyright 2021 Hugo Lundin <huglu@cendio.se> for Cendio AB.
 * Copyright 2021 Pierre Ossman for Cendio AB
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

#include <algorithm>
#include <vector>
#include <string>
#include <limits>
#include <set>
#include <stdlib.h>
#include <stdexcept>

#include "i18n.h"

#include <FL/Fl.H>

#include <core/LogWriter.h>

#include "MonitorIndicesParameter.h"

static core::LogWriter vlog("MonitorIndicesParameter");

MonitorIndicesParameter::MonitorIndicesParameter(const char* name_,
                                                 const char* desc_,
                                                 const ListType& v)
: IntListParameter(name_, desc_, v, 1, INT_MAX) {}

std::set<int> MonitorIndicesParameter::getMonitors() const
{
    std::set<int> indices;
    std::vector<MonitorIndicesParameter::Monitor> monitors = fetchMonitors();

    if (monitors.size() <= 0) {
        vlog.error(_("Failed to get system monitor configuration"));
        return indices;
    }

    // Go through the monitors and see what indices are present in the config.
    for (int i = 0; i < ((int) monitors.size()); i++) {
        if (std::find(begin(), end(), i+1) != end())
            indices.insert(monitors[i].fltkIndex);
    }

    return indices;
}

bool MonitorIndicesParameter::setParam(const char* v)
{
    if (!IntListParameter::setParam(v))
        return false;

    for (int index : value) {
        if (index <= 0 || index > Fl::screen_count())
            vlog.error(_("Monitor index %d does not exist"), index);
    }

    return true;
}

void MonitorIndicesParameter::setMonitors(const std::set<int>& indices)
{
    std::list<int> configIndices;
    std::vector<MonitorIndicesParameter::Monitor> monitors = fetchMonitors();

    if (monitors.size() <=  0) {
        vlog.error(_("Failed to get system monitor configuration"));
        // Don't return, store the configuration anyways.
    }

    for (int i = 0; i < ((int) monitors.size()); i++) {
        if (std::find(indices.begin(), indices.end(), monitors[i].fltkIndex) != indices.end())
            configIndices.push_back(i+1);
    }

    IntListParameter::setParam(configIndices);
}

std::vector<MonitorIndicesParameter::Monitor> MonitorIndicesParameter::fetchMonitors()
{
    std::vector<Monitor> monitors;

    // Start by creating a struct for every monitor.
    for (int i = 0; i < Fl::screen_count(); i++) {
        Monitor monitor;
        bool match;

        // Get the properties of the monitor at the current index;
        Fl::screen_xywh(
            monitor.x,
            monitor.y,
            monitor.w,
            monitor.h,
            i
        );

        // Only keep a single entry for mirrored screens
        match = false;
        for (int j = 0; j < ((int) monitors.size()); j++) {
            if (monitors[j].x != monitor.x)
                continue;
            if (monitors[j].y != monitor.y)
                continue;
            if (monitors[j].w != monitor.w)
                continue;
            if (monitors[j].h != monitor.h)
                continue;

            match = true;
            break;
        }
        if (match)
            continue;

        monitor.fltkIndex = i;
        monitors.push_back(monitor);
    }

    // Sort the monitors according to the specification in the vncviewer manual.
    qsort(&monitors[0], monitors.size(), sizeof(*(&monitors[0])), compare);
    return monitors;
}

int MonitorIndicesParameter::compare(const void *a, const void *b)
{
    MonitorIndicesParameter::Monitor * monitor1 = (MonitorIndicesParameter::Monitor *) a;
    MonitorIndicesParameter::Monitor * monitor2 = (MonitorIndicesParameter::Monitor *) b;

    if (monitor1->x < monitor2->x)
        return -1;

    if (monitor1->x > monitor2->x)
        return 1;

    if (monitor1->y < monitor2->y)
        return -1;

    if (monitor1->y > monitor2->y)
        return 1;

    return 0;
}
