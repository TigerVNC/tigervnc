/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2014-2024 Pierre Ossman for Cendio AB
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
#ifndef __RFB_EXCEPTION_H__
#define __RFB_EXCEPTION_H__

#include <stdexcept>

namespace rfb {
  class protocol_error : public std::runtime_error {
  public:
    protocol_error(const char* what_arg) noexcept : std::runtime_error(what_arg) {}
    protocol_error(const std::string& what_arg) noexcept : std::runtime_error(what_arg) {}
  };

  class auth_error : public std::runtime_error {
  public:
    auth_error(const char* reason) noexcept : std::runtime_error(reason) {}
    auth_error(std::string& reason) noexcept : std::runtime_error(reason) {}
  };

  class auth_cancelled : public std::runtime_error {
  public:
    auth_cancelled() noexcept
      : std::runtime_error("Authentication cancelled") {}
  };
}
#endif
