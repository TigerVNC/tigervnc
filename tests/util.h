/* Copyright 2013-2014 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __TESTS_UTIL_H__
#define __TESTS_UTIL_H__

typedef void* cpucounter_t;

void startCpuCounter(void);
void endCpuCounter(void);

double getCpuCounter(void);

cpucounter_t newCpuCounter(void);
void freeCpuCounter(cpucounter_t c);

void startCpuCounter(cpucounter_t c);
void endCpuCounter(cpucounter_t c);

double getCpuCounter(cpucounter_t c);

#endif
