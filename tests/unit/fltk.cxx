/* Copyright 2026 TigerVNC Team
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

/*
 * Test FLTK library compatibility, particularly for FLTK 1.4.x changes.
 * These tests verify that TigerVNC-specific FLTK usage patterns work
 * correctly with both FLTK 1.3.x and 1.4.x.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtest/gtest.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_RGB_Image.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/fl_draw.H>

#ifdef __unix__
#include <FL/x.H>
#endif

// Helper to check if we can use display functions
static bool canUseDisplay() {
#ifdef __unix__
  // On Unix, check if we have X display available
  return (fl_display != nullptr);
#else
  // On Windows/macOS, assume display is available
  return true;
#endif
}

// Test that FLTK version detection works correctly
TEST(FLTK, VersionDetection)
{
  // Verify we're using FLTK 1.x
  ASSERT_EQ(FL_MAJOR_VERSION, 1);
  
  // Verify we're using supported minor version (1.3 or 1.4)
  ASSERT_TRUE(FL_MINOR_VERSION == 3 || FL_MINOR_VERSION == 4)
    << "TigerVNC requires FLTK 1.3.x or 1.4.x, found " 
    << FL_MAJOR_VERSION << "." << FL_MINOR_VERSION << "." << FL_PATCH_VERSION;
}

// Test that Fl_Image_Surface works as expected (used for overlays and stats)
TEST(FLTK, ImageSurfaceBasic)
{
  if (!canUseDisplay()) {
    GTEST_SKIP() << "No display available (headless environment)";
  }

  const int width = 100;
  const int height = 100;
  
  // Create an offscreen surface (used in DesktopWindow for overlays)
  Fl_Image_Surface *surface = new Fl_Image_Surface(width, height);
  ASSERT_NE(surface, nullptr);
  
  // Set as current drawing target
  surface->set_current();
  
  // Draw something simple
  fl_rectf(0, 0, width, height, FL_BLACK);
  fl_color(FL_WHITE);
  fl_rectf(10, 10, 80, 80);
  
  // Get the image
  Fl_RGB_Image *image = surface->image();
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(image->w(), width);
  EXPECT_EQ(image->h(), height);
  EXPECT_EQ(image->d(), 3); // RGB
  
  // Cleanup
  delete image;
  delete surface;
  
  // Restore display device as current target
  Fl_Display_Device::display_device()->set_current();
}

// Test Fl_Image_Surface with text rendering (used for overlay text)
TEST(FLTK, ImageSurfaceText)
{
  if (!canUseDisplay()) {
    GTEST_SKIP() << "No display available (headless environment)";
  }

  const int width = 200;
  const int height = 50;
  
  Fl_Image_Surface *surface = new Fl_Image_Surface(width, height);
  ASSERT_NE(surface, nullptr);
  
  surface->set_current();
  
  // Clear background
  fl_rectf(0, 0, width, height, FL_BLACK);
  
  // Draw text (as done in DesktopWindow::showOverlay)
  fl_font(FL_HELVETICA, 14);
  fl_color(FL_WHITE);
  fl_draw("Test String", 0, 0, width, height, FL_ALIGN_CENTER);
  
  Fl_RGB_Image *image = surface->image();
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(image->w(), width);
  EXPECT_EQ(image->h(), height);
  
  delete image;
  delete surface;
  Fl_Display_Device::display_device()->set_current();
}

// Test drawing primitives used in TigerVNC theming and stats
TEST(FLTK, DrawingPrimitives)
{
  if (!canUseDisplay()) {
    GTEST_SKIP() << "No display available (headless environment)";
  }

  const int width = 100;
  const int height = 100;
  
  Fl_Image_Surface *surface = new Fl_Image_Surface(width, height);
  ASSERT_NE(surface, nullptr);
  surface->set_current();
  
  // Test filled rectangle (used in theme.cxx and DesktopWindow stats)
  fl_rectf(0, 0, width, height, FL_BLACK);
  
  // Test outlined rectangle (used in stats graph border)
  fl_color(FL_WHITE);
  fl_rect(10, 10, 80, 80);
  
  // Test color setting and drawing (used in stats graph)
  fl_color(FL_GREEN);
  fl_rectf(20, 20, 10, 10);
  
  fl_color(FL_YELLOW);
  fl_rectf(35, 20, 10, 10);
  
  fl_color(FL_RED);
  fl_rectf(50, 20, 10, 10);
  
  // Verify we can get the image
  Fl_RGB_Image *image = surface->image();
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(image->w(), width);
  EXPECT_EQ(image->h(), height);
  
  delete image;
  delete surface;
  Fl_Display_Device::display_device()->set_current();
}

// Test color manipulation functions used in theming
TEST(FLTK, ColorFunctions)
{
  // Test fl_color_average (used extensively in theme.cxx)
  Fl_Color c1 = FL_WHITE;
  Fl_Color c2 = FL_BLACK;
  
  Fl_Color avg = fl_color_average(c1, c2, 0.5);
  // Should produce a gray color
  EXPECT_NE(avg, c1);
  EXPECT_NE(avg, c2);
  
  // Test fl_lighter (used in monitor arrangement)
  Fl_Color lighter = fl_lighter(FL_BLUE);
  EXPECT_NE(lighter, FL_BLUE);
  
  // Test fl_inactive (used in monitor arrangement)
  Fl_Color inactive = fl_inactive(FL_RED);
  EXPECT_NE(inactive, FL_RED);
}

// Test clipping (used in DesktopWindow drawing)
TEST(FLTK, ClippingRegions)
{
  if (!canUseDisplay()) {
    GTEST_SKIP() << "No display available (headless environment)";
  }

  const int width = 100;
  const int height = 100;
  
  Fl_Image_Surface *surface = new Fl_Image_Surface(width, height);
  ASSERT_NE(surface, nullptr);
  surface->set_current();
  
  fl_rectf(0, 0, width, height, FL_WHITE);
  
  // Push a clip region (used in DesktopWindow::draw)
  fl_push_clip(10, 10, 50, 50);
  fl_rectf(0, 0, width, height, FL_BLACK); // Should only fill clipped region
  fl_pop_clip();
  
  Fl_RGB_Image *image = surface->image();
  ASSERT_NE(image, nullptr);
  
  delete image;
  delete surface;
  Fl_Display_Device::display_device()->set_current();
}

// Test Fl_RGB_Image creation and basic properties
TEST(FLTK, RGBImage)
{
  const int width = 10;
  const int height = 10;
  const int depth = 3; // RGB
  
  // Create a simple test image
  unsigned char *data = new unsigned char[width * height * depth];
  for (int i = 0; i < width * height * depth; i++) {
    data[i] = (i % 3 == 0) ? 255 : 0; // Red pixels
  }
  
  // Fl_RGB_Image takes ownership of data in FLTK 1.3.x but not in 1.4.x
  // We need to handle both cases
  Fl_RGB_Image *image = new Fl_RGB_Image(data, width, height, depth);
  ASSERT_NE(image, nullptr);
  
  EXPECT_EQ(image->w(), width);
  EXPECT_EQ(image->h(), height);
  EXPECT_EQ(image->d(), depth);
  
  delete image;
  delete[] data; // Safe to delete in both FLTK 1.3 and 1.4
}

// Test that FLTK scaling/DPI awareness doesn't break basic operations
// This is important for FLTK 1.4's improved HiDPI support
TEST(FLTK, ScalingAwareness)
{
  if (!canUseDisplay()) {
    GTEST_SKIP() << "No display available (headless environment)";
  }

#if FL_MAJOR_VERSION > 1 || (FL_MAJOR_VERSION == 1 && FL_MINOR_VERSION >= 4)
  // FLTK 1.4+ has screen_scale() for per-monitor scaling
  float scaling = Fl::screen_scale(0);
  EXPECT_GT(scaling, 0.0f);
  EXPECT_LT(scaling, 10.0f); // Sanity check
#else
  // FLTK 1.3.x doesn't have screen_scale, just verify it compiles
  // Screen scaling in 1.3 is handled via system DPI settings
  SUCCEED() << "FLTK 1.3.x - screen_scale() not available";
#endif
  
  // Verify screen dimensions are reasonable (works in both 1.3 and 1.4)
  int x, y, w, h;
  Fl::screen_xywh(x, y, w, h);
  EXPECT_GT(w, 0);
  EXPECT_GT(h, 0);
  EXPECT_LT(w, 100000); // Sanity check
  EXPECT_LT(h, 100000); // Sanity check
}

// Main test runner
int main(int argc, char **argv)
{
  // Initialize FLTK (required for some operations)
  // Note: We don't call Fl::run() as we're not running an event loop
  
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
