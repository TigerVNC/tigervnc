/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
//
// OptionsDialog.h
//

#ifndef __OPTIONSDIALOG_H__
#define __OPTIONSDIALOG_H__

#include "TXDialog.h"
#include "TXLabel.h"
#include "TXEntry.h"
#include "TXButton.h"
#include "TXCheckbox.h"
#include "parameters.h"

#define SECOND_COL_XPAD 350

class OptionsDialogCallback {
public:
  virtual void setOptions() = 0;
  virtual void getOptions() = 0;
};

class OptionsDialog : public TXDialog, public TXButtonCallback,
                      public TXCheckboxCallback, public TXEntryCallback  {
public:
  OptionsDialog(Display* dpy, OptionsDialogCallback* cb_)
    : TXDialog(dpy, 450, 450, _("VNC Viewer: Connection Options")), cb(cb_),

      /* Encoding and color level */
      formatAndEnc(dpy, _("Encoding and Color Level:"), this),
      inputs(dpy, _("Inputs:"), this),
      misc(dpy, _("Misc:"), this),
      autoSelect(dpy, _("Auto select"), this, false, this),
      fullColour(dpy, _("Full (all available colors)"), this, true, this),
      mediumColour(dpy, _("Medium (256 colors)"), this, true, this),
      lowColour(dpy, _("Low (64 colors)"), this, true, this),
      veryLowColour(dpy, _("Very low (8 colors)"), this, true, this),
      tight(dpy, "Tight", this, true, this),
      zrle(dpy, "ZRLE", this, true, this),
      hextile(dpy, "Hextile", this, true, this),
      raw(dpy, "Raw", this, true, this),

      /* Compression */
      customCompressLevel(dpy, _("Custom compression level:"), this, false, this),
      compressLevel(dpy, this, this, false, 30),
      compressLevelLabel(dpy, _("level (1=fast, 9=best)"), this),
      noJpeg(dpy, _("Allow JPEG compression:"), this, false, this),
      qualityLevel(dpy, this, this, false, 30),
      qualityLevelLabel(dpy, _("quality (1=poor, 9=best)"), this),

      /* Inputs */
      viewOnly(dpy, _("View only (ignore mouse & keyboard)"), this, false, this),
      acceptClipboard(dpy, _("Accept clipboard from server"), this, false, this),
      sendClipboard(dpy, _("Send clipboard to server"), this, false, this),
      sendPrimary(dpy, _("Send primary selection & cut buffer as clipboard"),
                  this, false, this),

      /* Misc */
      shared(dpy, _("Shared (don't disconnect other viewers)"), this, false,this),
      fullScreen(dpy, _("Full-screen mode"), this, false, this),
      useLocalCursor(dpy, _("Render cursor locally"), this, false, this),
      dotWhenNoCursor(dpy, _("Show dot when no cursor"), this, false, this),
      okButton(dpy, _("OK"), this, this, 60),
      cancelButton(dpy, _("Cancel"), this, this, 60)

#ifdef HAVE_GNUTLS
      ,
      /* Security */
      security(dpy, _("Security:"), this),
      secVeNCrypt(dpy, _("Extended encryption and authentication methods (VeNCrypt)"),
		  this, false, this),

      /* Encryption */
      encryption(dpy, _("Session encryption:"), this),
      encNone(dpy, _("None"), this, false, this),
      encTLS(dpy, _("TLS with anonymous certificates"), this, false, this),
      encX509(dpy, _("TLS with X509 certificates"), this, false, this),
      cacert(dpy, _("Path to X509 CA certificate"), this),
      ca(dpy, this, this, false, 350),
      crlcert(dpy, _("Path to X509 CRL file"), this),
      crl(dpy, this, this, false, 350),

      /* Authentication */
      authentication(dpy, _("Authentication:"), this),
      secNone(dpy, _("None"), this, false, this),
      secVnc(dpy, _("Standard VNC (insecure without encryption)"),
	     this, false, this),
      secPlain(dpy, _("Username and password (insecure without encryption)"),
	       this, false, this)
#endif
  {
    /* Render the first collumn */
    int y = yPad;
    formatAndEnc.move(xPad, y);
    y += formatAndEnc.height();
    autoSelect.move(xPad, y);
    int x2 = xPad + autoSelect.width() + xPad*5;
    y += autoSelect.height();
    tight.move(xPad, y);
    fullColour.move(x2, y);
    y += tight.height();
    zrle.move(xPad, y);
    mediumColour.move(x2, y);
    y += zrle.height();
    hextile.move(xPad, y);
    lowColour.move(x2, y);
    y += hextile.height();
    raw.move(xPad, y);
    veryLowColour.move(x2, y);
    y += raw.height() + yPad*2;

    customCompressLevel.move(xPad, y);
    y += customCompressLevel.height();
    compressLevel.move(xPad*10, y);
    compressLevelLabel.move(xPad*20, y);
    y += compressLevel.height();

    noJpeg.move(xPad, y);
    y += noJpeg.height();
    qualityLevel.move(xPad*10, y);
    qualityLevelLabel.move(xPad*20, y);
    y += qualityLevel.height();

    y += yPad*4;
    inputs.move(xPad, y);
    y += inputs.height();
    viewOnly.move(xPad, y);
    y += viewOnly.height();
    acceptClipboard.move(xPad, y);
    y += acceptClipboard.height();
    sendClipboard.move(xPad, y);
    y += sendClipboard.height();
    sendPrimary.move(xPad, y);
    y += sendPrimary.height();

    y += yPad*4;
    misc.move(xPad, y);
    y += misc.height();
    shared.move(xPad, y);
    y += shared.height();
    fullScreen.move(xPad, y);
    y += fullScreen.height();
    useLocalCursor.move(xPad, y);
    y += useLocalCursor.height();
    dotWhenNoCursor.move(xPad, y);
    y += dotWhenNoCursor.height();

#ifdef HAVE_GNUTLS
    /* Render the second collumn */
    y = yPad;
    xPad += SECOND_COL_XPAD;
    resize(750, height());

    security.move(xPad, y);
    y += security.height();
    secVeNCrypt.move(xPad, y);
    y += secVeNCrypt.height();

    encryption.move(xPad, y);
    y += encryption.height();
    encNone.move(xPad, y);
    y += encNone.height();
    encTLS.move(xPad, y);
    y += encTLS.height();
    encX509.move(xPad, y);
    y += encX509.height();
    cacert.move(xPad, y);
    y += cacert.height();
    ca.move(xPad, y);
    y += ca.height();
    crlcert.move(xPad, y);
    y += crlcert.height();
    crl.move(xPad, y);
    y += crl.height();

    authentication.move(xPad, y);
    y += authentication.height();
    secNone.move(xPad, y);
    y += secNone.height();
    secVnc.move(xPad, y);
    y += secVnc.height();
    secPlain.move(xPad, y);
    y += secPlain.height();

    xPad -= SECOND_COL_XPAD;
#endif

    /* Render "OK" and "Cancel" buttons */
    okButton.move(width() - xPad*12 - cancelButton.width() - okButton.width(),
                  height() - yPad*4 - okButton.height());
    cancelButton.move(width() - xPad*6 - cancelButton.width(),
                      height() - yPad*4 - cancelButton.height());

    setBorderWidth(1);
  }

  virtual void initDialog() {
    if (cb) cb->setOptions();
    tight.disabled(autoSelect.checked());
    zrle.disabled(autoSelect.checked());
    hextile.disabled(autoSelect.checked());
    raw.disabled(autoSelect.checked());
    fullColour.disabled(autoSelect.checked());
    mediumColour.disabled(autoSelect.checked());
    lowColour.disabled(autoSelect.checked());
    veryLowColour.disabled(autoSelect.checked());
    sendPrimary.disabled(!sendClipboard.checked());
    dotWhenNoCursor.disabled(!useLocalCursor.checked());
    compressLevel.disabled(!customCompressLevel.checked());
    qualityLevel.disabled(autoSelect.checked() || !noJpeg.checked());
  }

  virtual void takeFocus(Time time) {
    //XSetInputFocus(dpy, entry.win, RevertToParent, time);
  }

  virtual void buttonActivate(TXButton* b) {
    if (b == &okButton) {
      if (cb) cb->getOptions();
      unmap();
    } else if (b == &cancelButton) {
      unmap();
    }
  }

  virtual void checkboxSelect(TXCheckbox* checkbox) {
    if (checkbox == &autoSelect) {
      tight.disabled(autoSelect.checked());
      zrle.disabled(autoSelect.checked());
      hextile.disabled(autoSelect.checked());
      raw.disabled(autoSelect.checked());
      fullColour.disabled(autoSelect.checked());
      mediumColour.disabled(autoSelect.checked());
      lowColour.disabled(autoSelect.checked());
      veryLowColour.disabled(autoSelect.checked());
      qualityLevel.disabled(autoSelect.checked() || !noJpeg.checked());
    } else if (checkbox == &fullColour || checkbox == &mediumColour ||
               checkbox == &lowColour || checkbox == &veryLowColour) {
      fullColour.checked(checkbox == &fullColour);
      mediumColour.checked(checkbox == &mediumColour);
      lowColour.checked(checkbox == &lowColour);
      veryLowColour.checked(checkbox == &veryLowColour);
    } else if (checkbox == &tight || checkbox == &zrle || checkbox == &hextile || checkbox == &raw) {
      tight.checked(checkbox == &tight);
      zrle.checked(checkbox == &zrle);
      hextile.checked(checkbox == &hextile);
      raw.checked(checkbox == &raw);
    } else if (checkbox == &sendClipboard) {
      sendPrimary.disabled(!sendClipboard.checked());
    } else if (checkbox == &useLocalCursor) {
      dotWhenNoCursor.disabled(!useLocalCursor.checked());
    } else if (checkbox == &customCompressLevel) {
      compressLevel.disabled(!customCompressLevel.checked());
    } else if (checkbox == &noJpeg) {
      qualityLevel.disabled(autoSelect.checked() || !noJpeg.checked());
#ifdef HAVE_GNUTLS
    } else if (checkbox == &secVeNCrypt) {
      encTLS.checked(false);
      encTLS.disabled(!secVeNCrypt.checked());
      encX509.checked(false);
      encX509.disabled(!secVeNCrypt.checked());
      secPlain.checked(false);
      secPlain.disabled(!secVeNCrypt.checked());
#endif
    }
  }

  virtual void entryCallback(TXEntry* e, Detail detail, Time time) {
  }

  OptionsDialogCallback* cb;
  TXLabel formatAndEnc, inputs, misc;
  TXCheckbox autoSelect;
  TXCheckbox fullColour, mediumColour, lowColour, veryLowColour;
  TXCheckbox tight, zrle, hextile, raw;

  TXCheckbox customCompressLevel; TXEntry compressLevel; TXLabel compressLevelLabel;
  TXCheckbox noJpeg; TXEntry qualityLevel; TXLabel qualityLevelLabel;

  TXCheckbox viewOnly, acceptClipboard, sendClipboard, sendPrimary;
  TXCheckbox shared, fullScreen, useLocalCursor, dotWhenNoCursor;
  TXButton okButton, cancelButton;

#ifdef HAVE_GNUTLS
  TXLabel security;
  TXCheckbox secVeNCrypt;

  TXLabel encryption;
  TXCheckbox encNone, encTLS, encX509;
  TXLabel cacert; TXEntry ca; TXLabel crlcert; TXEntry crl;

  TXLabel authentication;
  TXCheckbox secNone, secVnc, secPlain;
#endif
};

#endif
