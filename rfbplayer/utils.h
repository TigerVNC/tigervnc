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

// -=- utils.h

// Convert a long integer to a string.

char *LongToStr(long num) {
  char str[20];
  _ltoa(num, str, 10);
  return strDup(str);
}

// Convert a double to a string:
//   len - number of digits after decimal point.

char *DoubleToStr(double num, int lenAfterPoint=1) {
  int decimal, sign;
  char *str;
  char dstr[20];
  str = _fcvt(num, lenAfterPoint, &decimal, &sign);
  int len = lenAfterPoint + decimal + sign + 2;
  dstr[0] = '\0';
  if (sign)
    strcat(dstr, "-");
  strncat(dstr, str, decimal);
  strcat(dstr, ".");
  strncat(dstr, str + decimal, len - int(dstr + decimal));
  return strDup(dstr);
}
