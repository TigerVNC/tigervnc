/* Copyright © 2019 Wim Taymans
 *
 * SPDX-License-Identifier: MIT
 */

// Adapted from spa/include/spa/utils/dll.h from PipeWire

#ifndef __DLL_H__
#define __DLL_H__

#include <math.h>

const double dllBwMax = 0.128;
const double dllBwMin = 0.016;

class DLL {
public:
  DLL() { init(); }

  void init()
  {
    bw = 0.0;
    z1 = z2 = z3 = 0.0;
  }

  void setBw(double bw_, unsigned period, unsigned rate)
  {
    double w = 2 * M_PI * bw_ * period / rate;
    w0 = 1.0 - exp(-20.0 * w);
    w1 = w * 1.5 / period;
    w2 = w / 1.5;
    bw = bw_;
  }

  double getBw() const { return bw; }

  double update(double err)
  {
    z1 += w0 * (w1 * err - z1);
    z2 += w0 * (z1 - z2);
    z3 += w2 * z2;
    return 1.0 - (z2 + z3);
  }

private:
  double bw;
  double z1, z2, z3;
  double w0, w1, w2;
};

#endif
