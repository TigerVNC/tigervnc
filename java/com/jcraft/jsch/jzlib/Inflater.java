/*
 * Copyright (c) 2011 ymnk, JCraft,Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3. The names of the authors may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JCRAFT, INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * This program is based on zlib-1.1.3, so all credit should go authors Jean-loup
 * Gailly(jloup@gzip.org) and Mark Adler(madler@alumni.caltech.edu) and contributors of zlib.
 */

package com.jcraft.jsch.jzlib;

final class Inflater extends ZStream {

  private static final int MAX_WBITS = 15; // 32K LZ77 window
  private static final int DEF_WBITS = MAX_WBITS;

  private static final int Z_NO_FLUSH = 0;
  private static final int Z_PARTIAL_FLUSH = 1;
  private static final int Z_SYNC_FLUSH = 2;
  private static final int Z_FULL_FLUSH = 3;
  private static final int Z_FINISH = 4;

  private static final int MAX_MEM_LEVEL = 9;

  private static final int Z_OK = 0;
  private static final int Z_STREAM_END = 1;
  private static final int Z_NEED_DICT = 2;
  private static final int Z_ERRNO = -1;
  private static final int Z_STREAM_ERROR = -2;
  private static final int Z_DATA_ERROR = -3;
  private static final int Z_MEM_ERROR = -4;
  private static final int Z_BUF_ERROR = -5;
  private static final int Z_VERSION_ERROR = -6;

  private int param_w = -1;
  private JZlib.WrapperType param_wrapperType = null;
  private boolean param_nowrap = false;

  Inflater() {
    super();
    init();
  }

  Inflater(JZlib.WrapperType wrapperType) throws GZIPException {
    this(DEF_WBITS, wrapperType);
  }

  Inflater(int w, JZlib.WrapperType wrapperType) throws GZIPException {
    super();
    param_w = w;
    param_wrapperType = wrapperType;
    int ret = init(w, wrapperType);
    if (ret != Z_OK)
      throw new GZIPException(ret + ": " + msg);
  }

  Inflater(int w) throws GZIPException {
    this(w, false);
  }

  Inflater(boolean nowrap) throws GZIPException {
    this(DEF_WBITS, nowrap);
  }

  Inflater(int w, boolean nowrap) throws GZIPException {
    super();
    param_w = w;
    param_nowrap = nowrap;
    int ret = init(w, nowrap);
    if (ret != Z_OK)
      throw new GZIPException(ret + ": " + msg);
  }

  void reset() {
    finished = false;
    if (param_wrapperType != null) {
      init(param_w, param_wrapperType);
    } else {
      init(param_w, param_nowrap);
    }
  }

  private boolean finished = false;

  int init() {
    return init(DEF_WBITS);
  }

  int init(JZlib.WrapperType wrapperType) {
    return init(DEF_WBITS, wrapperType);
  }

  int init(int w, JZlib.WrapperType wrapperType) {
    boolean nowrap = false;
    if (wrapperType == JZlib.W_NONE) {
      nowrap = true;
    } else if (wrapperType == JZlib.W_GZIP) {
      w += 16;
    } else if (wrapperType == JZlib.W_ANY) {
      w |= Inflate.INFLATE_ANY;
    } else if (wrapperType == JZlib.W_ZLIB) {
    }
    return init(w, nowrap);
  }

  int init(boolean nowrap) {
    return init(DEF_WBITS, nowrap);
  }

  int init(int w) {
    return init(w, false);
  }

  int init(int w, boolean nowrap) {
    finished = false;
    istate = new Inflate(this);
    return istate.inflateInit(nowrap ? -w : w);
  }

  @Override
  int inflate(int f) {
    if (istate == null)
      return Z_STREAM_ERROR;
    int ret = istate.inflate(f);
    if (ret == Z_STREAM_END)
      finished = true;
    return ret;
  }

  @Override
  int end() {
    finished = true;
    if (istate == null)
      return Z_STREAM_ERROR;
    int ret = istate.inflateEnd();
    // istate = null;
    return ret;
  }

  int sync() {
    if (istate == null)
      return Z_STREAM_ERROR;
    return istate.inflateSync();
  }

  int syncPoint() {
    if (istate == null)
      return Z_STREAM_ERROR;
    return istate.inflateSyncPoint();
  }

  int setDictionary(byte[] dictionary, int dictLength) {
    if (istate == null)
      return Z_STREAM_ERROR;
    return istate.inflateSetDictionary(dictionary, dictLength);
  }

  @Override
  boolean finished() {
    return istate.mode == 12 /* DONE */;
  }
}
