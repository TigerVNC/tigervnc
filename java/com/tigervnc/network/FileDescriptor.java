/* Copyright (C) 2012 Brian P. Hinz
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
package com.tigervnc.network;

import java.io.IOException;
import java.nio.ByteBuffer;
import com.tigervnc.rdr.Exception;

public interface FileDescriptor {

  public int read(ByteBuffer buf, int length) throws Exception;
  public int write(ByteBuffer buf, int length) throws Exception;
  public int select(int interestOps, Integer timeout) throws Exception;
  public void close() throws IOException;

}
