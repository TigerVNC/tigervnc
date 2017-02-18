
/***********************************************************

Copyright 1987, 1998  The Open Group

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


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

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

******************************************************************/

#ifndef _X11_XUTIL_H_
#define _X11_XUTIL_H_

/* You must include <X11/Xlib.h> before including this file */
#include "Xlib.h"

/****** Avoid symbol clash with "real" libX11 ******/
#define XClipBox vncXClipBox
#define XCreateRegion vncXCreateRegion
#define XDestroyRegion vncXDestroyRegion
#define XEmptyRegion vncXEmptyRegion
#define XEqualRegion vncXEqualRegion
#define XIntersectRegion vncXIntersectRegion
#define XOffsetRegion vncXOffsetRegion
#define XPointInRegion vncXPointInRegion
#define XPolygonRegion vncXPolygonRegion
#define XRectInRegion vncXRectInRegion
#define XShrinkRegion vncXShrinkRegion
#define XSubtractRegion vncXSubtractRegion
#define XUnionRectWithRegion vncXUnionRectWithRegion
#define XUnionRegion vncXUnionRegion
#define XXorRegion vncXXorRegion

/*
 * opaque reference to Region data type
 */
typedef struct _XRegion *Region;

/* Return values from XRectInRegion() */

#define RectangleOut 0
#define RectangleIn  1
#define RectanglePart 2

extern int XClipBox(
    Region		/* r */,
    XRectangle*		/* rect_return */
);

extern Region XCreateRegion(
    void
);

extern int XDestroyRegion(
    Region		/* r */
);

extern int XEmptyRegion(
    Region		/* r */
);

extern int XEqualRegion(
    Region		/* r1 */,
    Region		/* r2 */
);

extern int XIntersectRegion(
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
);

extern int XOffsetRegion(
    Region		/* r */,
    int			/* dx */,
    int			/* dy */
);

extern Bool XPointInRegion(
    Region		/* r */,
    int			/* x */,
    int			/* y */
);

extern Region XPolygonRegion(
    XPoint*		/* points */,
    int			/* n */,
    int			/* fill_rule */
);

extern int XRectInRegion(
    Region		/* r */,
    int			/* x */,
    int			/* y */,
    unsigned int	/* width */,
    unsigned int	/* height */
);

extern int XShrinkRegion(
    Region		/* r */,
    int			/* dx */,
    int			/* dy */
);

extern int XSubtractRegion(
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
);

extern int XUnionRectWithRegion(
    XRectangle*		/* rectangle */,
    Region		/* src_region */,
    Region		/* dest_region_return */
);

extern int XUnionRegion(
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
);

extern int XXorRegion(
    Region		/* sra */,
    Region		/* srb */,
    Region		/* dr_return */
);

#endif /* _X11_XUTIL_H_ */
