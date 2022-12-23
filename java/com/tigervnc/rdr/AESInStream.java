/* Copyright (C) 2022 Dinglan Peng
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

package com.tigervnc.rdr;

import java.nio.ByteBuffer;

public class AESInStream extends InStream {

  private static final int maxMessageSize = 65536;

  public AESInStream(InStream _in, byte[] key)
  {
    in = _in;
    offset = 0;
    bufSize = maxMessageSize;
    b = new byte[bufSize];
    ptr = end = start = 0;
    cipher = new AESEAXCipher(key);
    messageSize = 0;
    messageOffset = 0;
    message = new byte[maxMessageSize + 16];
    decryptedMessageOffset = 0;
    decryptedMessage = new byte[maxMessageSize];
    counter = new byte[16];
    state = 0;
  }

  public final int pos()
  {
    return offset + ptr - start;
  }

  protected final int overrun(int itemSize, int nItems, boolean wait)
  {
    if (itemSize > bufSize)
      throw new Exception("AESInStream overrun: max itemSize exceeded");

    if (end - ptr != 0)
      System.arraycopy(b, ptr, b, 0, end - ptr);

    offset += ptr - start;
    end -= ptr - start;
    ptr = start;

    while ((end - start) < itemSize) {
      int n = readMessage(b, end, start + bufSize - end, wait);
      if (!wait && n == 0)
        return 0;
      end += n;
    }

    int nAvail;
    nAvail = (end - ptr) / itemSize;
    if (nAvail < nItems)
      return nAvail;

    return nItems;
  }

  private int readMessage(byte[] buf, int bufPtr, int len, boolean wait)
  {
    if (state == 0 || state == 1) {
      if (!fillDecryptedMessageBuffer(wait) && !wait)
        return 0;
    }
    if (state == 2) {
      int readSize = messageSize - decryptedMessageOffset;
      if (readSize > len)
        readSize = len;
      System.arraycopy(decryptedMessage, decryptedMessageOffset,
                       buf, bufPtr, readSize);
      decryptedMessageOffset += readSize;
      if (decryptedMessageOffset == messageSize)
        state = 0;
      return readSize;
    }
    return 0;
  }

  private boolean fillDecryptedMessageBuffer(boolean wait)
  {
    if (state == 0) {
      while (true) {
        if (in.check(2, 1, wait) != 0) {
          messageSize = in.readU16();
          messageOffset = 0;
          state = 1;
          break;
        } else if (!wait) {
          return false;
        }
      }
    }
    if (state == 1) {
      if (wait) {
        in.readBytes(ByteBuffer.wrap(message, messageOffset,
                                     messageSize + 16 - messageOffset),
                     messageSize + 16 - messageOffset);
      } else {
        while (true) {
          int readSize = messageSize + 16 - messageOffset;
          if (in.check(1, readSize, false) != 0) {
            int availSize = in.getend() - in.getptr();
            if (readSize > availSize)
              readSize = availSize;
            in.readBytes(ByteBuffer.wrap(message, messageOffset, readSize),
                         readSize);
            messageOffset += readSize;
            if (messageSize + 16 == messageOffset) {
              break;
            }
          } else {
            return false;
          }
        }
      }
    }
    byte[] ad = new byte[] {
      (byte)((messageSize & 0xff00) >> 8),
      (byte)(messageSize & 0xff)
    };
    cipher.decrypt(message, 0, messageSize,
                   ad, 0, 2,
                   counter,
                   decryptedMessage, 0,
                   message, messageSize);
    // Update nonce by incrementing the counter as a
    // 128bit little endian unsigned integer
    for (int i = 0; i < 16; ++i) {
      // increment until there is no carry
      if (++counter[i] != 0) {
        break;
      }
    }
    decryptedMessageOffset = 0;
    state = 2;
    return true;
  }


  private AESEAXCipher cipher;
  private int offset;
  private int start;
  private int bufSize;
  private int state;
  private int messageSize;
  private int messageOffset;
  private byte[] message;
  private int decryptedMessageOffset;
  private byte[] decryptedMessage;
  private byte[] counter;
  private InStream in;
}
