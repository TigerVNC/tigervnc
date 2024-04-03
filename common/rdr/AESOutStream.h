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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef __RDR_AESOUTSTREAM_H__
#define __RDR_AESOUTSTREAM_H__

#ifdef HAVE_NETTLE
#include <nettle/eax.h>
#include <nettle/aes.h>
#include <rdr/BufferedOutStream.h>

namespace rdr {

  class AESOutStream : public BufferedOutStream {
  public:
    AESOutStream(OutStream* out, const uint8_t* key, int keySize);
    virtual ~AESOutStream();

    void flush() override;
    void cork(bool enable) override;

  private:
    bool flushBuffer() override;
    void writeMessage(const uint8_t* data, size_t length);

    int keySize;
    OutStream* out;
    uint8_t* msg;
    union {
      struct EAX_CTX(aes128_ctx) eaxCtx128;
      struct EAX_CTX(aes256_ctx) eaxCtx256;
    };
    uint8_t counter[16];
  };
};

#endif
#endif
