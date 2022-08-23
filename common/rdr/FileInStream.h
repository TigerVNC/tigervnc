/* Copyright (C) 2013 D. R. Commander.  All Rights Reserved.
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

#ifndef __RDR_FILEINSTREAM_H__
#define __RDR_FILEINSTREAM_H__

#include <stdio.h>

#include <rdr/BufferedInStream.h>

namespace rdr {

  class FileInStream : public BufferedInStream {

  public:

    FileInStream(const char *fileName);
    ~FileInStream(void);

  private:
    virtual bool fillBuffer();

  private:
    FILE *file;
  };

} // end of namespace rdr

#endif
