/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- SessionInfoDialog.h

#include <math.h>

#include <rfb/ConnParams.h>

#include <rfb_win32/Dialog.h>

#define log2(n) log(n) / 0.693147180559945

int max3(int v1, int v2, int v3) {
  return max(v1, max(v2, v3));
}

class SessionInfoDialog : public rfb::win32::Dialog {
public:
  SessionInfoDialog(ConnParams *_cp, int _currentEncoding) 
    : Dialog(GetModuleHandle(0)), cp(_cp), currentEncoding(_currentEncoding) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent = 0) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_SESSION_INFO), parent);
  }
protected:
  // Dialog methods (protected)
  virtual void initDialog() {
    char strValue[255] = "\0";
    setItemString(IDC_DESKTOP_NAME, cp->name());
    
    sprintf(strValue, "%ix%i", cp->width, cp->height);
    setItemString(IDC_DESKTOP_SIZE, strValue);
    
    int r = cp->pf().redShift, g = cp->pf().greenShift, b = cp->pf().blueShift;
    int i = 3;
    char buffer[10];
    sprintf(strValue, "depth %i(%ibpp), ", cp->pf().depth, cp->pf().bpp);
    while (i) {
      if (r == max3(r, g, b)) {
        strcat(strValue, "r");
        _itoa(ceil(log2(cp->pf().redMax)), buffer, 10);
        strcat(strValue, buffer);
        r = -1; 
        i--;
        continue;
      } else if (g == max3(r, g, b)) {
        strcat(strValue, "g");
        _itoa(ceil(log2(cp->pf().greenMax)), buffer, 10);
        strcat(strValue, buffer);
        g = -1; 
        i--;
        continue;
      } else if (b == max3(r, g, b)) {
        strcat(strValue, "b");
        _itoa(ceil(log2(cp->pf().blueMax)), buffer, 10);
        strcat(strValue, buffer);
        b = -1; 
        i--;
        continue;
      } else break;
    }
    if (cp->pf().bigEndian) strcat(strValue, ", big-endian");
    else strcat(strValue, ", little-endian");
    setItemString(IDC_PIXEL_FORMAT, strValue);

    switch (currentEncoding) {
    case encodingRaw:      strcpy(strValue, "Raw");      break;
    case encodingCopyRect: strcpy(strValue, "CopyRect"); break;
    case encodingRRE:      strcpy(strValue, "RRE");      break;
    case encodingCoRRE:    strcpy(strValue, "CoRRE");    break;
    case encodingHextile:  strcpy(strValue, "Hextile");  break;
    case encodingTight:    strcpy(strValue, "Tight");    break;
    case encodingZRLE:     strcpy(strValue, "ZRLE");     break;
    default:               strcpy(strValue, "Unknown");
    }
    setItemString(IDC_CURRENT_ENCODING, strValue);

    sprintf(strValue, "%i.%i", cp->majorVersion, cp->minorVersion);
    setItemString(IDC_VERSION, strValue);
  }
  ConnParams *cp;
  int currentEncoding;
};