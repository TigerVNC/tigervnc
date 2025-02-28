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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rdr/RandomStream.h>

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

static core::LogWriter vlog("RandomStream");

using namespace rdr;

unsigned int RandomStream::seed;

RandomStream::RandomStream()
{
#ifdef RFB_HAVE_WINCRYPT
  provider = 0;
  if (!CryptAcquireContext(&provider, nullptr, nullptr,
                           PROV_RSA_FULL, 0)) {
    if (GetLastError() == (DWORD)NTE_BAD_KEYSET) {
      if (!CryptAcquireContext(&provider, nullptr, nullptr,
                               PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
        vlog.error("Unable to create keyset");
        provider = 0;
      }
    } else {
      vlog.error("Unable to acquire context");
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
    vlog.error("No OS supplied random source, using rand()");
    seed += (unsigned int) time(nullptr) + getpid() + getpid() * 987654 + rand();
    srand(seed);
  }
}

RandomStream::~RandomStream() {
#ifdef RFB_HAVE_WINCRYPT
  if (provider)
    CryptReleaseContext(provider, 0);
#endif
#ifndef WIN32
  if (fp) fclose(fp);
#endif
}

bool RandomStream::fillBuffer() {
#ifdef RFB_HAVE_WINCRYPT
  if (provider) {
    if (!CryptGenRandom(provider, availSpace(), (uint8_t*)end))
      throw core::win32_error("Unable to CryptGenRandom", GetLastError());
    end += availSpace();
  } else {
#else
#ifndef WIN32
  if (fp) {
    size_t n = fread((uint8_t*)end, 1, availSpace(), fp);
    if (n <= 0)
      throw core::posix_error(
        "Reading /dev/urandom or /dev/random failed", errno);
    end += n;
  } else {
#else
  {
#endif
#endif
    for (size_t i=availSpace(); i>0; i--)
      *(uint8_t*)end++ = (int) (256.0*rand()/(RAND_MAX+1.0));
  }

  return true;
}
