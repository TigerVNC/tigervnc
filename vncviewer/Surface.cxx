/* Copyright 2016 Pierre Ossman for Cendio AB
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

#include <FL/Fl_RGB_Image.H>

#include "Surface.h"

Surface::Surface(int width, int height) :
  w(width), h(height)
{
  alloc();
}

Surface::Surface(const Fl_RGB_Image* image) :
  w(image->w()), h(image->h())
{
  alloc();
  update(image);
}

Surface::~Surface()
{
  dealloc();
}

covermap Surface::get_cover_map(int X, int Y, int W, int H, int w, int h)
{
	covermap map;
	int alignedX = (X/w)*w;
	int alignedY = (Y/h)*h;
	int alignedW = ((X+W-1)/w + 1)*w - alignedX;
	int alignedH = ((Y+H-1)/h + 1)*h - alignedY;
	int xSeg = alignedW / w;
	int ySeg = alignedH / h;
	int xi;
	int yi = 0;
	for(xi = 0; xi < xSeg; xi++){
		for(yi = 0; yi < ySeg; yi++){
			int dstX = alignedX + xi * w;
			int dstY = alignedY + yi * h;
			int srcX = 0;
			int srcY = 0;
			int blockW = w;
			int blockH = h;
			int delta = 0;
			if(dstX < X){
				delta = X - dstX;
				srcX += delta;
				blockW -= delta;
				dstX = X;
			}
			if(dstX - delta + w > X + W){
				blockW -= (dstX - delta + w - (X + W));
			}
			delta = 0;
			if(dstY < Y){
				delta = Y - dstY;
				srcY += delta;
				blockH -= delta;
				dstY = Y;
			}
			if(dstY -delta + h > Y + H){
				blockH -= ((dstY - delta + h) - (Y+H));
			}
			blockmap block = std::make_tuple(srcX, srcY, dstX, dstY, blockW, blockH);
			map.push_back(block);
		}
	}
	return map;
}
