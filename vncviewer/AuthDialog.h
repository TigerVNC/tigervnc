/* Copyright 2024 Adam Halim for Cendio AB
 * Copyright 2011-2026 Pierre Ossman for Cendio AB
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

#ifndef __AUTHDIALOG_H__
#define __AUTHDIALOG_H__

#include <string>

#include <FL/Fl_Window.H>

class Fl_Check_Button;
class Fl_Input;
class Fl_Secret_Input;

class CConn;

class AuthDialog : public Fl_Window
{
public:
  AuthDialog(bool secure, bool needsUser, bool needsPassword);
  ~AuthDialog();

  int result();

  std::string getUser();
  std::string getPassword();
  bool getKeepPassword();

private:
  static void button_cb(Fl_Widget *w, long val);

private:
  Fl_Check_Button* keepPasswdCheckbox;
  Fl_Input* username;
  Fl_Secret_Input* passwd;

  int result_;
};

#endif
