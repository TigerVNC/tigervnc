/* Copyright 2009 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

public class screenTypes {

  // Reasons
  public static final int reasonServer = 0;
  public static final int reasonClient = 1;
  public static final int reasonOtherClient = 2;

  // Result codes
  public static final int resultSuccess = 0;
  public static final int resultProhibited = 1;
  public static final int resultNoResources = 2;
  public static final int resultInvalid = 3;

  public static final int resultUnsolicited = 0xffff; // internal code used for server changes

}
