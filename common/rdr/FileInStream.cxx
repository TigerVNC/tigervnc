/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2013 D. R. Commander.  All Rights Reserved.
 * Copyright 2015 Pierre Ossman for Cendio AB
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

#include <errno.h>

#include <rdr/Exception.h>
#include <rdr/FileInStream.h>

using namespace rdr;

FileInStream::FileInStream(const char *fileName)
{
  file = fopen(fileName, "rb");
  if (!file)
    throw SystemException("fopen", errno);
}

FileInStream::~FileInStream(void) {
  if (file) {
    fclose(file);
    file = NULL;
  }
}

bool FileInStream::fillBuffer()
{
  size_t n = fread((U8 *)end, 1, availSpace(), file);
  if (n == 0) {
    if (ferror(file))
      throw SystemException("fread", errno);
    if (feof(file))
      throw EndOfStream();
    return false;
  }
  end += n;

  return true;
}
