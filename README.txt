
TightVNC Source Distribution for Windows platforms
==================================================

Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
Copyright (C) 2000-2004 Constantin Kaplinsky.
Copyright (C) 2004 Peter Astrand, Cendio AB

This software is distributed under the GNU General Public Licence as
published by the Free Software Foundation.  See the file LICENCE.TXT
for the conditions under which this software is made available.
TightVNC also contains code from other sources.  See the
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


This distribution contains Java DES software by Dave Zimmerman
<dzimm@widget.com> and Jef Poskanzer <jef@acme.com>.  This is:

    Copyright (c) 1996 Widget Workshop, Inc. All Rights Reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for NON-COMMERCIAL or COMMERCIAL purposes and without fee
    is hereby granted, provided that this copyright notice is kept intact.
    
    WIDGET WORKSHOP MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
    SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
    NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
    PARTICULAR PURPOSE, OR NON-INFRINGEMENT. WIDGET WORKSHOP SHALL NOT BE
    LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
    MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
    
    THIS SOFTWARE IS NOT DESIGNED OR INTENDED FOR USE OR RESALE AS ON-LINE
    CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING FAIL-SAFE
    PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES, AIRCRAFT
    NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT LIFE
    SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
    SOFTWARE COULD LEAD DIRECTLY TO DEATH, PERSONAL INJURY, OR SEVERE
    PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH RISK ACTIVITIES").  WIDGET
    WORKSHOP SPECIFICALLY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY OF
    FITNESS FOR HIGH RISK ACTIVITIES.

    Copyright (C) 1996 by Jef Poskanzer <jef@acme.com>.  All rights
    reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    Visit the ACME Labs Java page for up-to-date versions of this and other
    fine Java utilities: http://www.acme.com/java/
