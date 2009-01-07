/* Copyright (c) 1993  X Consortium
   Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.

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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <rfb/Configuration.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <network/TcpSocket.h>
#include "vncExtInit.h"

extern "C" {
#define class c_class
#define public c_public
#ifdef WIN32
#include <X11/Xwinsock.h>
#endif
#include <stdio.h>
#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#include <X11/Xos.h>
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
#include <X11/keysym.h>
  extern char buildtime[];
#undef class
#undef public
}

#define XVNCVERSION "TightVNC 1.5 series"
#define XVNCCOPYRIGHT ("Copyright (C) 2002-2005 RealVNC Ltd.\n" \
		       "Copyright (C) 2000-2006 Constantin Kaplinsky\n" \
		       "Copyright (C) 2004-2006 Peter Astrand, Cendio AB\n" \
                       "See http://www.tightvnc.com for information on TightVNC.\n")


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

  void ddxGiveUp()
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
    ErrorF("\nXvnc %s - built %s\n%s", XVNCVERSION, buildtime, XVNCCOPYRIGHT);
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

	ppix = (Pixel *)xalloc(entries * sizeof(Pixel));
	prgb = (xrgb *)xalloc(entries * sizeof(xrgb));
	defs = (xColorItem *)xalloc(entries * sizeof(xColorItem));

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
	
	xfree(ppix);
	xfree(prgb);
	xfree(defs);
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

static Bool vfbRealizeCursor(
#ifdef XORG_16
			     DeviceIntPtr pDev,
#endif
			     ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static Bool vfbUnrealizeCursor(
#ifdef XORG_16
			       DeviceIntPtr pDev,
#endif
			       ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static void vfbSetCursor(
#ifdef XORG_16
			 DeviceIntPtr pDev,
#endif
			 ScreenPtr pScreen, CursorPtr pCursor, int x, int y) 
{
}

static void vfbMoveCursor(
#ifdef XORG_16
			  DeviceIntPtr pDev,
#endif
			  ScreenPtr pScreen, int x, int y) 
{
}

#ifdef XORG_16
static Bool
vfbDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{   
    return TRUE;
}

static void
vfbDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{ 
}
#endif

static miPointerSpriteFuncRec vfbPointerSpriteFuncs = {
    vfbRealizeCursor,
    vfbUnrealizeCursor,
    vfbSetCursor,
    vfbMoveCursor
#ifdef XORG_16
    , vfbDeviceCursorInitialize,
    vfbDeviceCursorCleanup
#endif
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
    int dpi = 100;
    int ret;
    char *pbits;

    if (monitorResolution) dpi = monitorResolution;

    pvfb->paddedBytesWidth = PixmapBytePad(pvfb->width, pvfb->depth);
    pvfb->bitsPerPixel = vfbBitsPerPixel(pvfb->depth);
    pvfb->paddedWidth = pvfb->paddedBytesWidth * 8 / pvfb->bitsPerPixel;
    pbits = vfbAllocateFramebufferMemory(pvfb);
    if (!pbits) return FALSE;
    vncFbptr[index] = pbits;

    miSetPixmapDepths();

    switch (pvfb->depth) {
    case 8:
	miSetVisualTypesAndMasks (8,
				  ((1 << StaticGray) |
				  (1 << GrayScale) |
				  (1 << StaticColor) |
				  (1 << PseudoColor) |
				  (1 << TrueColor) |
				  (1 << DirectColor)),
				  8, PseudoColor, 0, 0, 0);
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
    case 32:
	miSetVisualTypesAndMasks (32,
				  ((1 << TrueColor) |
				  (1 << DirectColor)),
				  8, TrueColor, 0xff000000, 0x00ff0000, 0x0000ff00);
	break;
    default:
	return FALSE;
    }

    ret = fbScreenInit(pScreen, pbits, pvfb->width, pvfb->height,
		       dpi, dpi, pvfb->paddedWidth, pvfb->bitsPerPixel);
  
#ifdef RENDER
    if (ret && Render) 
	ret = fbPictureInit (pScreen, 0, 0);
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
    
    miPointerInitialize(pScreen, &vfbPointerSpriteFuncs, &vfbPointerCursorFuncs,
			FALSE);
    
    vfbWriteXWDFileHeader(pScreen);

    pScreen->blackPixel = pvfb->blackPixel;
    pScreen->whitePixel = pvfb->whitePixel;

    if (!pvfb->pixelFormatDefined && pvfb->depth == 16) {
	pvfb->pixelFormatDefined = TRUE;
	pvfb->rgbNotBgr = TRUE;
	pvfb->blueBits = pvfb->redBits = 5;
	pvfb->greenBits = 6;
    }
    
    if (pvfb->pixelFormatDefined) {
	VisualPtr vis = pScreen->visuals;
	for (int i = 0; i < pScreen->numVisuals; i++) {
	    if (pvfb->rgbNotBgr) {
		vis->offsetBlue = 0;
		vis->blueMask = (1 << pvfb->blueBits) - 1;
		vis->offsetGreen = pvfb->blueBits;
		vis->greenMask = ((1 << pvfb->greenBits) - 1) << vis->offsetGreen;
		vis->offsetRed = vis->offsetGreen + pvfb->greenBits;
		vis->redMask = ((1 << pvfb->redBits) - 1) << vis->offsetRed;
	    } else {
		vis->offsetRed = 0;
		vis->redMask = (1 << pvfb->redBits) - 1;
		vis->offsetGreen = pvfb->redBits;
		vis->greenMask = ((1 << pvfb->greenBits) - 1) << vis->offsetGreen;
		vis->offsetBlue = vis->offsetGreen + pvfb->greenBits;
		vis->blueMask = ((1 << pvfb->blueBits) - 1) << vis->offsetBlue;
	    }
	    vis++;
	}
    }
    
    ret = fbCreateDefColormap(pScreen);

    miSetZeroLineBias(pScreen, pvfb->lineBias);

    pvfb->closeScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = vfbCloseScreen;

#ifndef NO_INIT_BACKING_STORE
  miInitializeBackingStore(pScreen);
  pScreen->backingStoreSupport = Always;
#endif

  return ret;

} /* end vfbScreenInit */


static void vfbClientStateChange(CallbackListPtr*, pointer, pointer) {
  dispatchException &= ~DE_RESET;
}

void
InitOutput(ScreenInfo *screenInfo, int argc, char **argv)
{
  ErrorF("\nXvnc %s - built %s\n%s", XVNCVERSION, buildtime, XVNCCOPYRIGHT);
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

/* this is just to get the server to link on AIX */
#ifdef AIXV3
int SelectWaitTime = 10000; /* usec */
#endif

void DDXRingBell(int percent, int pitch, int duration)
{
  if (percent > 0)
    vncBell();
}

Bool LegalModifier(unsigned int key, DeviceIntPtr pDev)
{
  return TRUE;
}

void ProcessInputEvents()
{
  mieqProcessInputEvents();
#ifdef XORG_15
  miPointerUpdate();
#endif
}

// InitInput is called after InitExtensions, so we're guaranteed that
// vncExtensionInit() has already been called.

void InitInput(int argc, char *argv[])
{
  mieqInit ();
}
