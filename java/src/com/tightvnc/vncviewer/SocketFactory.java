//
//  Copyright (C) 2002 HorizonLive.com, Inc.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// SocketFactory.java describes an interface used to substitute the
// standard Socket class by its alternative implementations.
//

import java.applet.*;
import java.net.*;
import java.io.*;

public interface SocketFactory {

  public Socket createSocket(String host, int port, Applet applet)
    throws IOException;

  public Socket createSocket(String host, int port, String[] args)
    throws IOException;
}
