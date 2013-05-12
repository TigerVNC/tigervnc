/* Copyright 2011 Pierre Ossman for Cendio AB
 * Copyright (C) 2012 Brian P. Hinz
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

public class fenceTypes {
  public static final int fenceFlagBlockBefore = 1<<0;
  public static final int fenceFlagBlockAfter  = 1<<1;
  public static final int fenceFlagSyncNext    = 1<<2;

  public static final int fenceFlagRequest     = 1<<31;

  public static final int fenceFlagsSupported  = (fenceFlagBlockBefore |
                                                  fenceFlagBlockAfter |
                                                  fenceFlagSyncNext |
                                                  fenceFlagRequest);
}
