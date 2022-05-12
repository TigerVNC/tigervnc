About TigerVNC
==============

Virtual Network Computing (VNC) is a remote display system which allows you to
view and interact with a virtual desktop environment that is running on another
computer on the network.  Using VNC, you can run graphical applications on a
remote machine and send only the display from these applications to your local
machine.  VNC is platform-independent and supports a wide variety of operating
systems and architectures as both servers and clients.

TigerVNC is a high-speed version of VNC based on the RealVNC 4 and X.org code
bases.  TigerVNC started as a next-generation development effort for TightVNC
on Unix and Linux platforms, but it split from its parent project in early 2009
so that TightVNC could focus on Windows platforms.  TigerVNC supports a variant
of Tight encoding that is greatly accelerated by the use of the libjpeg-turbo
JPEG codec.


Legal
=====

Incomplete and generally out of date copyright list::

        Copyright (C) 1999 AT&T Laboratories Cambridge
        Copyright (C) 2002-2005 RealVNC Ltd.
        Copyright (C) 2000-2006 TightVNC Group
        Copyright (C) 2005-2006 Martin Koegler
        Copyright (C) 2005-2006 Sun Microsystems, Inc.
        Copyright (C) 2006 OCCAM Financial Technology
        Copyright (C) 2000-2008 Constantin Kaplinsky
        Copyright (C) 2004-2017 Peter Astrand for Cendio AB
        Copyright (C) 2010 Antoine Martin
        Copyright (C) 2010 m-privacy GmbH
        Copyright (C) 2009-2011 D. R. Commander
        Copyright (C) 2009-2011 Pierre Ossman for Cendio AB
        Copyright (C) 2004, 2009-2011 Red Hat, Inc.
        Copyright (C) 2009-2022 TigerVNC Team
        All Rights Reserved.

This software is distributed under the GNU General Public Licence as published
by the Free Software Foundation.  See the file LICENCE.TXT for the conditions
under which this software is made available.  TigerVNC also contains code from
other sources.  See the Acknowledgements section below, and the individual
source files, for details of the conditions under which they are made
available.


All Platforms
=============

All versions of TigerVNC contain the following programs:

* vncviewer - the cross-platform TigerVNC Viewer, written using FLTK.
              vncviewer connects to a VNC server and allows you to interact
              with the remote desktop being displayed by the VNC server.  The
              VNC server can be running on a Windows or a Unix/Linux machine.


Windows-Specific
================

The Windows version of TigerVNC contains the following programs:

* winvnc - the TigerVNC Server for Windows.  winvnc allows a Windows desktop to
           be accessed remotely using a VNC viewer.

WARNING: winvnc is currently unmaintained and and may not function correctly.

winvnc may not work if the Fast User Switching or Remote Desktop features are
in use.


Unix/Linux-Specific (not Mac)
=============================

The Unix/Linux version of TigerVNC contains the following programs:

* Xvnc - the TigerVNC Server for Unix.  Xvnc is both a VNC server and an X
         server with a "virtual" framebuffer.  You should normally use the
         vncserver service to start Xvnc.

* vncpasswd - a program which allows you to change the VNC password used to
              access your VNC server sessions (assuming that VNC authentication
              is being used.) This command must be run to set a password before
              using VNC authentication with any of the servers or services.

* vncconfig - a program which is used to configure and control a running
              instance of Xvnc.

* x0vncserver - an inefficient VNC server which continuously polls any X
                display, allowing it to be controlled via VNC.  It is intended
                mainly as a demonstration of a simple VNC server.

It also contains the following systemd service:

* vncserver@.service - a service to start a user session with Xvnc and one of
                       the desktop environments available on the system.

ACKNOWLEDGEMENTS
================

This distribution contains public domain DES software by Richard Outerbridge.
This is:

    Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge.
    (GEnie : OUTER; CIS : [71755,204]) Graven Imagery, 1992.


This distribution contains software from the X Window System.  This is:

 Copyright 1987, 1988, 1998  The Open Group
 
 Permission to use, copy, modify, distribute, and sell this software and its
 documentation for any purpose is hereby granted without fee, provided that
 the above copyright notice appear in all copies and that both that
 copyright notice and this permission notice appear in supporting
 documentation.
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name of The Open Group shall not be
 used in advertising or otherwise to promote the sale, use or other dealings
 in this Software without prior written authorization from The Open Group.
 
 
 Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.
 
                         All Rights Reserved
 
 Permission to use, copy, modify, and distribute this software and its 
 documentation for any purpose and without fee is hereby granted, 
 provided that the above copyright notice appear in all copies and that
 both that copyright notice and this permission notice appear in 
 supporting documentation, and that the name of Digital not be
 used in advertising or publicity pertaining to distribution of the
 software without specific, written prior permission.  
 
 DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 SOFTWARE.
