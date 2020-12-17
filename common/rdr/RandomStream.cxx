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

#include <rdr/RandomStream.h>
#include <rdr/Exception.h>
#include <rfb/LogWriter.h>
#include <time.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#include <errno.h>
#else
#define getpid() GetCurrentProcessId()
#ifndef RFB_HAVE_WINCRYPT
#pragma message("  NOTE: Not building WinCrypt-based RandomStream")
#endif
#endif

static rfb::LogWriter vlog("RandomStream");

using namespace rdr;

const size_t DEFAULT_BUF_LEN = 256;

unsigned int RandomStream::seed;

RandomStream::RandomStream()
  : offset(0)
{
  ptr = end = start = new U8[DEFAULT_BUF_LEN];

#ifdef RFB_HAVE_WINCRYPT
  provider = 0;
  if (!CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, 0)) {
    if (GetLastError() == (DWORD)NTE_BAD_KEYSET) {
      if (!CryptAcquireContext(&provider, 0, 0, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
        vlog.error("unable to create keyset");
        provider = 0;
      }
    } else {
      vlog.error("unable to acquire context");
      provider = 0;
    }
  }
  if (!provider) {
#else
#ifndef WIN32
  fp = fopen("/dev/urandom", "r");
  if (!fp)
    fp = fopen("/dev/random", "r");
  if (!fp) {
#else
  {
#endif
#endif
    vlog.error("no OS supplied random source - using rand()");
    seed += (unsigned int) time(0) + getpid() + getpid() * 987654 + rand();
    srand(seed);
  }
}

RandomStream::~RandomStream() {
  delete [] start;

#ifdef RFB_HAVE_WINCRYPT
  if (provider)
    CryptReleaseContext(provider, 0);
#endif
#ifndef WIN32
  if (fp) fclose(fp);
#endif
}

size_t RandomStream::pos() {
  return offset + ptr - start;
}

size_t RandomStream::overrun(size_t itemSize, size_t nItems, bool wait) {
  if (itemSize > DEFAULT_BUF_LEN)
    throw Exception("RandomStream overrun: max itemSize exceeded");

  if (end - ptr != 0)
    memmove(start, ptr, end - ptr);

  end -= ptr - start;
  offset += ptr - start;
  ptr = start;

  size_t length = start + DEFAULT_BUF_LEN - end;

#ifdef RFB_HAVE_WINCRYPT
  if (provider) {
    if (!CryptGenRandom(provider, length, (U8*)end))
      throw rdr::SystemException("unable to CryptGenRandom", GetLastError());
    end += length;
  } else {
#else
#ifndef WIN32
  if (fp) {
    size_t n = fread((U8*)end, length, 1, fp);
    if (n != 1)
      throw rdr::SystemException("reading /dev/urandom or /dev/random failed",
                                 errno);
    end += length;
  } else {
#else
  {
#endif
#endif
    for (size_t i=0; i<length; i++)
      *(U8*)end++ = (int) (256.0*rand()/(RAND_MAX+1.0));
  }

  size_t nAvail;
  nAvail = (end - ptr) / itemSize;
  if (nAvail < nItems)
    return nAvail;

  return nItems;
}
