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
#include <rfb/LogWriter.h>

#include "MonitorIndicesParameter.h"

using namespace rfb;
static LogWriter vlog("MonitorIndicesParameter");

MonitorIndicesParameter::MonitorIndicesParameter(const char* name_, const char* desc_, const char* v)
: StringParameter(name_, desc_, v) {}

std::set<int> MonitorIndicesParameter::getParam()
{
    bool valid = false;
    std::set<int> indices;
    std::set<int> configIndices;
    std::vector<MonitorIndicesParameter::Monitor> monitors = fetchMonitors();

    if (monitors.size() <= 0) {
        vlog.error(_("Failed to get system monitor configuration"));
        return indices;
    }

    valid = parseIndices(value, &configIndices);
    if (!valid) {
        return indices;
    }

    if (configIndices.size() <= 0) {
        return indices;
    }

    // Go through the monitors and see what indices are present in the config.
    for (int i = 0; i < ((int) monitors.size()); i++) {
        if (std::find(configIndices.begin(), configIndices.end(), i) != configIndices.end())
            indices.insert(monitors[i].fltkIndex);
    }

    return indices;
}

bool MonitorIndicesParameter::setParam(const char* value)
{
    int index;
    std::set<int> indices;

    if (strlen(value) < 0)
        return false;

    if (!parseIndices(value, &indices, true)) {
        vlog.error(_("Invalid configuration specified for %s"), name);
        return false;
    }

    for (std::set<int>::iterator it = indices.begin(); it != indices.end(); it++) {
        index = *it + 1;

        if (index <= 0 || index > Fl::screen_count())
            vlog.error(_("Monitor index %d does not exist"), index);
    }

    return StringParameter::setParam(value);
}

bool MonitorIndicesParameter::setParam(std::set<int> indices)
{
    static const int BUF_MAX_LEN = 1024;
    char buf[BUF_MAX_LEN] = {0};
    std::set<int> configIndices;
    std::vector<MonitorIndicesParameter::Monitor> monitors = fetchMonitors();

    if (monitors.size() <=  0) {
        vlog.error(_("Failed to get system monitor configuration"));
        // Don't return, store the configuration anyways.
    }

    for (int i = 0; i < ((int) monitors.size()); i++) {
        if (std::find(indices.begin(), indices.end(), monitors[i].fltkIndex) != indices.end())
            configIndices.insert(i);
    }

    int bytesWritten = 0;
    char const * separator = "";

    for (std::set<int>::iterator index = configIndices.begin();
         index != configIndices.end();
         index++)
    {
        bytesWritten += snprintf(
            buf+bytesWritten,
            BUF_MAX_LEN-bytesWritten,
            "%s%u",
            separator,
            (*index)+1
        );

        separator = ",";
    }

    return setParam(buf);
}

static bool parseNumber(std::string number, std::set<int> *indices)
{
    if (number.size() <= 0)
        return false;

    int v = strtol(number.c_str(), NULL, 0);

    if (v <= 0)
        return false;

    if (v > INT_MAX)
        return false;

    indices->insert(v-1);
    return true;
}

bool MonitorIndicesParameter::parseIndices(const char* value,
                                           std::set<int> *indices,
                                           bool complain)
{
    char d;
    std::string current;

    for (size_t i = 0; i < strlen(value); i++) {
        d = value[i];

        if (d == ' ')
            continue;
        else if (d >= '0' && d <= '9')
            current.push_back(d);
        else if (d == ',') {
            if (!parseNumber(current, indices)) {
                if (complain)
                    vlog.error(_("Invalid monitor index '%s'"),
                               current.c_str());
                return false;
            }

            current.clear();
        } else {
            if (complain)
                vlog.error(_("Unexpected character '%c'"), d);
            return false;
        }
    }

    // If we have nothing left to parse we are in a valid state.
    if (current.size() == 0)
        return true;

    // Parsing anything we have left.
    if (!parseNumber(current, indices)) {
        if (complain)
            vlog.error(_("Invalid monitor index '%s'"),
                       current.c_str());
        return false;
    }

    return true;
}

std::vector<MonitorIndicesParameter::Monitor> MonitorIndicesParameter::fetchMonitors()
{
    std::vector<Monitor> monitors;

    // Start by creating a struct for every monitor.
    for (int i = 0; i < Fl::screen_count(); i++) {
        Monitor monitor = {0};
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
