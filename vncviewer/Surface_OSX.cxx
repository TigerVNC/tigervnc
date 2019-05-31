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

#include <assert.h>

#include <ApplicationServices/ApplicationServices.h>

#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>

#include <rdr/Exception.h>

#include "cocoa.h"
#include "Surface.h"

static CGColorSpaceRef srgb = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

static CGImageRef create_image(CGColorSpaceRef lut,
                               const unsigned char* data,
                               int w, int h, bool skip_alpha)
{
  CGDataProviderRef provider;
  CGImageAlphaInfo alpha;

  CGImageRef image;

  provider = CGDataProviderCreateWithData(NULL, data,
                                          w * h * 4, NULL);
  if (!provider)
    throw rdr::Exception("CGDataProviderCreateWithData");

  // FIXME: This causes a performance hit, but is necessary to avoid
  //        artifacts in the edges of the window
  if (skip_alpha)
    alpha = kCGImageAlphaNoneSkipFirst;
  else
    alpha = kCGImageAlphaPremultipliedFirst;

  image = CGImageCreate(w, h, 8, 32, w * 4, lut,
                        alpha | kCGBitmapByteOrder32Little,
                        provider, NULL, false, kCGRenderingIntentDefault);
  CGDataProviderRelease(provider);
  if (!image)
    throw rdr::Exception("CGImageCreate");

  return image;
}

static void render(CGContextRef gc, CGColorSpaceRef lut,
                   const unsigned char* data,
                   CGBlendMode mode, CGFloat alpha,
                   int src_x, int src_y, int src_w, int src_h,
                   int x, int y, int w, int h)
{
  CGRect rect;
  CGImageRef image, subimage;

  image = create_image(lut, data, src_w, src_h, mode == kCGBlendModeCopy);

  rect.origin.x = src_x;
  rect.origin.y = src_y;
  rect.size.width = w;
  rect.size.height = h;

  subimage = CGImageCreateWithImageInRect(image, rect);
  if (!subimage)
    throw rdr::Exception("CGImageCreateImageWithImageInRect");

  CGContextSaveGState(gc);

  CGContextSetBlendMode(gc, mode);
  CGContextSetAlpha(gc, alpha);

  rect.origin.x = x;
  rect.origin.y = y;
  rect.size.width = w;
  rect.size.height = h;

  CGContextDrawImage(gc, rect, subimage);

  CGContextRestoreGState(gc);

  CGImageRelease(subimage);
  CGImageRelease(image);
}

static CGContextRef make_bitmap(int width, int height, unsigned char* data)
{
  CGContextRef bitmap;

  bitmap = CGBitmapContextCreate(data, width, height, 8, width*4, srgb,
                                 kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little);
  if (!bitmap)
    throw rdr::Exception("CGBitmapContextCreate");

  return bitmap;
}

void Surface::clear(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  unsigned char* out;
  int x, y;

  r = (unsigned)r * a / 255;
  g = (unsigned)g * a / 255;
  b = (unsigned)b * a / 255;

  out = data;
  for (y = 0;y < width();y++) {
    for (x = 0;x < height();x++) {
      *out++ = b;
      *out++ = g;
      *out++ = r;
      *out++ = a;
    }
  }
}

void Surface::draw(int src_x, int src_y, int x, int y, int w, int h)
{
  CGColorSpaceRef lut;

  CGContextSaveGState(fl_gc);

  // Reset the transformation matrix back to the default identity
  // matrix as otherwise we get a massive performance hit
  CGContextConcatCTM(fl_gc, CGAffineTransformInvert(CGContextGetCTM(fl_gc)));

  // macOS Coordinates are from bottom left, not top left
  y = Fl_Window::current()->h() - (y + h);

  lut = cocoa_win_color_space(Fl_Window::current());
  render(fl_gc, lut, data, kCGBlendModeCopy, 1.0,
         src_x, src_y, width(), height(), x, y, w, h);
  CGColorSpaceRelease(lut);

  CGContextRestoreGState(fl_gc);
}

void Surface::draw(Surface* dst, int src_x, int src_y, int x, int y, int w, int h)
{
  CGContextRef bitmap;

  bitmap = make_bitmap(dst->width(), dst->height(), dst->data);

  // macOS Coordinates are from bottom left, not top left
  y = dst->height() - (y + h);

  render(bitmap, srgb, data, kCGBlendModeCopy, 1.0,
         src_x, src_y, width(), height(), x, y, w, h);

  CGContextRelease(bitmap);
}



void Surface::blend(int src_x, int src_y, int x, int y, int w, int h, int a)
{
  CGColorSpaceRef lut;

  CGContextSaveGState(fl_gc);

  // Reset the transformation matrix back to the default identity
  // matrix as otherwise we get a massive performance hit
  CGContextConcatCTM(fl_gc, CGAffineTransformInvert(CGContextGetCTM(fl_gc)));

  // macOS Coordinates are from bottom left, not top left
  y = Fl_Window::current()->h() - (y + h);

  lut = cocoa_win_color_space(Fl_Window::current());
  render(fl_gc, lut, data, kCGBlendModeNormal, (CGFloat)a/255.0,
         src_x, src_y, width(), height(), x, y, w, h);
  CGColorSpaceRelease(lut);

  CGContextRestoreGState(fl_gc);
}

void Surface::blend(Surface* dst, int src_x, int src_y, int x, int y, int w, int h, int a)
{
  CGContextRef bitmap;

  bitmap = make_bitmap(dst->width(), dst->height(), dst->data);

  // macOS Coordinates are from bottom left, not top left
  y = dst->height() - (y + h);

  render(bitmap, srgb, data, kCGBlendModeNormal, (CGFloat)a/255.0,
         src_x, src_y, width(), height(), x, y, w, h);

  CGContextRelease(bitmap);
}

void Surface::blendWatermark(int X, int Y, int W, int H, int a)
{

  CGColorSpaceRef lut;

  CGContextSaveGState(fl_gc);

  // Reset the transformation matrix back to the default identity
  // matrix as otherwise we get a massive performance hit
  CGContextConcatCTM(fl_gc, CGAffineTransformInvert(CGContextGetCTM(fl_gc)));

  lut = cocoa_win_color_space(Fl_Window::current());

  covermap map = get_cover_map(X, Y, W, H, w, h);
  covermap::iterator it ;
  for(it = map.begin(); it != map.end(); it ++){
     blockmap block = *it;
     int src_x = std::get<0>(block);
     int src_y = std::get<1>(block);
     int dst_x = std::get<2>(block);
     int dst_y = std::get<3>(block);

     int ow = std::get<4>(block);
     int oh = std::get<5>(block);

     dst_y = Fl_Window::current()->h() - (dst_y + oh);
     render(fl_gc, lut, data, kCGBlendModeNormal, (CGFloat)a/255.0,
        src_x, src_y, ow, oh, dst_x, dst_y, ow, oh);
  }
  CGColorSpaceRelease(lut);
  CGContextRestoreGState(fl_gc);
}

void Surface::blendWatermark(Surface* dst, int X, int Y, int W, int H, int a)
{
  CGContextRef bitmap;

  bitmap = make_bitmap(dst->width(), dst->height(), dst->data);
  covermap map = get_cover_map(X, Y, W, H, w, h);
  covermap::iterator it ;
  for(it = map.begin(); it != map.end(); it ++){
     blockmap block = *it;
     int src_x = std::get<0>(block);
     int src_y = std::get<1>(block);
     int dst_x = std::get<2>(block);
     int dst_y = std::get<3>(block);

     int ow = std::get<4>(block);
     int oh = std::get<5>(block);

     dst_y = dst->height() - (dst_y + oh);

     render(bitmap, srgb, data, kCGBlendModeNormal, (CGFloat)a/255.0,
        src_x, src_y, ow, oh, dst_x, dst_y, ow, oh);
  }
  CGContextRelease(bitmap);
}

void Surface::alloc()
{
  data = new unsigned char[width() * height() * 4];
}

void Surface::dealloc()
{
  delete [] data;
}

void Surface::update(const Fl_RGB_Image* image)
{
  int x, y;
  const unsigned char* in;
  unsigned char* out;

  assert(image->w() == width());
  assert(image->h() == height());

  // Convert data and pre-multiply alpha
  in = (const unsigned char*)image->data()[0];
  out = data;
  for (y = 0;y < image->h();y++) {
    for (x = 0;x < image->w();x++) {
      switch (image->d()) {
      case 1:
        *out++ = in[0];
        *out++ = in[0];
        *out++ = in[0];
        *out++ = 0xff;
        break;
      case 2:
        *out++ = (unsigned)in[0] * in[1] / 255;
        *out++ = (unsigned)in[0] * in[1] / 255;
        *out++ = (unsigned)in[0] * in[1] / 255;
        *out++ = in[1];
        break;
      case 3:
        *out++ = in[2];
        *out++ = in[1];
        *out++ = in[0];
        *out++ = 0xff;
        break;
      case 4:
        *out++ = (unsigned)in[2] * in[3] / 255;
        *out++ = (unsigned)in[1] * in[3] / 255;
        *out++ = (unsigned)in[0] * in[3] / 255;
        *out++ = in[3];
        break;
      }
      in += image->d();
    }
    if (image->ld() != 0)
      in += image->ld() - image->w() * image->d();
  }
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
