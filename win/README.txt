
TigerVNC Source Distribution for Windows platforms
==================================================

Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
Copyright (C) 2000-2004 Constantin Kaplinsky.
Copyright (C) 2004-2009 Peter Astrand for Cendio AB

This software is distributed under the GNU General Public Licence as
published by the Free Software Foundation.  See the file LICENCE.TXT
for the conditions under which this software is made available.
TigerVNC also contains code from other sources.  See the
Acknowledgements section below, and the individual files for details
of the conditions under which they are made available.

The source tree contains a number of directories, and is most easily
built by loading the VNC workspace file (vnc.dsw) into Microsoft
Visual Studio 6/7.  This will preserve the required dependencies
between the sub-projects.

There are three main executable projects:

	vncviewer - The VNC Viewer for Win32.

	winvnc - The VNC Server for Win32 (command-line operation
		 only).

	vncconfig - The configuration applet and GUI front-end for VNC
		    Server.

These projects are designed to be built using Microsoft Visual C++
6.0, and should also compile with Microsoft Visual Studio .NET
(version 7).  Other compilers have not been tested but the code base
is extremely portable.


ACKNOWLEDGEMENTS
================

This distribution contains zlib software by Jean-loup Gailly and Mark Adler.
This is:

    Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler.

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

	Jean-loup Gailly        Mark Adler
	jloup@gzip.org          madler@alumni.caltech.edu


	The data format used by the zlib library is described by RFCs (Request for
	Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
	(zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).


This distribution contains public domain DES software by Richard Outerbridge.
This is:

	Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge.
	(GEnie : OUTER; CIS : [71755,204]) Graven Imagery, 1992.
