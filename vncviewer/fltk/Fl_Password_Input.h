/* Copyright 2025 Samuel Mannehed for Cendio AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __PASSWORD_INPUT_H__
#define __PASSWORD_INPUT_H__

#include <FL/Fl_Group.H>

class Fl_Group;
class Fl_Secret_Input;
class Fl_Button;
class Fl_Box;
class Fl_Pixmap;

class Fl_Password_Input: public Fl_Group {
public:
  Fl_Password_Input(int x, int y, int w, int h, const char *label);
  ~Fl_Password_Input();

  bool password_visible() const;
  void password_visible(bool visible);
  void caps_warning_visible(bool visible);
  void toggle_password_visible();

  const char * value() const;
  int value(const char * t);

  int insert(const char *t, int l=0);
  void position(int x, int y);
  int take_focus();

  static const char* caps_warning_tooltip;

private:
  int eye_x_offset;
  int eye_y_offset;
  int warning_x_offset;
  int warning_y_offset;
  bool caps_warning_shown;

  Fl_Secret_Input * input;
  Fl_Button       * toggle_button;
  Fl_Box          * caps_warning;

  Fl_Pixmap       * open_eye_pixmap;
  Fl_Pixmap       * closed_eye_pixmap;
  Fl_Pixmap       * deactivated_eye_pixmap;
  Fl_Pixmap       * shift_lock_pixmap;

  int handle(int event) override;

  void determine_eye_visibility();
  void determine_caps_warning_visibility();

  static void text_changed_cb(Fl_Widget *, void * w);
  static void toggle_cb(Fl_Widget *, void * w);
};

#endif
