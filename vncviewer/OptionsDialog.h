/* Copyright 2011-2021 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

#include <map>

#include <FL/Fl_Window.H>

class Fl_Widget;
class Fl_Group;
class Fl_Check_Button;
class Fl_Round_Button;
class Fl_Input;
class Fl_Int_Input;
class Fl_Choice;
class MonitorArrangement;

typedef void (OptionsCallback)(void*);

class OptionsDialog : public Fl_Window {
protected:
  OptionsDialog();
  ~OptionsDialog();

public:
  static void showDialog(void);

  static void addCallback(OptionsCallback *cb, void *data = NULL);
  static void removeCallback(OptionsCallback *cb);

  void show(void);

protected:
  void loadOptions(void);
  void storeOptions(void);

  void createCompressionPage(int tx, int ty, int tw, int th);
  void createSecurityPage(int tx, int ty, int tw, int th);
  void createInputPage(int tx, int ty, int tw, int th);
  void createDisplayPage(int tx, int ty, int tw, int th);
  void createMiscPage(int tx, int ty, int tw, int th);

  static void handleAutoselect(Fl_Widget *widget, void *data);
  static void handleCompression(Fl_Widget *widget, void *data);
  static void handleJpeg(Fl_Widget *widget, void *data);

  static void handleX509(Fl_Widget *widget, void *data);
  static void handleRSAAES(Fl_Widget *widget, void *data);

  static void handleClipboard(Fl_Widget *widget, void *data);

  static void handleFullScreenMode(Fl_Widget *widget, void *data);

  static void handleCancel(Fl_Widget *widget, void *data);
  static void handleOK(Fl_Widget *widget, void *data);

protected:
  static std::map<OptionsCallback*, void*> callbacks;

  /* Compression */
  Fl_Check_Button *autoselectCheckbox;

  Fl_Group *encodingGroup;
  Fl_Round_Button *tightButton;
  Fl_Round_Button *zrleButton;
  Fl_Round_Button *hextileButton;
#ifdef HAVE_H264
  Fl_Round_Button *h264Button;
#endif
  Fl_Round_Button *rawButton;

  Fl_Group *colorlevelGroup;
  Fl_Round_Button *fullcolorCheckbox;
  Fl_Round_Button *mediumcolorCheckbox;
  Fl_Round_Button *lowcolorCheckbox;
  Fl_Round_Button *verylowcolorCheckbox;

  Fl_Check_Button *compressionCheckbox;
  Fl_Check_Button *jpegCheckbox;
  Fl_Int_Input *compressionInput;
  Fl_Int_Input *jpegInput;

  /* Security */
  Fl_Group *encryptionGroup;
  Fl_Check_Button *encNoneCheckbox;
  Fl_Check_Button *encTLSCheckbox;
  Fl_Check_Button *encX509Checkbox;
  Fl_Check_Button *encRSAAESCheckbox;
  Fl_Input *caInput;
  Fl_Input *crlInput;

  Fl_Group *authenticationGroup;
  Fl_Check_Button *authNoneCheckbox;
  Fl_Check_Button *authVncCheckbox;
  Fl_Check_Button *authPlainCheckbox;

  /* Input */
  Fl_Check_Button *viewOnlyCheckbox;
  Fl_Group *mouseGroup;
  Fl_Check_Button *emulateMBCheckbox;
  Fl_Check_Button *dotCursorCheckbox;
  Fl_Group *keyboardGroup;
  Fl_Check_Button *systemKeysCheckbox;
  Fl_Choice *menuKeyChoice;
  Fl_Group *clipboardGroup;
  Fl_Check_Button *acceptClipboardCheckbox;
#if !defined(WIN32) && !defined(__APPLE__)
  Fl_Check_Button *setPrimaryCheckbox;
#endif
  Fl_Check_Button *sendClipboardCheckbox;
#if !defined(WIN32) && !defined(__APPLE__)
  Fl_Check_Button *sendPrimaryCheckbox;
#endif

  /* Display */
  Fl_Group *displayModeGroup;
  Fl_Round_Button *windowedButton;
  Fl_Round_Button *currentMonitorButton;
  Fl_Round_Button *allMonitorsButton;
  Fl_Round_Button *selectedMonitorsButton;
  MonitorArrangement *monitorArrangement;

  /* Misc. */
  Fl_Check_Button *sharedCheckbox;
  Fl_Check_Button *reconnectCheckbox;

private:
  static int fltk_event_handler(int event);
  static void handleScreenConfigTimeout(void *data);
};

#endif
