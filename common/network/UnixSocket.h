/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (c) 2012 University of Oslo.  All Rights Reserved.
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

// -=- UnixSocket.h - base-class for UNIX stream sockets.
//     This header also defines the UnixListener class, used
//     to listen for incoming socket connections over UNIX
//
//     NB: Any file descriptors created by the UnixSocket or
//     UnixListener classes are close-on-exec if the OS supports
//     it.  UnixSockets initialised with a caller-supplied fd
//     are NOT set to close-on-exec.

#ifndef __NETWORK_UNIX_SOCKET_H__
#define __NETWORK_UNIX_SOCKET_H__

#include <network/Socket.h>

namespace network {

  class UnixSocket : public Socket {
  public:
    UnixSocket(int sock);
    UnixSocket(const char *name);

    virtual char* getPeerAddress();
    virtual char* getPeerEndpoint();
  };

  class UnixListener : public SocketListener {
  public:
    UnixListener(const char *listenaddr, int mode);
    virtual ~UnixListener();

    int getMyPort();

  protected:
    virtual Socket* createSocket(int fd);
  };

}

#endif // __NETWORK_UNIX_SOCKET_H__
