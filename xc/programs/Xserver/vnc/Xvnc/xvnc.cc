/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

/*

Copyright (c) 1993  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

#include <rfb/Configuration.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <network/TcpSocket.h>
#include "vncExtInit.h"

extern "C" {
#define class c_class
#define public c_public
#define xor c_xor
#define and c_and
#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include <stdio.h>
#include "X11/X.h"
#define NEED_EVENTS
#include "X11/Xproto.h"
#include "X11/Xos.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "fb.h"
#include "mi.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#define new New
#include "micmap.h"
#undef new
#include <sys/types.h>
#ifdef HAS_MMAP
#include <sys/mman.h>
#ifndef MAP_FILE
#define MAP_FILE 0
#endif
#endif /* HAS_MMAP */
#include <sys/stat.h>
#include <errno.h>
#ifndef WIN32
#include <sys/param.h>
#endif
#include <X11/XWDFile.h>
#ifdef HAS_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif /* HAS_SHM */
#include "dix.h"
#include "miline.h"
#include "inputstr.h"
#include "keysym.h"
  extern int defaultColorVisualClass;
  extern char buildtime[];
#undef class
#undef public
#undef xor
#undef and
}

#define XVNCVERSION "4.0"

extern char *display;
extern int monitorResolution;

#define VFB_DEFAULT_WIDTH  1024
#define VFB_DEFAULT_HEIGHT 768
#define VFB_DEFAULT_DEPTH  16
#define VFB_DEFAULT_WHITEPIXEL 0xffff
#define VFB_DEFAULT_BLACKPIXEL 0
#define VFB_DEFAULT_LINEBIAS 0
#define XWD_WINDOW_NAME_LEN 60

typedef struct
{
    int scrnum;
    int width;
    int paddedBytesWidth;
    int paddedWidth;
    int height;
    int depth;
    int bitsPerPixel;
    int sizeInBytes;
    int ncolors;
    char *pfbMemory;
    XWDColor *pXWDCmap;
    XWDFileHeader *pXWDHeader;
    Pixel blackPixel;
    Pixel whitePixel;
    unsigned int lineBias;
    CloseScreenProcPtr closeScreen;

#ifdef HAS_MMAP
    int mmap_fd;
    char mmap_file[MAXPATHLEN];
#endif

#ifdef HAS_SHM
    int shmid;
#endif

    Bool pixelFormatDefined;
    Bool rgbNotBgr;
    int redBits, greenBits, blueBits;

} vfbScreenInfo, *vfbScreenInfoPtr;

static int vfbNumScreens;
static vfbScreenInfo vfbScreens[MAXSCREENS];
static Bool vfbPixmapDepths[33];
#ifdef HAS_MMAP
static char *pfbdir = NULL;
#endif
typedef enum { NORMAL_MEMORY_FB, SHARED_MEMORY_FB, MMAPPED_FILE_FB } fbMemType;
static fbMemType fbmemtype = NORMAL_MEMORY_FB;
static char needswap = 0;
static int lastScreen = -1;
static Bool Render = TRUE;

static bool displaySpecified = false;
static bool wellKnownSocketsCreated = false;
static char displayNumStr[16];

#define swapcopy16(_dst, _src) \
    if (needswap) { CARD16 _s = _src; cpswaps(_s, _dst); } \
    else _dst = _src;

#define swapcopy32(_dst, _src) \
    if (needswap) { CARD32 _s = _src; cpswapl(_s, _dst); } \
    else _dst = _src;


static void
vfbInitializePixmapDepths(void)
{
    int i;
    vfbPixmapDepths[1] = TRUE; /* always need bitmaps */
    for (i = 2; i <= 32; i++)
	vfbPixmapDepths[i] = FALSE;
}

static void
vfbInitializeDefaultScreens(void)
{
    int i;

    for (i = 0; i < MAXSCREENS; i++)
    {
	vfbScreens[i].scrnum = i;
	vfbScreens[i].width  = VFB_DEFAULT_WIDTH;
	vfbScreens[i].height = VFB_DEFAULT_HEIGHT;
	vfbScreens[i].depth  = VFB_DEFAULT_DEPTH;
	vfbScreens[i].blackPixel = VFB_DEFAULT_BLACKPIXEL;
	vfbScreens[i].whitePixel = VFB_DEFAULT_WHITEPIXEL;
	vfbScreens[i].lineBias = VFB_DEFAULT_LINEBIAS;
	vfbScreens[i].pixelFormatDefined = FALSE;
	vfbScreens[i].pfbMemory = NULL;
    }
    vfbNumScreens = 1;
}

static int
vfbBitsPerPixel(int depth)
{
    if (depth == 1) return 1;
    else if (depth <= 8) return 8;
    else if (depth <= 16) return 16;
    else return 32;
}

extern "C" {
void
ddxGiveUp()
{
    int i;

    /* clean up the framebuffers */

    switch (fbmemtype)
    {
#ifdef HAS_MMAP
    case MMAPPED_FILE_FB: 
	for (i = 0; i < vfbNumScreens; i++)
	{
	    if (-1 == unlink(vfbScreens[i].mmap_file))
	    {
		perror("unlink");
		ErrorF("unlink %s failed, errno %d",
		       vfbScreens[i].mmap_file, errno);
	    }
	}
	break;
#else /* HAS_MMAP */
    case MMAPPED_FILE_FB:
        break;
#endif /* HAS_MMAP */
	
#ifdef HAS_SHM
    case SHARED_MEMORY_FB:
	for (i = 0; i < vfbNumScreens; i++)
	{
	    if (-1 == shmdt((char *)vfbScreens[i].pXWDHeader))
	    {
		perror("shmdt");
		ErrorF("shmdt failed, errno %d", errno);
	    }
	}
	break;
#else /* HAS_SHM */
    case SHARED_MEMORY_FB:
        break;
#endif /* HAS_SHM */
	
    case NORMAL_MEMORY_FB:
	for (i = 0; i < vfbNumScreens; i++)
	{
	    Xfree(vfbScreens[i].pXWDHeader);
	}
	break;
    }
}

void
AbortDDX()
{
    ddxGiveUp();
}

#ifdef __DARWIN__
void
DarwinHandleGUI(int argc, char *argv[])
{
}

void GlxExtensionInit();
void GlxWrapInitVisuals(void *procPtr);

void
DarwinGlxExtensionInit()
{
    GlxExtensionInit();
}

void
DarwinGlxWrapInitVisuals(
    void *procPtr)
{
    GlxWrapInitVisuals(procPtr);
}
#endif

void
OsVendorInit()
{
}

void
OsVendorFatalError()
{
}

void ddxBeforeReset(void)
{
    return;
}

void 
ddxUseMsg()
{
    ErrorF("\nXvnc version %s - built %s\n", XVNCVERSION, buildtime);
    ErrorF("Underlying X server release %d, %s\n\n", VENDOR_RELEASE,
           VENDOR_STRING);
    ErrorF("-screen scrn WxHxD     set screen's width, height, depth\n");
    ErrorF("-pixdepths list-of-int support given pixmap depths\n");
#ifdef RENDER
    ErrorF("+/-render		   turn on/off RENDER extension support"
	   "(default on)\n");
#endif
    ErrorF("-linebias n            adjust thin line pixelization\n");
    ErrorF("-blackpixel n          pixel value for black\n");
    ErrorF("-whitepixel n          pixel value for white\n");

#ifdef HAS_MMAP
    ErrorF("-fbdir directory       put framebuffers in mmap'ed files in directory\n");
#endif

#ifdef HAS_SHM
    ErrorF("-shmem                 put framebuffers in shared memory\n");
#endif

    ErrorF("-geometry WxH          set screen 0's width, height\n");
    ErrorF("-depth D               set screen 0's depth\n");
    ErrorF("-pixelformat fmt       set pixel format (rgbNNN or bgrNNN)\n");
    ErrorF("-inetd                 has been launched from inetd\n");
    ErrorF("\nVNC parameters:\n");

    fprintf(stderr,"\n"
            "Parameters can be turned on with -<param> or off with -<param>=0\n"
            "Parameters which take a value can be specified as "
            "-<param> <value>\n"
            "Other valid forms are <param>=<value> -<param>=<value> "
            "--<param>=<value>\n"
            "Parameter names are case-insensitive.  The parameters are:\n\n");
    rfb::Configuration::listParams(79, 14);
  }
}

/* ddxInitGlobals - called by |InitGlobals| from os/util.c */
void ddxInitGlobals(void)
{
}

static 
bool displayNumFree(int num)
{
    try {
	network::TcpListener l(6000+num);
    } catch (rdr::Exception& e) {
	return false;
    }
    char file[256];
    sprintf(file, "/tmp/.X%d-lock", num);
    if (access(file, F_OK) == 0) return false;
    sprintf(file, "/tmp/.X11-unix/X%d", num);
    if (access(file, F_OK) == 0) return false;
    sprintf(file, "/usr/spool/sockets/X11/%d", num);
    if (access(file, F_OK) == 0) return false;
    return true;
}

int 
ddxProcessArgument(int argc, char *argv[], int i)
{
    static Bool firstTime = TRUE;

    if (firstTime)
    {
	vfbInitializeDefaultScreens();
	vfbInitializePixmapDepths();
	firstTime = FALSE;
	rfb::initStdIOLoggers();
	rfb::LogWriter::setLogParams("*:stderr:30");
    }

    if (argv[i][0] ==  ':')
	displaySpecified = true;

    if (strcmp (argv[i], "-screen") == 0)	/* -screen n WxHxD */
    {
	int screenNum;
	if (i + 2 >= argc) UseMsg();
	screenNum = atoi(argv[i+1]);
	if (screenNum < 0 || screenNum >= MAXSCREENS)
	{
	    ErrorF("Invalid screen number %d\n", screenNum);
	    UseMsg();
	}
	if (3 != sscanf(argv[i+2], "%dx%dx%d",
			&vfbScreens[screenNum].width,
			&vfbScreens[screenNum].height,
			&vfbScreens[screenNum].depth))
	{
	    ErrorF("Invalid screen configuration %s\n", argv[i+2]);
	    UseMsg();
	}

	if (screenNum >= vfbNumScreens)
	    vfbNumScreens = screenNum + 1;
	lastScreen = screenNum;
	return 3;
    }

    if (strcmp (argv[i], "-pixdepths") == 0)	/* -pixdepths list-of-depth */
    {
	int depth, ret = 1;

	if (++i >= argc) UseMsg();
	while ((i < argc) && (depth = atoi(argv[i++])) != 0)
	{
	    if (depth < 0 || depth > 32)
	    {
		ErrorF("Invalid pixmap depth %d\n", depth);
		UseMsg();
	    }
	    vfbPixmapDepths[depth] = TRUE;
	    ret++;
	}
	return ret;
    }
    
    if (strcmp (argv[i], "+render") == 0)	/* +render */
    {
	Render = TRUE;
	return 1;
    }
  
    if (strcmp (argv[i], "-render") == 0)	/* -render */
    {
	Render = FALSE;
	return 1;
    }

    if (strcmp (argv[i], "-blackpixel") == 0)	/* -blackpixel n */
    {
	Pixel pix;
	if (++i >= argc) UseMsg();
	pix = atoi(argv[i]);
	if (-1 == lastScreen)
	{
	    int i;
	    for (i = 0; i < MAXSCREENS; i++)
	    {
		vfbScreens[i].blackPixel = pix;
	    }
	}
	else
	{
	    vfbScreens[lastScreen].blackPixel = pix;
	}
	return 2;
    }

    if (strcmp (argv[i], "-whitepixel") == 0)	/* -whitepixel n */
    {
	Pixel pix;
	if (++i >= argc) UseMsg();
	pix = atoi(argv[i]);
	if (-1 == lastScreen)
	{
	    int i;
	    for (i = 0; i < MAXSCREENS; i++)
	    {
		vfbScreens[i].whitePixel = pix;
	    }
	}
	else
	{
	    vfbScreens[lastScreen].whitePixel = pix;
	}
	return 2;
    }
    
    if (strcmp (argv[i], "-linebias") == 0)	/* -linebias n */
    {
	unsigned int linebias;
	if (++i >= argc) UseMsg();
	linebias = atoi(argv[i]);
	if (-1 == lastScreen)
	{
	    int i;
	    for (i = 0; i < MAXSCREENS; i++)
	    {
		vfbScreens[i].lineBias = linebias;
	    }
	}
	else
	{
	    vfbScreens[lastScreen].lineBias = linebias;
	}
	return 2;
    }

#ifdef HAS_MMAP
    if (strcmp (argv[i], "-fbdir") == 0)	/* -fbdir directory */
    {
	if (++i >= argc) UseMsg();
	pfbdir = argv[i];
	fbmemtype = MMAPPED_FILE_FB;
	return 2;
    }
#endif /* HAS_MMAP */

#ifdef HAS_SHM
    if (strcmp (argv[i], "-shmem") == 0)	/* -shmem */
    {
	fbmemtype = SHARED_MEMORY_FB;
	return 1;
    }
#endif
    
    if (strcmp(argv[i], "-geometry") == 0)
    {
	if (++i >= argc) UseMsg();
	if (sscanf(argv[i],"%dx%d",&vfbScreens[0].width,
		   &vfbScreens[0].height) != 2) {
	    ErrorF("Invalid geometry %s\n", argv[i]);
	    UseMsg();
	}
	return 2;
    }
    
    if (strcmp(argv[i], "-depth") == 0)
    {
	if (++i >= argc) UseMsg();
	vfbScreens[0].depth = atoi(argv[i]);
	return 2;
    }

    if (strcmp(argv[i], "-pixelformat") == 0)
    {
	char rgbbgr[4];
	int bits1, bits2, bits3;
	if (++i >= argc) UseMsg();
	if (sscanf(argv[i], "%3s%1d%1d%1d", rgbbgr,&bits1,&bits2,&bits3) < 4) {
	    ErrorF("Invalid pixel format %s\n", argv[i]);
	    UseMsg();
	}

#define SET_PIXEL_FORMAT(vfbScreen)                     \
    (vfbScreen).pixelFormatDefined = TRUE;              \
    (vfbScreen).depth = bits1 + bits2 + bits3;          \
    (vfbScreen).greenBits = bits2;                      \
    if (strcasecmp(rgbbgr, "bgr") == 0) {               \
        (vfbScreen).rgbNotBgr = FALSE;                  \
        (vfbScreen).redBits = bits3;                    \
        (vfbScreen).blueBits = bits1;                   \
    } else if (strcasecmp(rgbbgr, "rgb") == 0) {        \
        (vfbScreen).rgbNotBgr = TRUE;                   \
        (vfbScreen).redBits = bits1;                    \
        (vfbScreen).blueBits = bits3;                   \
    } else {                                            \
        ErrorF("Invalid pixel format %s\n", argv[i]);   \
        UseMsg();                                       \
    }

	if (-1 == lastScreen)
	{
	    int i;
	    for (i = 0; i < MAXSCREENS; i++)
	    {
		SET_PIXEL_FORMAT(vfbScreens[i]);
	    }
	}
	else
	{
	    SET_PIXEL_FORMAT(vfbScreens[lastScreen]);
	}

	return 2;
    }

    if (strcmp(argv[i], "-inetd") == 0)
    {
	dup2(0,3);
	vncInetdSock = 3;
	close(2);
	
	if (!displaySpecified) {
	    int port = network::TcpSocket::getSockPort(vncInetdSock);
	    int displayNum = port - 5900;
	    if (displayNum < 0 || displayNum > 99 || !displayNumFree(displayNum)) {
		for (displayNum = 1; displayNum < 100; displayNum++)
		    if (displayNumFree(displayNum)) break;
		
		if (displayNum == 100)
		    FatalError("Xvnc error: no free display number for -inetd");
	    }
	    
	    display = displayNumStr;
	    sprintf(displayNumStr, "%d", displayNum);
	}
	
	return 1;
    }
    
    if (rfb::Configuration::setParam(argv[i]))
	return 1;
    
    if (argv[i][0] == '-' && i+1 < argc) {
	if (rfb::Configuration::setParam(&argv[i][1], argv[i+1]))
	    return 2;
    }
    
    return 0;
}

#ifdef DDXTIME /* from ServerOSDefines */
CARD32
GetTimeInMillis()
{
    struct timeval  tp;

    X_GETTIMEOFDAY(&tp);
    return(tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}
#endif

static ColormapPtr InstalledMaps[MAXSCREENS];

static int 
vfbListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps)
{
    /* By the time we are processing requests, we can guarantee that there
     * is always a colormap installed */
    *pmaps = InstalledMaps[pScreen->myNum]->mid;
    return (1);
}


static void 
vfbInstallColormap(ColormapPtr pmap)
{
    int index = pmap->pScreen->myNum;
    ColormapPtr oldpmap = InstalledMaps[index];

    if (pmap != oldpmap)
    {
	int entries;
	XWDFileHeader *pXWDHeader;
	XWDColor *pXWDCmap;
	VisualPtr pVisual;
	Pixel *     ppix;
	xrgb *      prgb;
	xColorItem *defs;
	int i;

	if(oldpmap != (ColormapPtr)None)
	    WalkTree(pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);
	/* Install pmap */
	InstalledMaps[index] = pmap;
	WalkTree(pmap->pScreen, TellGainedMap, (char *)&pmap->mid);

	entries = pmap->pVisual->ColormapEntries;
	pXWDHeader = vfbScreens[pmap->pScreen->myNum].pXWDHeader;
	pXWDCmap = vfbScreens[pmap->pScreen->myNum].pXWDCmap;
	pVisual = pmap->pVisual;

	swapcopy32(pXWDHeader->visual_class, pVisual->c_class);
	swapcopy32(pXWDHeader->red_mask, pVisual->redMask);
	swapcopy32(pXWDHeader->green_mask, pVisual->greenMask);
	swapcopy32(pXWDHeader->blue_mask, pVisual->blueMask);
	swapcopy32(pXWDHeader->bits_per_rgb, pVisual->bitsPerRGBValue);
	swapcopy32(pXWDHeader->colormap_entries, pVisual->ColormapEntries);

	ppix = (Pixel *)ALLOCATE_LOCAL(entries * sizeof(Pixel));
	prgb = (xrgb *)ALLOCATE_LOCAL(entries * sizeof(xrgb));
	defs = (xColorItem *)ALLOCATE_LOCAL(entries * sizeof(xColorItem));

	for (i = 0; i < entries; i++)  ppix[i] = i;
	/* XXX truecolor */
	QueryColors(pmap, entries, ppix, prgb);

	for (i = 0; i < entries; i++) { /* convert xrgbs to xColorItems */
	    defs[i].pixel = ppix[i] & 0xff; /* change pixel to index */
	    defs[i].red = prgb[i].red;
	    defs[i].green = prgb[i].green;
	    defs[i].blue = prgb[i].blue;
	    defs[i].flags =  DoRed|DoGreen|DoBlue;
	}
	(*pmap->pScreen->StoreColors)(pmap, entries, defs);
	
	DEALLOCATE_LOCAL(ppix);
	DEALLOCATE_LOCAL(prgb);
	DEALLOCATE_LOCAL(defs);
    }
}

static void
vfbUninstallColormap(ColormapPtr pmap)
{
    ColormapPtr curpmap = InstalledMaps[pmap->pScreen->myNum];

    if(pmap == curpmap)
    {
	if (pmap->mid != pmap->pScreen->defColormap)
	{
	    curpmap = (ColormapPtr) LookupIDByType(pmap->pScreen->defColormap,
						   RT_COLORMAP);
	    (*pmap->pScreen->InstallColormap)(curpmap);
	}
    }
}

static void
vfbStoreColors(ColormapPtr pmap, int ndef, xColorItem *pdefs)
{
    XWDColor *pXWDCmap;
    int i;

    if (pmap != InstalledMaps[pmap->pScreen->myNum])
    {
	return;
    }

    pXWDCmap = vfbScreens[pmap->pScreen->myNum].pXWDCmap;

    if ((pmap->pVisual->c_class | DynamicClass) == DirectColor)
    {
	return;
    }

    for (i = 0; i < ndef; i++)
    {
	if (pdefs[i].flags & DoRed)
	{
	    swapcopy16(pXWDCmap[pdefs[i].pixel].red, pdefs[i].red);
	}
	if (pdefs[i].flags & DoGreen)
	{
	    swapcopy16(pXWDCmap[pdefs[i].pixel].green, pdefs[i].green);
	}
	if (pdefs[i].flags & DoBlue)
	{
	    swapcopy16(pXWDCmap[pdefs[i].pixel].blue, pdefs[i].blue);
	}
    }
}

static Bool
vfbSaveScreen(ScreenPtr pScreen, int on)
{
    return TRUE;
}

#ifdef HAS_MMAP

/* this flushes any changes to the screens out to the mmapped file */
static void
vfbBlockHandler(pointer blockData, OSTimePtr pTimeout, pointer pReadmask)
{
    int i;

    for (i = 0; i < vfbNumScreens; i++)
    {
#ifdef MS_ASYNC
	if (-1 == msync((caddr_t)vfbScreens[i].pXWDHeader,
			(size_t)vfbScreens[i].sizeInBytes, MS_ASYNC))
#else
	/* silly NetBSD and who else? */
	if (-1 == msync((caddr_t)vfbScreens[i].pXWDHeader,
			(size_t)vfbScreens[i].sizeInBytes))
#endif
	{
	    perror("msync");
	    ErrorF("msync failed, errno %d", errno);
	}
    }
}


static void
vfbWakeupHandler(pointer blockData, int result, pointer pReadmask)
{
}


static void
vfbAllocateMmappedFramebuffer(vfbScreenInfoPtr pvfb)
{
#define DUMMY_BUFFER_SIZE 65536
    char dummyBuffer[DUMMY_BUFFER_SIZE];
    int currentFileSize, writeThisTime;

    sprintf(pvfb->mmap_file, "%s/Xvfb_screen%d", pfbdir, pvfb->scrnum);
    if (-1 == (pvfb->mmap_fd = open(pvfb->mmap_file, O_CREAT|O_RDWR, 0666)))
    {
	perror("open");
	ErrorF("open %s failed, errno %d", pvfb->mmap_file, errno);
	return;
    }

    /* Extend the file to be the proper size */

    bzero(dummyBuffer, DUMMY_BUFFER_SIZE);
    for (currentFileSize = 0;
	 currentFileSize < pvfb->sizeInBytes;
	 currentFileSize += writeThisTime)
    {
	writeThisTime = min(DUMMY_BUFFER_SIZE,
			    pvfb->sizeInBytes - currentFileSize);
	if (-1 == write(pvfb->mmap_fd, dummyBuffer, writeThisTime))
	{
	    perror("write");
	    ErrorF("write %s failed, errno %d", pvfb->mmap_file, errno);
	    return;
	}
    }

    /* try to mmap the file */

    pvfb->pXWDHeader = (XWDFileHeader *)mmap((caddr_t)NULL, pvfb->sizeInBytes,
				    PROT_READ|PROT_WRITE,
				    MAP_FILE|MAP_SHARED,
				    pvfb->mmap_fd, 0);
    if (-1 == (long)pvfb->pXWDHeader)
    {
	perror("mmap");
	ErrorF("mmap %s failed, errno %d", pvfb->mmap_file, errno);
	pvfb->pXWDHeader = NULL;
	return;
    }

    if (!RegisterBlockAndWakeupHandlers(vfbBlockHandler, vfbWakeupHandler,
					NULL))
    {
	pvfb->pXWDHeader = NULL;
    }
}
#endif /* HAS_MMAP */


#ifdef HAS_SHM
static void
vfbAllocateSharedMemoryFramebuffer(vfbScreenInfoPtr pvfb)
{
    /* create the shared memory segment */

    pvfb->shmid = shmget(IPC_PRIVATE, pvfb->sizeInBytes, IPC_CREAT|0777);
    if (pvfb->shmid < 0)
    {
	perror("shmget");
	ErrorF("shmget %d bytes failed, errno %d", pvfb->sizeInBytes, errno);
	return;
    }

    /* try to attach it */

    pvfb->pXWDHeader = (XWDFileHeader *)shmat(pvfb->shmid, 0, 0);
    if (-1 == (long)pvfb->pXWDHeader)
    {
	perror("shmat");
	ErrorF("shmat failed, errno %d", errno);
	pvfb->pXWDHeader = NULL; 
	return;
    }

    ErrorF("screen %d shmid %d\n", pvfb->scrnum, pvfb->shmid);
}
#endif /* HAS_SHM */


static char *
vfbAllocateFramebufferMemory(vfbScreenInfoPtr pvfb)
{
    if (pvfb->pfbMemory) return pvfb->pfbMemory; /* already done */

    pvfb->sizeInBytes = pvfb->paddedBytesWidth * pvfb->height;

    /* Calculate how many entries in colormap.  This is rather bogus, because
     * the visuals haven't even been set up yet, but we need to know because we
     * have to allocate space in the file for the colormap.  The number 10
     * below comes from the MAX_PSEUDO_DEPTH define in cfbcmap.c.
     */

    if (pvfb->depth <= 10)
    { /* single index colormaps */
	pvfb->ncolors = 1 << pvfb->depth;
    }
    else
    { /* decomposed colormaps */
	int nplanes_per_color_component = pvfb->depth / 3;
	if (pvfb->depth % 3) nplanes_per_color_component++;
	pvfb->ncolors = 1 << nplanes_per_color_component;
    }

    /* add extra bytes for XWDFileHeader, window name, and colormap */

    pvfb->sizeInBytes += SIZEOF(XWDheader) + XWD_WINDOW_NAME_LEN +
		    pvfb->ncolors * SIZEOF(XWDColor);

    pvfb->pXWDHeader = NULL; 
    switch (fbmemtype)
    {
#ifdef HAS_MMAP
    case MMAPPED_FILE_FB:  vfbAllocateMmappedFramebuffer(pvfb); break;
#else
    case MMAPPED_FILE_FB: break;
#endif

#ifdef HAS_SHM
    case SHARED_MEMORY_FB: vfbAllocateSharedMemoryFramebuffer(pvfb); break;
#else
    case SHARED_MEMORY_FB: break;
#endif

    case NORMAL_MEMORY_FB:
	pvfb->pXWDHeader = (XWDFileHeader *)Xalloc(pvfb->sizeInBytes);
	break;
    }

    if (pvfb->pXWDHeader)
    {
	pvfb->pXWDCmap = (XWDColor *)((char *)pvfb->pXWDHeader
				+ SIZEOF(XWDheader) + XWD_WINDOW_NAME_LEN);
	pvfb->pfbMemory = (char *)(pvfb->pXWDCmap + pvfb->ncolors);

	return pvfb->pfbMemory;
    }
    else
	return NULL;
}

static void
vfbWriteXWDFileHeader(ScreenPtr pScreen)
{
    vfbScreenInfoPtr pvfb = &vfbScreens[pScreen->myNum];
    XWDFileHeader *pXWDHeader = pvfb->pXWDHeader;
    char hostname[XWD_WINDOW_NAME_LEN];
    unsigned long swaptest = 1;
    int i;

    needswap = *(char *) &swaptest;

    pXWDHeader->header_size = (char *)pvfb->pXWDCmap - (char *)pvfb->pXWDHeader;
    pXWDHeader->file_version = XWD_FILE_VERSION;

    pXWDHeader->pixmap_format = ZPixmap;
    pXWDHeader->pixmap_depth = pvfb->depth;
    pXWDHeader->pixmap_height = pXWDHeader->window_height = pvfb->height;
    pXWDHeader->xoffset = 0;
    pXWDHeader->byte_order = IMAGE_BYTE_ORDER;
    pXWDHeader->bitmap_bit_order = BITMAP_BIT_ORDER;
#ifndef INTERNAL_VS_EXTERNAL_PADDING
    pXWDHeader->pixmap_width = pXWDHeader->window_width = pvfb->width;
    pXWDHeader->bitmap_unit = BITMAP_SCANLINE_UNIT;
    pXWDHeader->bitmap_pad = BITMAP_SCANLINE_PAD;
#else
    pXWDHeader->pixmap_width = pXWDHeader->window_width = pvfb->paddedWidth;
    pXWDHeader->bitmap_unit = BITMAP_SCANLINE_UNIT_PROTO;
    pXWDHeader->bitmap_pad = BITMAP_SCANLINE_PAD_PROTO;
#endif
    pXWDHeader->bits_per_pixel = pvfb->bitsPerPixel;
    pXWDHeader->bytes_per_line = pvfb->paddedBytesWidth;
    pXWDHeader->ncolors = pvfb->ncolors;

    /* visual related fields are written when colormap is installed */

    pXWDHeader->window_x = pXWDHeader->window_y = 0;
    pXWDHeader->window_bdrwidth = 0;

    /* write xwd "window" name: Xvfb hostname:server.screen */

    if (-1 == gethostname(hostname, sizeof(hostname)))
	hostname[0] = 0;
    else
	hostname[XWD_WINDOW_NAME_LEN-1] = 0;
    sprintf((char *)(pXWDHeader+1), "Xvfb %s:%s.%d", hostname, display,
	    pScreen->myNum);

    /* write colormap pixel slot values */

    for (i = 0; i < pvfb->ncolors; i++)
    {
	pvfb->pXWDCmap[i].pixel = i;
    }

    /* byte swap to most significant byte first */

    if (needswap)
    {
	SwapLongs((CARD32 *)pXWDHeader, SIZEOF(XWDheader)/4);
	for (i = 0; i < pvfb->ncolors; i++)
	{
	    register char n;
	    swapl(&pvfb->pXWDCmap[i].pixel, n);
	}
    }
}


static Bool
vfbCursorOffScreen (ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
vfbCrossScreen (ScreenPtr pScreen, Bool entering)
{
}

static Bool vfbRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static Bool vfbUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static void vfbSetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y) 
{
}

static void vfbMoveCursor(ScreenPtr pScreen, int x, int y) 
{
}

static miPointerSpriteFuncRec vfbPointerSpriteFuncs = {
    vfbRealizeCursor,
    vfbUnrealizeCursor,
    vfbSetCursor,
    vfbMoveCursor
};

static miPointerScreenFuncRec vfbPointerCursorFuncs = {
    vfbCursorOffScreen,
    vfbCrossScreen,
    miPointerWarpCursor
};

static Bool
vfbCloseScreen(int index, ScreenPtr pScreen)
{
    vfbScreenInfoPtr pvfb = &vfbScreens[index];
    int i;
 
    pScreen->CloseScreen = pvfb->closeScreen;

    /*
     * XXX probably lots of stuff to clean.  For now,
     * clear InstalledMaps[] so that server reset works correctly.
     */
    for (i = 0; i < MAXSCREENS; i++)
	InstalledMaps[i] = NULL;

    return pScreen->CloseScreen(index, pScreen);
}

static Bool
vfbScreenInit(int index, ScreenPtr pScreen, int argc, char **argv)
{
    vfbScreenInfoPtr pvfb = &vfbScreens[index];
    int dpix = 100, dpiy = 100;
    int ret;
    char *pbits;

    pvfb->paddedBytesWidth = PixmapBytePad(pvfb->width, pvfb->depth);
    pvfb->bitsPerPixel = vfbBitsPerPixel(pvfb->depth);
    if (pvfb->bitsPerPixel >= 8 )
	pvfb->paddedWidth = pvfb->paddedBytesWidth / (pvfb->bitsPerPixel / 8);
    else
	pvfb->paddedWidth = pvfb->paddedBytesWidth * 8;
    pbits = vfbAllocateFramebufferMemory(pvfb);
    if (!pbits) return FALSE;

    miSetPixmapDepths ();

    switch (pvfb->depth) {
    case 8:
	miSetVisualTypesAndMasks (8,
				  ((1 << StaticGray) |
				   (1 << GrayScale) |
				   (1 << StaticColor) |
				   (1 << PseudoColor) |
				   (1 << TrueColor) |
				   (1 << DirectColor)),
				  8, PseudoColor, 0x07, 0x38, 0xc0);
	break;
    case 15:
	miSetVisualTypesAndMasks (15,
				  ((1 << TrueColor) |
				   (1 << DirectColor)),
				  8, TrueColor, 0x7c00, 0x03e0, 0x001f);
	break;
    case 16:
	miSetVisualTypesAndMasks (16,
				  ((1 << TrueColor) |
				   (1 << DirectColor)),
				  8, TrueColor, 0xf800, 0x07e0, 0x001f);
	break;
    case 24:
	miSetVisualTypesAndMasks (24,
				  ((1 << TrueColor) |
				   (1 << DirectColor)),
				  8, TrueColor, 0xff0000, 0x00ff00, 0x0000ff);
	break;
    }
	
    ret = fbScreenInit(pScreen, pbits, pvfb->width, pvfb->height,
		       dpix, dpiy, pvfb->paddedWidth,pvfb->bitsPerPixel);
#ifdef RENDER
    if (ret && Render) 
	fbPictureInit (pScreen, 0, 0);
#endif

    if (!ret) return FALSE;

    /* miInitializeBackingStore(pScreen); */

    /*
     * Circumvent the backing store that was just initialised.  This amounts
     * to a truely bizarre way of initialising SaveDoomedAreas and friends.
     */

    pScreen->InstallColormap = vfbInstallColormap;
    pScreen->UninstallColormap = vfbUninstallColormap;
    pScreen->ListInstalledColormaps = vfbListInstalledColormaps;

    pScreen->SaveScreen = vfbSaveScreen;
    pScreen->StoreColors = vfbStoreColors;

    miDCInitialize(pScreen, &vfbPointerCursorFuncs);

    vfbWriteXWDFileHeader(pScreen);

    pScreen->blackPixel = pvfb->blackPixel;
    pScreen->whitePixel = pvfb->whitePixel;

    ret = fbCreateDefColormap(pScreen);

    miSetZeroLineBias(pScreen, pvfb->lineBias);

    pvfb->closeScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = vfbCloseScreen;

    return ret;

} /* end vfbScreenInit */


static void vfbClientStateChange(CallbackListPtr*, pointer, pointer) {
  dispatchException &= ~DE_RESET;
}

void
InitOutput(ScreenInfo *screenInfo, int argc, char **argv)
{
    ErrorF("\nXvnc version %s - built %s\n", XVNCVERSION, buildtime);
    ErrorF("Underlying X server release %d, %s\n\n", VENDOR_RELEASE,
	   VENDOR_STRING);
    int i;
    int NumFormats = 0;

    /* initialize pixmap formats */

    /* must have a pixmap depth to match every screen depth */
    for (i = 0; i < vfbNumScreens; i++)
    {
	vfbPixmapDepths[vfbScreens[i].depth] = TRUE;
    }

    /* RENDER needs a good set of pixmaps. */
    if (Render) {
	vfbPixmapDepths[1] = TRUE;
	vfbPixmapDepths[4] = TRUE;
	vfbPixmapDepths[8] = TRUE;
/*	vfbPixmapDepths[15] = TRUE; */
	vfbPixmapDepths[16] = TRUE;
	vfbPixmapDepths[24] = TRUE;
	vfbPixmapDepths[32] = TRUE;
    }

    for (i = 1; i <= 32; i++)
    {
	if (vfbPixmapDepths[i])
	{
	    if (NumFormats >= MAXFORMATS)
		FatalError ("MAXFORMATS is too small for this server\n");
	    screenInfo->formats[NumFormats].depth = i;
	    screenInfo->formats[NumFormats].bitsPerPixel = vfbBitsPerPixel(i);
	    screenInfo->formats[NumFormats].scanlinePad = BITMAP_SCANLINE_PAD;
	    NumFormats++;
	}
    }

    screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
    screenInfo->numPixmapFormats = NumFormats;

    /* initialize screens */

    for (i = 0; i < vfbNumScreens; i++)
    {
	if (-1 == AddScreen(vfbScreenInit, argc, argv))
	{
	    FatalError("Couldn't add screen %d", i);
	}
    }

    if (!AddCallback(&ClientStateCallback, vfbClientStateChange, 0)) {
	FatalError("AddCallback failed\n");
    }
} /* end InitOutput */

#ifdef DPMSExtension
extern "C" {
#if NeedFunctionPrototypes
  void DPMSSet(CARD16 level)
#else
    void DPMSSet(level)
    CARD16 level;
#endif
  {
    return;
  }

  Bool DPMSSupported()
  {
    return FALSE;
  }
}
#endif

/* this is just to get the server to link on AIX */
#ifdef AIXV3
int SelectWaitTime = 10000; /* usec */
#endif

Bool LegalModifier(unsigned int key, DevicePtr pDev)
{
  return TRUE;
}

void ProcessInputEvents()
{
  mieqProcessInputEvents();
  miPointerUpdate();
}

/* Fairly standard US PC Keyboard */

#define VFB_MIN_KEY 8
#define VFB_MAX_KEY 255
#define VFB_MAP_LEN (VFB_MAX_KEY - VFB_MIN_KEY + 1)
#define KEYSYMS_PER_KEY 2
KeySym keyboardMap[VFB_MAP_LEN * KEYSYMS_PER_KEY] = {
  NoSymbol, NoSymbol,
  XK_Escape, NoSymbol,
  XK_1, XK_exclam,
  XK_2, XK_at,
  XK_3, XK_numbersign,
  XK_4, XK_dollar,
  XK_5, XK_percent,
  XK_6, XK_asciicircum,
  XK_7, XK_ampersand,
  XK_8, XK_asterisk,
  XK_9, XK_parenleft,
  XK_0, XK_parenright,
  XK_minus, XK_underscore,
  XK_equal, XK_plus,
  XK_BackSpace, NoSymbol,
  XK_Tab, NoSymbol,
  XK_q, XK_Q,
  XK_w, XK_W,
  XK_e, XK_E,
  XK_r, XK_R,
  XK_t, XK_T,
  XK_y, XK_Y,
  XK_u, XK_U,
  XK_i, XK_I,
  XK_o, XK_O,
  XK_p, XK_P,
  XK_bracketleft, XK_braceleft,
  XK_bracketright, XK_braceright,
  XK_Return, NoSymbol,
  XK_Control_L, NoSymbol,
  XK_a, XK_A,
  XK_s, XK_S,
  XK_d, XK_D,
  XK_f, XK_F,
  XK_g, XK_G,
  XK_h, XK_H,
  XK_j, XK_J,
  XK_k, XK_K,
  XK_l, XK_L,
  XK_semicolon, XK_colon,
  XK_apostrophe, XK_quotedbl,
  XK_grave, XK_asciitilde,
  XK_Shift_L, NoSymbol,
  XK_backslash, XK_bar,
  XK_z, XK_Z,
  XK_x, XK_X,
  XK_c, XK_C,
  XK_v, XK_V,
  XK_b, XK_B,
  XK_n, XK_N,
  XK_m, XK_M,
  XK_comma, XK_less,
  XK_period, XK_greater,
  XK_slash, XK_question,
  XK_Shift_R, NoSymbol,
  XK_KP_Multiply, NoSymbol,
  XK_Alt_L, XK_Meta_L,
  XK_space, NoSymbol,
  /*XK_Caps_Lock*/ NoSymbol, NoSymbol,
  XK_F1, NoSymbol,
  XK_F2, NoSymbol,
  XK_F3, NoSymbol,
  XK_F4, NoSymbol,
  XK_F5, NoSymbol,
  XK_F6, NoSymbol,
  XK_F7, NoSymbol,
  XK_F8, NoSymbol,
  XK_F9, NoSymbol,
  XK_F10, NoSymbol,
  XK_Num_Lock, XK_Pointer_EnableKeys,
  XK_Scroll_Lock, NoSymbol,
  XK_KP_Home, XK_KP_7,
  XK_KP_Up, XK_KP_8,
  XK_KP_Prior, XK_KP_9,
  XK_KP_Subtract, NoSymbol,
  XK_KP_Left, XK_KP_4,
  XK_KP_Begin, XK_KP_5,
  XK_KP_Right, XK_KP_6,
  XK_KP_Add, NoSymbol,
  XK_KP_End, XK_KP_1,
  XK_KP_Down, XK_KP_2,
  XK_KP_Next, XK_KP_3,
  XK_KP_Insert, XK_KP_0,
  XK_KP_Delete, XK_KP_Decimal,
  NoSymbol, NoSymbol,
  NoSymbol, NoSymbol,
  NoSymbol, NoSymbol,
  XK_F11, NoSymbol,
  XK_F12, NoSymbol,
  XK_Home, NoSymbol,
  XK_Up, NoSymbol,
  XK_Prior, NoSymbol,
  XK_Left, NoSymbol,
  NoSymbol, NoSymbol,
  XK_Right, NoSymbol,
  XK_End, NoSymbol,
  XK_Down, NoSymbol,
  XK_Next, NoSymbol,
  XK_Insert, NoSymbol,
  XK_Delete, NoSymbol,
  XK_KP_Enter, NoSymbol,
  XK_Control_R, NoSymbol,
  XK_Pause, XK_Break,
  XK_Print, XK_Execute,
  XK_KP_Divide, NoSymbol,
  XK_Alt_R, XK_Meta_R,
};

static Bool GetMappings(KeySymsPtr pKeySyms, CARD8 *pModMap)
{
  int i;

  for (i = 0; i < MAP_LENGTH; i++)
    pModMap[i] = NoSymbol;

  for (i = 0; i < VFB_MAP_LEN; i++) {
    if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Caps_Lock)
      pModMap[i + VFB_MIN_KEY] = LockMask;
    else if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Shift_L ||
             keyboardMap[i * KEYSYMS_PER_KEY] == XK_Shift_R)
      pModMap[i + VFB_MIN_KEY] = ShiftMask;
    else if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Control_L ||
             keyboardMap[i * KEYSYMS_PER_KEY] == XK_Control_R) {
      pModMap[i + VFB_MIN_KEY] = ControlMask;
    }
    else if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Alt_L ||
             keyboardMap[i * KEYSYMS_PER_KEY] == XK_Alt_R)
      pModMap[i + VFB_MIN_KEY] = Mod1Mask;
  }

  pKeySyms->minKeyCode = VFB_MIN_KEY;
  pKeySyms->maxKeyCode = VFB_MAX_KEY;
  pKeySyms->mapWidth = KEYSYMS_PER_KEY;
  pKeySyms->map = keyboardMap;

  return TRUE;
}

static void vfbBell(int percent, DeviceIntPtr device, pointer ctrl, int class_)
{
  if (percent > 0)
    vncBell();
}

static int vfbKeybdProc(DeviceIntPtr pDevice, int onoff)
{
  KeySymsRec		keySyms;
  CARD8 		modMap[MAP_LENGTH];
  DevicePtr pDev = (DevicePtr)pDevice;

  switch (onoff)
  {
  case DEVICE_INIT: 
    GetMappings(&keySyms, modMap);
    InitKeyboardDeviceStruct(pDev, &keySyms, modMap,
                             (BellProcPtr)vfbBell, (KbdCtrlProcPtr)NoopDDA);
    break;
  case DEVICE_ON: 
    pDev->on = TRUE;
    break;
  case DEVICE_OFF: 
    pDev->on = FALSE;
    break;
  case DEVICE_CLOSE:
    break;
  }
  return Success;
}

static int vfbMouseProc(DeviceIntPtr pDevice, int onoff)
{
  BYTE map[6];
  DevicePtr pDev = (DevicePtr)pDevice;

  switch (onoff)
  {
  case DEVICE_INIT:
    map[1] = 1;
    map[2] = 2;
    map[3] = 3;
    map[4] = 4;
    map[5] = 5;
    InitPointerDeviceStruct(pDev, map, 5, miPointerGetMotionEvents,
                            (PtrCtrlProcPtr)NoopDDA, miPointerGetMotionBufferSize());
    break;

  case DEVICE_ON:
    pDev->on = TRUE;
    break;

  case DEVICE_OFF:
    pDev->on = FALSE;
    break;

  case DEVICE_CLOSE:
    break;
  }
  return Success;
}

// InitInput is called after InitExtensions, so we're guaranteed that
// vncExtensionInit() has already been called.

void InitInput(int argc, char *argv[])
{
  DeviceIntPtr p, k;
  p = AddInputDevice(vfbMouseProc, TRUE);
  k = AddInputDevice(vfbKeybdProc, TRUE);
  RegisterPointerDevice(p);
  RegisterKeyboardDevice(k);
  miRegisterPointerDevice(screenInfo.screens[0], p);
  (void)mieqInit ((DevicePtr)k, (DevicePtr)p);
}
