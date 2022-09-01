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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <list>

#include <rdr/types.h>
#include <rfb/encodings.h>

#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
#include <rfb/Security.h>
#include <rfb/SecurityClient.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif
#endif

#include "OptionsDialog.h"
#include "fltk_layout.h"
#include "i18n.h"
#include "menukey.h"
#include "parameters.h"
#include "MonitorArrangement.h"

#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Choice.H>

using namespace std;
using namespace rdr;
using namespace rfb;

std::map<OptionsCallback*, void*> OptionsDialog::callbacks;

static std::set<OptionsDialog *> instances;

OptionsDialog::OptionsDialog()
  : Fl_Window(450, 460, _("VNC Viewer: Connection Options"))
{
  int x, y;
  Fl_Button *button;

  Fl_Tabs *tabs = new Fl_Tabs(OUTER_MARGIN, OUTER_MARGIN,
                             w() - OUTER_MARGIN*2,
                             h() - OUTER_MARGIN*2 - INNER_MARGIN - BUTTON_HEIGHT);

  {
    int tx, ty, tw, th;

    tabs->client_area(tx, ty, tw, th, TABS_HEIGHT);

    createCompressionPage(tx, ty, tw, th);
    createSecurityPage(tx, ty, tw, th);
    createInputPage(tx, ty, tw, th);
    createDisplayPage(tx, ty, tw, th);
    createMiscPage(tx, ty, tw, th);
  }

  tabs->end();

  x = w() - BUTTON_WIDTH * 2 - INNER_MARGIN - OUTER_MARGIN;
  y = h() - BUTTON_HEIGHT - OUTER_MARGIN;

  button = new Fl_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("Cancel"));
  button->callback(this->handleCancel, this);

  x += BUTTON_WIDTH + INNER_MARGIN;

  button = new Fl_Return_Button(x, y, BUTTON_WIDTH, BUTTON_HEIGHT, _("OK"));
  button->callback(this->handleOK, this);

  callback(this->handleCancel, this);

  set_modal();

  if (instances.size() == 0)
    Fl::add_handler(fltk_event_handler);
  instances.insert(this);
}


OptionsDialog::~OptionsDialog()
{
  instances.erase(this);

  if (instances.size() == 0)
    Fl::remove_handler(fltk_event_handler);
}


void OptionsDialog::showDialog(void)
{
  static OptionsDialog *dialog = NULL;

  if (!dialog)
    dialog = new OptionsDialog();

  if (dialog->shown())
    return;

  dialog->show();
}


void OptionsDialog::addCallback(OptionsCallback *cb, void *data)
{
  callbacks[cb] = data;
}


void OptionsDialog::removeCallback(OptionsCallback *cb)
{
  callbacks.erase(cb);
}


void OptionsDialog::show(void)
{
  /* show() gets called for raise events as well */
  if (!shown())
    loadOptions();

  Fl_Window::show();
}


void OptionsDialog::loadOptions(void)
{
  /* Compression */
  autoselectCheckbox->value(autoSelect);

  int encNum = encodingNum(preferredEncoding);

  switch (encNum) {
  case encodingTight:
    tightButton->setonly();
    break;
  case encodingZRLE:
    zrleButton->setonly();
    break;
  case encodingHextile:
    hextileButton->setonly();
    break;
#ifdef HAVE_H264
  case encodingH264:
    h264Button->setonly();
    break;
#endif
  case encodingRaw:
    rawButton->setonly();
    break;
  }

  if (fullColour)
    fullcolorCheckbox->setonly();
  else {
    switch (lowColourLevel) {
    case 0:
      verylowcolorCheckbox->setonly();
      break;
    case 1:
      lowcolorCheckbox->setonly();
      break;
    case 2:
      mediumcolorCheckbox->setonly();
      break;
    }
  }

  char digit[2] = "0";

  compressionCheckbox->value(customCompressLevel);
  jpegCheckbox->value(!noJpeg);
  digit[0] = '0' + compressLevel;
  compressionInput->value(digit);
  digit[0] = '0' + qualityLevel;
  jpegInput->value(digit);

  handleAutoselect(autoselectCheckbox, this);
  handleCompression(compressionCheckbox, this);
  handleJpeg(jpegCheckbox, this);

#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  /* Security */
  Security security(SecurityClient::secTypes);

  list<U8> secTypes;
  list<U8>::iterator iter;

   list<U32> secTypesExt;
   list<U32>::iterator iterExt;

  encNoneCheckbox->value(false);
#ifdef HAVE_GNUTLS
  encTLSCheckbox->value(false);
  encX509Checkbox->value(false);
#endif
#ifdef HAVE_NETTLE
  encRSAAESCheckbox->value(false);
#endif

  authNoneCheckbox->value(false);
  authVncCheckbox->value(false);
  authPlainCheckbox->value(false);

  secTypes = security.GetEnabledSecTypes();
  for (iter = secTypes.begin(); iter != secTypes.end(); ++iter) {
    switch (*iter) {
    case secTypeNone:
      encNoneCheckbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case secTypeVncAuth:
      encNoneCheckbox->value(true);
      authVncCheckbox->value(true);
      break;
    }
  }

  secTypesExt = security.GetEnabledExtSecTypes();
  for (iterExt = secTypesExt.begin(); iterExt != secTypesExt.end(); ++iterExt) {
    switch (*iterExt) {
    case secTypePlain:
      encNoneCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
#ifdef HAVE_GNUTLS
    case secTypeTLSNone:
      encTLSCheckbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case secTypeTLSVnc:
      encTLSCheckbox->value(true);
      authVncCheckbox->value(true);
      break;
    case secTypeTLSPlain:
      encTLSCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
    case secTypeX509None:
      encX509Checkbox->value(true);
      authNoneCheckbox->value(true);
      break;
    case secTypeX509Vnc:
      encX509Checkbox->value(true);
      authVncCheckbox->value(true);
      break;
    case secTypeX509Plain:
      encX509Checkbox->value(true);
      authPlainCheckbox->value(true);
      break;
#endif
#ifdef HAVE_NETTLE
    case secTypeRA2:
    case secTypeRA256:
      encRSAAESCheckbox->value(true);
    case secTypeRA2ne:
    case secTypeRAne256:
      authVncCheckbox->value(true);
      authPlainCheckbox->value(true);
      break;
#endif
    
    }
  }

#ifdef HAVE_GNUTLS
  caInput->value(CSecurityTLS::X509CA);
  crlInput->value(CSecurityTLS::X509CRL);

  handleX509(encX509Checkbox, this);
#endif
#endif

  /* Input */
  const char *menuKeyBuf;

  viewOnlyCheckbox->value(viewOnly);
  emulateMBCheckbox->value(emulateMiddleButton);
  acceptClipboardCheckbox->value(acceptClipboard);
#if !defined(WIN32) && !defined(__APPLE__)
  setPrimaryCheckbox->value(setPrimary);
#endif
  sendClipboardCheckbox->value(sendClipboard);
#if !defined(WIN32) && !defined(__APPLE__)
  sendPrimaryCheckbox->value(sendPrimary);
#endif
  systemKeysCheckbox->value(fullscreenSystemKeys);

  menuKeyChoice->value(0);

  menuKeyBuf = menuKey;
  for (int i = 0; i < getMenuKeySymbolCount(); i++)
    if (!strcmp(getMenuKeySymbols()[i].name, menuKeyBuf))
      menuKeyChoice->value(i + 1);

  /* Display */
  if (!fullScreen) {
    windowedButton->setonly();
  } else {
    if (!strcasecmp(fullScreenMode, "all")) {
      allMonitorsButton->setonly();
    } else if (!strcasecmp(fullScreenMode, "selected")) {
      selectedMonitorsButton->setonly();
    } else {
      currentMonitorButton->setonly();
    }
  }

  monitorArrangement->set(fullScreenSelectedMonitors.getParam());

  handleFullScreenMode(selectedMonitorsButton, this);

  /* Misc. */
  sharedCheckbox->value(shared);
  reconnectCheckbox->value(reconnectOnError);
  dotCursorCheckbox->value(dotWhenNoCursor);
}


void OptionsDialog::storeOptions(void)
{
  /* Compression */
  autoSelect.setParam(autoselectCheckbox->value());

  if (tightButton->value())
    preferredEncoding.setParam(encodingName(encodingTight));
  else if (zrleButton->value())
    preferredEncoding.setParam(encodingName(encodingZRLE));
  else if (hextileButton->value())
    preferredEncoding.setParam(encodingName(encodingHextile));
#ifdef HAVE_H264
  else if (h264Button->value())
    preferredEncoding.setParam(encodingName(encodingH264));
#endif
  else if (rawButton->value())
    preferredEncoding.setParam(encodingName(encodingRaw));

  fullColour.setParam(fullcolorCheckbox->value());
  if (verylowcolorCheckbox->value())
    lowColourLevel.setParam(0);
  else if (lowcolorCheckbox->value())
    lowColourLevel.setParam(1);
  else if (mediumcolorCheckbox->value())
    lowColourLevel.setParam(2);

  customCompressLevel.setParam(compressionCheckbox->value());
  noJpeg.setParam(!jpegCheckbox->value());
  compressLevel.setParam(atoi(compressionInput->value()));
  qualityLevel.setParam(atoi(jpegInput->value()));

#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  /* Security */
  Security security;

  /* Process security types which don't use encryption */
  if (encNoneCheckbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(secTypeNone);
    if (authVncCheckbox->value()) {
      security.EnableSecType(secTypeVncAuth);
#ifdef HAVE_NETTLE
      security.EnableSecType(secTypeRAne256);
#endif
    }
    if (authPlainCheckbox->value()) {
      security.EnableSecType(secTypePlain);
#ifdef HAVE_NETTLE
      security.EnableSecType(secTypeRA2ne);
      security.EnableSecType(secTypeRAne256);
#endif
    }
  }

#ifdef HAVE_GNUTLS
  /* Process security types which use TLS encryption */
  if (encTLSCheckbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(secTypeTLSNone);
    if (authVncCheckbox->value())
      security.EnableSecType(secTypeTLSVnc);
    if (authPlainCheckbox->value())
      security.EnableSecType(secTypeTLSPlain);
  }

  /* Process security types which use X509 encryption */
  if (encX509Checkbox->value()) {
    if (authNoneCheckbox->value())
      security.EnableSecType(secTypeX509None);
    if (authVncCheckbox->value())
      security.EnableSecType(secTypeX509Vnc);
    if (authPlainCheckbox->value())
      security.EnableSecType(secTypeX509Plain);
  }

  CSecurityTLS::X509CA.setParam(caInput->value());
  CSecurityTLS::X509CRL.setParam(crlInput->value());
#endif

#ifdef HAVE_NETTLE
  if (encRSAAESCheckbox->value()) {
    security.EnableSecType(secTypeRA2);
    security.EnableSecType(secTypeRA256);
  }
#endif
  SecurityClient::secTypes.setParam(security.ToString());
#endif

  /* Input */
  viewOnly.setParam(viewOnlyCheckbox->value());
  emulateMiddleButton.setParam(emulateMBCheckbox->value());
  acceptClipboard.setParam(acceptClipboardCheckbox->value());
#if !defined(WIN32) && !defined(__APPLE__)
  setPrimary.setParam(setPrimaryCheckbox->value());
#endif
  sendClipboard.setParam(sendClipboardCheckbox->value());
#if !defined(WIN32) && !defined(__APPLE__)
  sendPrimary.setParam(sendPrimaryCheckbox->value());
#endif
  fullscreenSystemKeys.setParam(systemKeysCheckbox->value());

  if (menuKeyChoice->value() == 0)
    menuKey.setParam("");
  else {
    menuKey.setParam(menuKeyChoice->text());
  }

  /* Display */
  if (windowedButton->value()) {
    fullScreen.setParam(false);
  } else {
    fullScreen.setParam(true);

    if (allMonitorsButton->value()) {
      fullScreenMode.setParam("All");
    } else if (selectedMonitorsButton->value()) {
      fullScreenMode.setParam("Selected");
    } else {
      fullScreenMode.setParam("Current");
    }
  }

  fullScreenSelectedMonitors.setParam(monitorArrangement->get());

  /* Misc. */
  shared.setParam(sharedCheckbox->value());
  reconnectOnError.setParam(reconnectCheckbox->value());
  dotWhenNoCursor.setParam(dotCursorCheckbox->value());

  std::map<OptionsCallback*, void*>::const_iterator iter;

  for (iter = callbacks.begin();iter != callbacks.end();++iter)
    iter->first(iter->second);
}


void OptionsDialog::createCompressionPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Compression"));

  int orig_tx, orig_ty;
  int col1_ty, col2_ty;
  int half_width, full_width;
  int height;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  full_width = tw - OUTER_MARGIN * 2;
  half_width = (full_width - INNER_MARGIN) / 2;

  /* AutoSelect checkbox */
  autoselectCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Auto select")));
  autoselectCheckbox->callback(handleAutoselect, this);
  ty += CHECK_HEIGHT + INNER_MARGIN;

  /* Two columns */
  orig_tx = tx;
  orig_ty = ty;

  /* VNC encoding box */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 3 + RADIO_HEIGHT * 4;
#ifdef HAVE_H264
  height += TIGHT_MARGIN + RADIO_HEIGHT;
#endif
  encodingGroup = new Fl_Group(tx, ty, half_width, height,
                                _("Preferred encoding"));
  encodingGroup->box(FL_ENGRAVED_BOX);
  encodingGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    tightButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                               RADIO_MIN_WIDTH,
                                               RADIO_HEIGHT,
                                               "Tight"));
    tightButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    zrleButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                              RADIO_MIN_WIDTH,
                                              RADIO_HEIGHT,
                                              "ZRLE"));
    zrleButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    hextileButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                 RADIO_MIN_WIDTH,
                                                 RADIO_HEIGHT,
                                                 "Hextile"));
    hextileButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

#ifdef HAVE_H264
    h264Button = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                             RADIO_MIN_WIDTH,
                                             RADIO_HEIGHT,
                                             "H.264"));
    h264Button->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
#endif

    rawButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                             RADIO_MIN_WIDTH,
                                             RADIO_HEIGHT,
                                             "Raw"));
    rawButton->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  encodingGroup->end();
  col1_ty = ty;

  /* Second column */
  tx = orig_tx + half_width + INNER_MARGIN;
  ty = orig_ty;

  /* Color box */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 3 + RADIO_HEIGHT * 4;
  colorlevelGroup = new Fl_Group(tx, ty, half_width, height, _("Color level"));
  colorlevelGroup->box(FL_ENGRAVED_BOX);
  colorlevelGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    fullcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                     RADIO_MIN_WIDTH,
                                                     RADIO_HEIGHT,
                                                     _("Full")));
    fullcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    mediumcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                       RADIO_MIN_WIDTH,
                                                       RADIO_HEIGHT,
                                                       _("Medium")));
    mediumcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    lowcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                    RADIO_MIN_WIDTH,
                                                    RADIO_HEIGHT,
                                                    _("Low")));
    lowcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    verylowcolorCheckbox = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                        RADIO_MIN_WIDTH,
                                                        RADIO_HEIGHT,
                                                        _("Very low")));
    verylowcolorCheckbox->type(FL_RADIO_BUTTON);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  colorlevelGroup->end();
  col2_ty = ty;

  /* Back to normal */
  tx = orig_tx;
  ty = (col1_ty > col2_ty ? col1_ty : col2_ty) + INNER_MARGIN;

  /* Checkboxes */
  compressionCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Custom compression level:")));
  compressionCheckbox->callback(handleCompression, this);
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  compressionInput = new Fl_Int_Input(tx + INDENT, ty,
                                      INPUT_HEIGHT, INPUT_HEIGHT,
                                      _("level (0=fast, 9=best)"));
  compressionInput->align(FL_ALIGN_RIGHT);
  ty += INPUT_HEIGHT + INNER_MARGIN;

  jpegCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                              CHECK_MIN_WIDTH,
                                              CHECK_HEIGHT,
                                              _("Allow JPEG compression:")));
  jpegCheckbox->callback(handleJpeg, this);
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  jpegInput = new Fl_Int_Input(tx + INDENT, ty,
                               INPUT_HEIGHT, INPUT_HEIGHT,
                               _("quality (0=poor, 9=best)"));
  jpegInput->align(FL_ALIGN_RIGHT);
  ty += INPUT_HEIGHT + INNER_MARGIN;

  group->end();
}


void OptionsDialog::createSecurityPage(int tx, int ty, int tw, int th)
{
#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Security"));

  int orig_tx;
  int width, height;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  orig_tx = tx;

  /* Encryption */
  ty += GROUP_LABEL_OFFSET;

#if defined(HAVE_GNUTLS) && defined(HAVE_NETTLE)
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 5 + CHECK_HEIGHT * 4 + (INPUT_LABEL_OFFSET + INPUT_HEIGHT) * 2;
#elif defined(HAVE_GNUTLS)
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 4 + CHECK_HEIGHT * 3 + (INPUT_LABEL_OFFSET + INPUT_HEIGHT) * 2;
#elif defined(HAVE_NETTLE)
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 1 + CHECK_HEIGHT * 2;
#endif
  encryptionGroup = new Fl_Group(tx, ty, width, height, _("Encryption"));
  encryptionGroup->box(FL_ENGRAVED_BOX);
  encryptionGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    encNoneCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("None")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

#ifdef HAVE_GNUTLS
    encTLSCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("TLS with anonymous certificates")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    encX509Checkbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("TLS with X509 certificates")));
    encX509Checkbox->callback(handleX509, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    ty += INPUT_LABEL_OFFSET;
    caInput = new Fl_Input(tx + INDENT, ty, 
                           width - GROUP_MARGIN*2 - INDENT, INPUT_HEIGHT,
                           _("Path to X509 CA certificate"));
    caInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;

    ty += INPUT_LABEL_OFFSET;
    crlInput = new Fl_Input(tx + INDENT, ty,
                            width - GROUP_MARGIN*2 - INDENT, INPUT_HEIGHT,
                            _("Path to X509 CRL file"));
    crlInput->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
    ty += INPUT_HEIGHT + TIGHT_MARGIN;
#endif
#ifdef HAVE_NETTLE
    encRSAAESCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("RSA-AES")));
    encRSAAESCheckbox->callback(handleRSAAES, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
#endif
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  encryptionGroup->end();

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Authentication */
  ty += GROUP_LABEL_OFFSET;
  height = GROUP_MARGIN * 2 + TIGHT_MARGIN * 2 + CHECK_HEIGHT * 3;
  authenticationGroup = new Fl_Group(tx, ty, width, height, _("Authentication"));
  authenticationGroup->box(FL_ENGRAVED_BOX);
  authenticationGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    authNoneCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                    CHECK_MIN_WIDTH,
                                                    CHECK_HEIGHT,
                                                    _("None")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    authVncCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                   CHECK_MIN_WIDTH,
                                                   CHECK_HEIGHT,
                                                   _("Standard VNC (insecure without encryption)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    authPlainCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Username and password (insecure without encryption)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
  }

  ty += GROUP_MARGIN - TIGHT_MARGIN;

  authenticationGroup->end();

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  group->end();
#endif
}


void OptionsDialog::createInputPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Input"));

  int orig_tx;
  int width;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  viewOnlyCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("View only (ignore mouse and keyboard)")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  orig_tx = tx;

  /* Mouse */
  ty += GROUP_LABEL_OFFSET;
  mouseGroup = new Fl_Group(tx, ty, width, 0, _("Mouse"));
  mouseGroup->box(FL_ENGRAVED_BOX);
  mouseGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  /* Needed for final resize to work sanely */
  mouseGroup->resizable(NULL);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    emulateMBCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                     CHECK_MIN_WIDTH,
                                                     CHECK_HEIGHT,
                                                     _("Emulate middle mouse button")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    dotCursorCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                    CHECK_MIN_WIDTH,
                                                    CHECK_HEIGHT,
                                                    _("Show dot when no cursor")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
  }
  ty += GROUP_MARGIN - TIGHT_MARGIN;

  mouseGroup->end();
  mouseGroup->size(mouseGroup->w(), ty - mouseGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Keyboard */
  ty += GROUP_LABEL_OFFSET;
  keyboardGroup = new Fl_Group(tx, ty, width, 0, _("Keyboard"));
  keyboardGroup->box(FL_ENGRAVED_BOX);
  keyboardGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  /* Needed for final resize to work sanely */
  keyboardGroup->resizable(NULL);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    systemKeysCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                      CHECK_MIN_WIDTH,
                                                      CHECK_HEIGHT,
                                                      _("Pass system keys directly to server (full screen)")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

    menuKeyChoice = new Fl_Choice(LBLLEFT(tx, ty, 150, CHOICE_HEIGHT, _("Menu key")));

    fltk_menu_add(menuKeyChoice, _("None"), 0, NULL, (void*)0, FL_MENU_DIVIDER);
    for (int i = 0; i < getMenuKeySymbolCount(); i++)
      fltk_menu_add(menuKeyChoice, getMenuKeySymbols()[i].name, 0, NULL, 0, 0);

    ty += CHOICE_HEIGHT + TIGHT_MARGIN;
  }
  ty += GROUP_MARGIN - TIGHT_MARGIN;

  keyboardGroup->end();
  keyboardGroup->size(keyboardGroup->w(), ty - keyboardGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  /* Clipboard */
  ty += GROUP_LABEL_OFFSET;
  clipboardGroup = new Fl_Group(tx, ty, width, 0, _("Clipboard"));
  clipboardGroup->box(FL_ENGRAVED_BOX);
  clipboardGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  /* Needed for final resize to work sanely */
  clipboardGroup->resizable(NULL);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;

    acceptClipboardCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                           CHECK_MIN_WIDTH,
                                                           CHECK_HEIGHT,
                                                           _("Accept clipboard from server")));
    acceptClipboardCheckbox->callback(handleClipboard, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

#if !defined(WIN32) && !defined(__APPLE__)
    setPrimaryCheckbox = new Fl_Check_Button(LBLRIGHT(tx + INDENT, ty,
                                                      CHECK_MIN_WIDTH,
                                                      CHECK_HEIGHT,
                                                      _("Also set primary selection")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
#endif

    sendClipboardCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                         CHECK_MIN_WIDTH,
                                                         CHECK_HEIGHT,
                                                         _("Send clipboard to server")));
    sendClipboardCheckbox->callback(handleClipboard, this);
    ty += CHECK_HEIGHT + TIGHT_MARGIN;

#if !defined(WIN32) && !defined(__APPLE__)
    sendPrimaryCheckbox = new Fl_Check_Button(LBLRIGHT(tx + INDENT, ty,
                                                       CHECK_MIN_WIDTH,
                                                       CHECK_HEIGHT,
                                                       _("Send primary selection as clipboard")));
    ty += CHECK_HEIGHT + TIGHT_MARGIN;
#endif
  }
  ty += GROUP_MARGIN - TIGHT_MARGIN;

  clipboardGroup->end();
  clipboardGroup->size(clipboardGroup->w(), ty - clipboardGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;

  group->end();
}


void OptionsDialog::createDisplayPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Display"));

  int orig_tx;
  int width;

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  width = tw - OUTER_MARGIN * 2;

  orig_tx = tx;

  /* Display mode */
  ty += GROUP_LABEL_OFFSET;
  displayModeGroup = new Fl_Group(tx, ty, width, 0, _("Display mode"));
  displayModeGroup->box(FL_ENGRAVED_BOX);
  displayModeGroup->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);

  /* Needed for final resize to work sanely */
  displayModeGroup->resizable(NULL);

  {
    tx += GROUP_MARGIN;
    ty += GROUP_MARGIN;
    width -= GROUP_MARGIN * 2;

    windowedButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                  RADIO_MIN_WIDTH,
                                                  RADIO_HEIGHT,
                                                  _("Windowed")));
    windowedButton->type(FL_RADIO_BUTTON);
    windowedButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    currentMonitorButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                        RADIO_MIN_WIDTH,
                                                        RADIO_HEIGHT,
                                                        _("Full screen on current monitor")));
    currentMonitorButton->type(FL_RADIO_BUTTON);
    currentMonitorButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    allMonitorsButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                            RADIO_MIN_WIDTH,
                                            RADIO_HEIGHT,
                                            _("Full screen on all monitors")));
    allMonitorsButton->type(FL_RADIO_BUTTON);
    allMonitorsButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    selectedMonitorsButton = new Fl_Round_Button(LBLRIGHT(tx, ty,
                                                 RADIO_MIN_WIDTH,
                                                 RADIO_HEIGHT,
                                                 _("Full screen on selected monitor(s)")));
    selectedMonitorsButton->type(FL_RADIO_BUTTON);
    selectedMonitorsButton->callback(handleFullScreenMode, this);
    ty += RADIO_HEIGHT + TIGHT_MARGIN;

    monitorArrangement = new MonitorArrangement(
                              tx + INDENT, ty,
                              width - INDENT, 150);
    ty += 150 + TIGHT_MARGIN;
  }
  ty += GROUP_MARGIN - TIGHT_MARGIN;

  displayModeGroup->end();
  displayModeGroup->size(displayModeGroup->w(),
                         ty - displayModeGroup->y());

  /* Back to normal */
  tx = orig_tx;
  ty += INNER_MARGIN;
  width = tw - OUTER_MARGIN * 2;

  group->end();
}


void OptionsDialog::createMiscPage(int tx, int ty, int tw, int th)
{
  Fl_Group *group = new Fl_Group(tx, ty, tw, th, _("Misc."));

  tx += OUTER_MARGIN;
  ty += OUTER_MARGIN;

  sharedCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Shared (don't disconnect other viewers)")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  reconnectCheckbox = new Fl_Check_Button(LBLRIGHT(tx, ty,
                                                  CHECK_MIN_WIDTH,
                                                  CHECK_HEIGHT,
                                                  _("Ask to reconnect on connection errors")));
  ty += CHECK_HEIGHT + TIGHT_MARGIN;

  group->end();
}


void OptionsDialog::handleAutoselect(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->autoselectCheckbox->value()) {
    dialog->encodingGroup->deactivate();
    dialog->colorlevelGroup->deactivate();
  } else {
    dialog->encodingGroup->activate();
    dialog->colorlevelGroup->activate();
  }

  // JPEG setting is also affected by autoselection
  dialog->handleJpeg(dialog->jpegCheckbox, dialog);
}


void OptionsDialog::handleCompression(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->compressionCheckbox->value())
    dialog->compressionInput->activate();
  else
    dialog->compressionInput->deactivate();
}


void OptionsDialog::handleJpeg(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->jpegCheckbox->value() &&
      !dialog->autoselectCheckbox->value())
    dialog->jpegInput->activate();
  else
    dialog->jpegInput->deactivate();
}


void OptionsDialog::handleX509(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->encX509Checkbox->value()) {
    dialog->caInput->activate();
    dialog->crlInput->activate();
  } else {
    dialog->caInput->deactivate();
    dialog->crlInput->deactivate();
  }
}


void OptionsDialog::handleRSAAES(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->encRSAAESCheckbox->value()) {
    dialog->authVncCheckbox->value(true);
    dialog->authPlainCheckbox->value(true);
  }
}


void OptionsDialog::handleClipboard(Fl_Widget *widget, void *data)
{
#if !defined(WIN32) && !defined(__APPLE__)
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->acceptClipboardCheckbox->value())
    dialog->setPrimaryCheckbox->activate();
  else
    dialog->setPrimaryCheckbox->deactivate();
  if (dialog->sendClipboardCheckbox->value())
    dialog->sendPrimaryCheckbox->activate();
  else
    dialog->sendPrimaryCheckbox->deactivate();
#endif
}

void OptionsDialog::handleFullScreenMode(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  if (dialog->selectedMonitorsButton->value()) {
    dialog->monitorArrangement->activate();
  } else {
    dialog->monitorArrangement->deactivate();
  }
}

void OptionsDialog::handleCancel(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();
}


void OptionsDialog::handleOK(Fl_Widget *widget, void *data)
{
  OptionsDialog *dialog = (OptionsDialog*)data;

  dialog->hide();

  dialog->storeOptions();
}

int OptionsDialog::fltk_event_handler(int event)
{
  std::set<OptionsDialog *>::iterator iter;

  if (event != FL_SCREEN_CONFIGURATION_CHANGED)
    return 0;

  // Refresh monitor arrangement widget to match the parameter settings after
  // screen configuration has changed. The MonitorArrangement index doesn't work
  // the same way as the FLTK screen index.
  for (iter = instances.begin(); iter != instances.end(); iter++)
      Fl::add_timeout(0, handleScreenConfigTimeout, (*iter));

  return 0;
}

void OptionsDialog::handleScreenConfigTimeout(void *data)
{
    OptionsDialog *self = (OptionsDialog *)data;

    assert(self);

    self->monitorArrangement->set(fullScreenSelectedMonitors.getParam());
}
