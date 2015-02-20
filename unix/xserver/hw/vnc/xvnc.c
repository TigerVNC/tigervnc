/* Copyright (c) 1993  X Consortium
   Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
   Copyright 2009-2015 Pierre Ossman for Cendio AB

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

#include "vncExtInit.h"
#include "RFBGlue.h"
#include "XorgGlue.h"
#include "xorg-version.h"

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
#if XORG < 114
#include "mibstore.h"
#endif
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "micmap.h"
#include <sys/types.h>
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
#include "os.h"
#include "miline.h"
#include "inputstr.h"
#ifdef RANDR
#include "randrstr.h"
#endif /* RANDR */
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#include <X11/keysym.h>
  extern char buildtime[];
#if XORG >= 17
#undef VENDOR_RELEASE
#undef VENDOR_STRING
#include "version-config.h"
#endif
#include "site.h"

#if XORG >= 110
#define Xalloc malloc
#define Xfree free
#endif

#define XVNCVERSION "TigerVNC 1.4.80"
#define XVNCCOPYRIGHT ("Copyright (C) 1999-2015 TigerVNC Team and many others (see README.txt)\n" \
                       "See http://www.tigervnc.org for information on TigerVNC.\n")

#define VFB_DEFAULT_WIDTH  1024
#define VFB_DEFAULT_HEIGHT 768
#define VFB_DEFAULT_DEPTH  24
#define VFB_DEFAULT_WHITEPIXEL 0xffffffff
#define VFB_DEFAULT_BLACKPIXEL 0
#define VFB_DEFAULT_LINEBIAS 0
#define XWD_WINDOW_NAME_LEN 60

typedef struct
{
    int width;
    int height;

    int depth;

    /* Computed when allocated */

    int paddedBytesWidth;
    int paddedWidth;

    int bitsPerPixel;

    /* Private */

    int sizeInBytes;

    void *pfbMemory;

#ifdef HAS_SHM
    int shmid;
#endif
} vfbFramebufferInfo, *vfbFramebufferInfoPtr;

typedef struct
{
    int scrnum;

    Pixel blackPixel;
    Pixel whitePixel;

    unsigned int lineBias;

    CloseScreenProcPtr closeScreen;

    vfbFramebufferInfo fb;

    Bool pixelFormatDefined;
    Bool rgbNotBgr;
    int redBits, greenBits, blueBits;
} vfbScreenInfo, *vfbScreenInfoPtr;

static int vfbNumScreens;
static vfbScreenInfo vfbScreens[MAXSCREENS];
static Bool vfbPixmapDepths[33];
typedef enum { NORMAL_MEMORY_FB, SHARED_MEMORY_FB } fbMemType;
static fbMemType fbmemtype = NORMAL_MEMORY_FB;
static int lastScreen = -1;
static Bool Render = TRUE;

static Bool displaySpecified = FALSE;
static char displayNumStr[16];

static int vncVerbose = DEFAULT_LOG_VERBOSITY;


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
	vfbScreens[i].blackPixel = VFB_DEFAULT_BLACKPIXEL;
	vfbScreens[i].whitePixel = VFB_DEFAULT_WHITEPIXEL;
	vfbScreens[i].lineBias = VFB_DEFAULT_LINEBIAS;
	vfbScreens[i].fb.width  = VFB_DEFAULT_WIDTH;
	vfbScreens[i].fb.height = VFB_DEFAULT_HEIGHT;
	vfbScreens[i].fb.pfbMemory = NULL;
	vfbScreens[i].fb.depth  = VFB_DEFAULT_DEPTH;
	vfbScreens[i].pixelFormatDefined = FALSE;
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

static void vfbFreeFramebufferMemory(vfbFramebufferInfoPtr pfb);

#ifdef DPMSExtension
    /* Why support DPMS? Because stupid modern desktop environments
       such as Unity 2D on Ubuntu 11.10 crashes if DPMS is not
       available. (DPMSSet is called by dpms.c, but the return value
       is ignored.) */
int DPMSSet(ClientPtr client, int level)
{
    return Success;
}

Bool DPMSSupported(void)
{
    /* Causes DPMSCapable to return false, meaning no devices are DPMS
       capable */
    return FALSE;
}
#endif

#if XORG < 111
void ddxGiveUp()
#else
void ddxGiveUp(enum ExitCode error)
#endif
{
    int i;

    /* clean up the framebuffers */
    for (i = 0; i < vfbNumScreens; i++)
        vfbFreeFramebufferMemory(&vfbScreens[i].fb);
}

void
#if XORG < 111
AbortDDX()
#else
AbortDDX(enum ExitCode error)
#endif
{
#if XORG < 111
    ddxGiveUp();
#else
    ddxGiveUp(error);
#endif
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
OsVendorInit(void)
{
}

void
#if XORG < 113
OsVendorFatalError()
#else
OsVendorFatalError(const char *f, va_list args)
#endif
{
}

#ifdef DDXBEFORERESET
void ddxBeforeReset(void)
{
    return;
}
#endif

void ddxUseMsg(void)
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

#ifdef HAS_SHM
    ErrorF("-shmem                 put framebuffers in shared memory\n");
#endif

    ErrorF("-geometry WxH          set screen 0's width, height\n");
    ErrorF("-depth D               set screen 0's depth\n");
    ErrorF("-pixelformat fmt       set pixel format (rgbNNN or bgrNNN)\n");
    ErrorF("-inetd                 has been launched from inetd\n");
    ErrorF("-noclipboard           disable clipboard settings modification via vncconfig utility\n");
    ErrorF("-verbose [n]           verbose startup messages\n");
    ErrorF("-quiet                 minimal startup messages\n");
    ErrorF("\nVNC parameters:\n");

    fprintf(stderr,"\n"
            "Parameters can be turned on with -<param> or off with -<param>=0\n"
            "Parameters which take a value can be specified as "
            "-<param> <value>\n"
            "Other valid forms are <param>=<value> -<param>=<value> "
            "--<param>=<value>\n"
            "Parameter names are case-insensitive.  The parameters are:\n\n");
    vncListParams(79, 14);
}

static 
Bool displayNumFree(int num)
{
    char file[256];
    if (vncIsTCPPortUsed(6000+num))
        return FALSE;
    sprintf(file, "/tmp/.X%d-lock", num);
    if (access(file, F_OK) == 0)
        return FALSE;
    sprintf(file, "/tmp/.X11-unix/X%d", num);
    if (access(file, F_OK) == 0)
        return FALSE;
    sprintf(file, "/usr/spool/sockets/X11/%d", num);
    if (access(file, F_OK) == 0)
        return FALSE;
    return TRUE;
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
	vncInitRFB();
    }

    if (argv[i][0] ==  ':')
        displaySpecified = TRUE;

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
			&vfbScreens[screenNum].fb.width,
			&vfbScreens[screenNum].fb.height,
			&vfbScreens[screenNum].fb.depth))
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
	    int j;
	    for (j = 0; j < MAXSCREENS; j++)
	    {
		vfbScreens[j].blackPixel = pix;
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
	    int j;
	    for (j = 0; j < MAXSCREENS; j++)
	    {
		vfbScreens[j].whitePixel = pix;
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
	    int j;
	    for (j = 0; j < MAXSCREENS; j++)
	    {
		vfbScreens[j].lineBias = linebias;
	    }
	}
	else
	{
	    vfbScreens[lastScreen].lineBias = linebias;
	}
	return 2;
    }

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
	if (sscanf(argv[i],"%dx%d",&vfbScreens[0].fb.width,
		   &vfbScreens[0].fb.height) != 2) {
	    ErrorF("Invalid geometry %s\n", argv[i]);
	    UseMsg();
	}
	return 2;
    }
    
    if (strcmp(argv[i], "-depth") == 0)
    {
	if (++i >= argc) UseMsg();
	vfbScreens[0].fb.depth = atoi(argv[i]);
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
    (vfbScreen).fb.depth = bits1 + bits2 + bits3;          \
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
	    int j;
	    for (j = 0; j < MAXSCREENS; j++)
	    {
		SET_PIXEL_FORMAT(vfbScreens[j]);
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
	    int port = vncGetSocketPort(vncInetdSock);
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

    if (strcmp(argv[i], "-noclipboard") == 0) {
	vncNoClipboard = 1;
	return 1;
    }

    if (!strcmp(argv[i], "-verbose")) {
        if (++i < argc && argv[i]) {
            char *end;
            long val;

            val = strtol(argv[i], &end, 0);
            if (*end == '\0') {
                vncVerbose = val;
                LogSetParameter(XLOG_VERBOSITY, vncVerbose);
                return 2;
            }
        }
        vncVerbose++;
        LogSetParameter(XLOG_VERBOSITY, vncVerbose);
        return 1;
    }

    if (!strcmp(argv[i], "-quiet")) {
        vncVerbose = -1;
        LogSetParameter(XLOG_VERBOSITY, vncVerbose);
        return 1;
    }

    if (vncSetParamSimple(argv[i]))
	return 1;
    
    if (argv[i][0] == '-' && i+1 < argc) {
	if (vncSetParam(&argv[i][1], argv[i+1]))
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

#if XORG < 113
static ColormapPtr InstalledMaps[MAXSCREENS];
#else
static DevPrivateKeyRec cmapScrPrivateKeyRec;
#define cmapScrPrivateKey (&cmapScrPrivateKeyRec)
#define GetInstalledColormap(s) ((ColormapPtr) dixLookupPrivate(&(s)->devPrivates, cmapScrPrivateKey))
#define SetInstalledColormap(s,c) (dixSetPrivate(&(s)->devPrivates, cmapScrPrivateKey, c))
#endif

static int 
vfbListInstalledColormaps(ScreenPtr pScreen, Colormap *pmaps)
{
    /* By the time we are processing requests, we can guarantee that there
     * is always a colormap installed */
#if XORG < 113
    *pmaps = InstalledMaps[pScreen->myNum]->mid;
#else
    *pmaps = GetInstalledColormap(pScreen)->mid;
#endif
    return (1);
}


static void 
vfbInstallColormap(ColormapPtr pmap)
{
#if XORG < 113
    int index = pmap->pScreen->myNum;
#endif
    ColormapPtr oldpmap;

#if XORG < 113
    oldpmap = InstalledMaps[index];
#else
    oldpmap = GetInstalledColormap(pmap->pScreen);
#endif

    if (pmap != oldpmap)
    {
	int entries;
	VisualPtr pVisual;
	Pixel *     ppix;
	xrgb *      prgb;
	xColorItem *defs;
	int i;

	if(oldpmap != (ColormapPtr)None)
	    WalkTree(pmap->pScreen, TellLostMap, (char *)&oldpmap->mid);
	/* Install pmap */
#if XORG < 113
	InstalledMaps[index] = pmap;
#else
	SetInstalledColormap(pmap->pScreen, pmap);
#endif
	WalkTree(pmap->pScreen, TellGainedMap, (char *)&pmap->mid);

	entries = pmap->pVisual->ColormapEntries;
	pVisual = pmap->pVisual;

	ppix = (Pixel *)xalloc(entries * sizeof(Pixel));
	prgb = (xrgb *)xalloc(entries * sizeof(xrgb));
	defs = (xColorItem *)xalloc(entries * sizeof(xColorItem));

	for (i = 0; i < entries; i++)  ppix[i] = i;
	/* XXX truecolor */
#if XORG < 19
	QueryColors(pmap, entries, ppix, prgb);
#else
	QueryColors(pmap, entries, ppix, prgb, serverClient);
#endif

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
#if XORG < 113
    ColormapPtr curpmap = InstalledMaps[pmap->pScreen->myNum];
#else
    ColormapPtr curpmap = GetInstalledColormap(pmap->pScreen);
#endif

    if(pmap == curpmap)
    {
	if (pmap->mid != pmap->pScreen->defColormap)
	{
#if XORG < 111
	    curpmap = (ColormapPtr) LookupIDByType(pmap->pScreen->defColormap,
						   RT_COLORMAP);
#else
	    dixLookupResourceByType((void * *) &curpmap, pmap->pScreen->defColormap,
				    RT_COLORMAP, serverClient, DixUnknownAccess);
#endif
	    (*pmap->pScreen->InstallColormap)(curpmap);
	}
    }
}

static Bool
vfbSaveScreen(ScreenPtr pScreen, int on)
{
    return TRUE;
}

#ifdef HAS_SHM
static void
vfbAllocateSharedMemoryFramebuffer(vfbFramebufferInfoPtr pfb)
{
    /* create the shared memory segment */

    pfb->shmid = shmget(IPC_PRIVATE, pfb->sizeInBytes, IPC_CREAT|0777);
    if (pfb->shmid < 0) {
        perror("shmget");
        ErrorF("shmget %d bytes failed, errno %d", pfb->sizeInBytes, errno);
        return;
    }

    /* try to attach it */

    pfb->pfbMemory = shmat(pfb->shmid, 0, 0);
    if (-1 == (long)pfb->pfbMemory) {
        perror("shmat");
        ErrorF("shmat failed, errno %d", errno);
        pfb->pfbMemory = NULL; 
        return;
    }
}
#endif /* HAS_SHM */


static void *
vfbAllocateFramebufferMemory(vfbFramebufferInfoPtr pfb)
{
    if (pfb->pfbMemory != NULL)
        return pfb->pfbMemory; /* already done */

    /* Compute memory layout */
    pfb->paddedBytesWidth = PixmapBytePad(pfb->width, pfb->depth);
    pfb->bitsPerPixel = vfbBitsPerPixel(pfb->depth);
    pfb->paddedWidth = pfb->paddedBytesWidth * 8 / pfb->bitsPerPixel;
    pfb->sizeInBytes = pfb->paddedBytesWidth * pfb->height;

    /* And allocate buffer */
    switch (fbmemtype) {
#ifdef HAS_SHM
    case SHARED_MEMORY_FB:
        vfbAllocateSharedMemoryFramebuffer(pfb);
        break;
#else
    case SHARED_MEMORY_FB:
        break;
#endif
    case NORMAL_MEMORY_FB:
        pfb->pfbMemory = Xalloc(pfb->sizeInBytes);
        break;
    }

    /* This will be NULL if any of the above failed */
    return pfb->pfbMemory;
}

static void
vfbFreeFramebufferMemory(vfbFramebufferInfoPtr pfb)
{
    if ((pfb == NULL) || (pfb->pfbMemory == NULL))
        return;

    switch (fbmemtype) {
#ifdef HAS_SHM
    case SHARED_MEMORY_FB:
        if (-1 == shmdt(pfb->pfbMemory)) {
            perror("shmdt");
            ErrorF("shmdt failed, errno %d", errno);
        }
        break;
#else /* HAS_SHM */
    case SHARED_MEMORY_FB:
        break;
#endif /* HAS_SHM */
    case NORMAL_MEMORY_FB:
        Xfree(pfb->pfbMemory);
        break;
    }

    pfb->pfbMemory = NULL;
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
#if XORG >= 16
			     DeviceIntPtr pDev,
#endif
			     ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static Bool vfbUnrealizeCursor(
#if XORG >= 16
			       DeviceIntPtr pDev,
#endif
			       ScreenPtr pScreen, CursorPtr pCursor) {
    return TRUE;
}

static void vfbSetCursor(
#if XORG >= 16
			 DeviceIntPtr pDev,
#endif
			 ScreenPtr pScreen, CursorPtr pCursor, int x, int y) 
{
}

static void vfbMoveCursor(
#if XORG >= 16
			  DeviceIntPtr pDev,
#endif
			  ScreenPtr pScreen, int x, int y) 
{
}

#if XORG >= 16
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
#if XORG >= 16
    , vfbDeviceCursorInitialize,
    vfbDeviceCursorCleanup
#endif
};

static miPointerScreenFuncRec vfbPointerCursorFuncs = {
    vfbCursorOffScreen,
    vfbCrossScreen,
    miPointerWarpCursor
};

#ifdef RANDR

static Bool vncRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
  // We update all information right away, so there is nothing to
  // do here.
  return TRUE;
}

/* from hw/xfree86/common/xf86Helper.c */

#include "mivalidate.h"
static void
xf86SetRootClip (ScreenPtr pScreen, Bool enable)
{
#if XORG < 19
    WindowPtr	pWin = WindowTable[pScreen->myNum];
#else
    WindowPtr	pWin = pScreen->root;
#endif
    WindowPtr	pChild;
    Bool	WasViewable = (Bool)(pWin->viewable);
    Bool	anyMarked = FALSE;
#if XORG < 110
    RegionPtr	pOldClip = NULL, bsExposed;
#ifdef DO_SAVE_UNDERS
    Bool	dosave = FALSE;
#endif
#endif
    WindowPtr   pLayerWin;
    BoxRec	box;

    if (WasViewable)
    {
	for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib)
	{
	    (void) (*pScreen->MarkOverlappedWindows)(pChild,
						     pChild,
						     &pLayerWin);
	}
	(*pScreen->MarkWindow) (pWin);
	anyMarked = TRUE;
	if (pWin->valdata)
	{
	    if (HasBorder (pWin))
	    {
		RegionPtr	borderVisible;

		borderVisible = REGION_CREATE(pScreen, NullBox, 1);
		REGION_SUBTRACT(pScreen, borderVisible,
				&pWin->borderClip, &pWin->winSize);
		pWin->valdata->before.borderVisible = borderVisible;
	    }
	    pWin->valdata->before.resized = TRUE;
	}
    }
    
    /*
     * Use REGION_BREAK to avoid optimizations in ValidateTree
     * that assume the root borderClip can't change well, normally
     * it doesn't...)
     */
    if (enable)
    {
	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pScreen->width;
	box.y2 = pScreen->height;
	REGION_INIT (pScreen, &pWin->winSize, &box, 1);
	REGION_INIT (pScreen, &pWin->borderSize, &box, 1);
	if (WasViewable)
	    REGION_RESET(pScreen, &pWin->borderClip, &box);
	pWin->drawable.width = pScreen->width;
	pWin->drawable.height = pScreen->height;
        REGION_BREAK (pWin->drawable.pScreen, &pWin->clipList);
    }
    else
    {
	REGION_EMPTY(pScreen, &pWin->borderClip);
	REGION_BREAK (pWin->drawable.pScreen, &pWin->clipList);
    }
    
    ResizeChildrenWinSize (pWin, 0, 0, 0, 0);
    
    if (WasViewable)
    {
#if XORG < 110
	if (pWin->backStorage)
	{
	    pOldClip = REGION_CREATE(pScreen, NullBox, 1);
	    REGION_COPY(pScreen, pOldClip, &pWin->clipList);
	}
#endif

	if (pWin->firstChild)
	{
	    anyMarked |= (*pScreen->MarkOverlappedWindows)(pWin->firstChild,
							   pWin->firstChild,
							   (WindowPtr *)NULL);
	}
	else
	{
	    (*pScreen->MarkWindow) (pWin);
	    anyMarked = TRUE;
	}

#if XORG < 110 && defined(DO_SAVE_UNDERS)
	if (DO_SAVE_UNDERS(pWin))
	{
	    dosave = (*pScreen->ChangeSaveUnder)(pLayerWin, pLayerWin);
	}
#endif /* DO_SAVE_UNDERS */

	if (anyMarked)
	    (*pScreen->ValidateTree)(pWin, NullWindow, VTOther);
    }

#if XORG < 110
    if (pWin->backStorage &&
	((pWin->backingStore == Always) || WasViewable))
    {
	if (!WasViewable)
	    pOldClip = &pWin->clipList; /* a convenient empty region */
	bsExposed = (*pScreen->TranslateBackingStore)
			     (pWin, 0, 0, pOldClip,
			      pWin->drawable.x, pWin->drawable.y);
	if (WasViewable)
	    REGION_DESTROY(pScreen, pOldClip);
	if (bsExposed)
	{
	    RegionPtr	valExposed = NullRegion;
    
	    if (pWin->valdata)
		valExposed = &pWin->valdata->after.exposed;
	    (*pScreen->WindowExposures) (pWin, valExposed, bsExposed);
	    if (valExposed)
		REGION_EMPTY(pScreen, valExposed);
	    REGION_DESTROY(pScreen, bsExposed);
	}
    }
#endif
    if (WasViewable)
    {
	if (anyMarked)
	    (*pScreen->HandleExposures)(pWin);

#if XORG < 110 && defined(DO_SAVE_UNDERS)
	if (dosave)
	    (*pScreen->PostChangeSaveUnder)(pLayerWin, pLayerWin);
#endif /* DO_SAVE_UNDERS */
	if (anyMarked && pScreen->PostValidateTree)
	    (*pScreen->PostValidateTree)(pWin, NullWindow, VTOther);
    }
    if (pWin->realized)
	WindowsRestructured ();
    FlushAllOutput ();
}

static Bool vncRandRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode,
                            int x, int y, Rotation rotation, int num_outputs,
                            RROutputPtr *outputs);

static Bool vncRandRScreenSetSize(ScreenPtr pScreen,
                                  CARD16 width, CARD16 height,
                                  CARD32 mmWidth, CARD32 mmHeight)
{
    vfbScreenInfoPtr pvfb = &vfbScreens[pScreen->myNum];
    vfbFramebufferInfo fb;
    rrScrPrivPtr rp = rrGetScrPriv(pScreen);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    void *pbits;
    Bool ret;
    int oldwidth, oldheight, oldmmWidth, oldmmHeight;

    /* Prevent updates while we fiddle */
    xf86SetRootClip(pScreen, FALSE);

    /* Store current state in case we fail */
    oldwidth = pScreen->width;
    oldheight = pScreen->height;
    oldmmWidth = pScreen->mmWidth;
    oldmmHeight = pScreen->mmHeight;

    /* Then set the new dimensions */
    pScreen->width = width;
    pScreen->height = height;
    pScreen->mmWidth = mmWidth;
    pScreen->mmHeight = mmHeight;

    /* Allocate a new framebuffer */
    memset(&fb, 0, sizeof(vfbFramebufferInfo));

    fb.width = pScreen->width;
    fb.height = pScreen->height;
    fb.depth = pvfb->fb.depth;

    pbits = vfbAllocateFramebufferMemory(&fb);
    if (!pbits) {
        /* Allocation failed. Restore old state */
        pScreen->width = oldwidth;
        pScreen->height = oldheight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;

        xf86SetRootClip(pScreen, TRUE);

        return FALSE;
    }

    /* Update root pixmap with the new dimensions and buffer */
    ret = pScreen->ModifyPixmapHeader(rootPixmap, fb.width, fb.height,
                                      -1, -1, fb.paddedBytesWidth, pbits);
    if (!ret) {
        /* Update failed. Free the new framebuffer and restore old state */
        vfbFreeFramebufferMemory(&fb);

        pScreen->width = oldwidth;
        pScreen->height = oldheight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;

        xf86SetRootClip(pScreen, TRUE);

        return FALSE;
    }

    /* Free the old framebuffer and keep the info about the new one */
    vfbFreeFramebufferMemory(&pvfb->fb);
    memcpy(&pvfb->fb, &fb, sizeof(vfbFramebufferInfo));

    /* Let VNC get the new framebuffer (actual update is in vncHooks.cc) */
    vncFbptr[pScreen->myNum] = pbits;
    vncFbstride[pScreen->myNum] = fb.paddedWidth;

    /* Restore ability to update screen, now with new dimensions */
    xf86SetRootClip(pScreen, TRUE);

    /*
     * Let RandR know we changed something (it doesn't assume that
     * TRUE means something changed for some reason...).
     */
    RRScreenSizeNotify(pScreen);

    /* Crop all CRTCs to the new screen */
    for (int i = 0;i < rp->numCrtcs;i++) {
        RRCrtcPtr crtc;
        RRModePtr mode;

        crtc = rp->crtcs[i];

        /* Disabled? */
        if (crtc->mode == NULL)
            continue;

        /* Fully inside? */
        if ((crtc->x + crtc->mode->mode.width <= width) &&
            (crtc->y + crtc->mode->mode.height <= height))
            continue;

        /* Fully outside? */
        if ((crtc->x >= width) || (crtc->y >= height)) {
            /* Disable it */
            ret = vncRandRCrtcSet(pScreen, crtc, NULL,
                                  crtc->x, crtc->y, crtc->rotation, 0, NULL);
            if (!ret)
                ErrorF("Warning: Unable to disable CRTC that is outside of new screen dimensions");
            continue;
        }

        /* Just needs to be resized */
        mode = vncRandRModeGet(width - crtc->x, height - crtc->y);
        if (mode == NULL) {
            ErrorF("Warning: Unable to create custom mode for %dx%d",
                   width - crtc->x, height - crtc->y);
            continue;
        }

        ret = vncRandRCrtcSet(pScreen, crtc, mode,
                              crtc->x, crtc->y, crtc->rotation,
                              crtc->numOutputs, crtc->outputs);
        RRModeDestroy(mode);
        if (!ret)
            ErrorF("Warning: Unable to crop CRTC to new screen dimensions");
    }

    return TRUE;
}

static Bool vncRandRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode,
                            int x, int y, Rotation rotation, int num_outputs,
                            RROutputPtr *outputs)
{
    Bool ret;
    int i;

    /*
     * Some applications get confused by a connected output without a
     * mode or CRTC, so we need to fiddle with the connection state as well.
     */
    for (i = 0;i < crtc->numOutputs;i++)
        RROutputSetConnection(crtc->outputs[i], RR_Disconnected);

    for (i = 0;i < num_outputs;i++) {
        if (mode != NULL)
            RROutputSetConnection(outputs[i], RR_Connected);
        else
            RROutputSetConnection(outputs[i], RR_Disconnected);
    }

    /* Let RandR know we approve, and let it update its internal state */
    ret = RRCrtcNotify(crtc, mode, x, y, rotation,
#if XORG >= 16
                       NULL,
#endif
                       num_outputs, outputs);
    if (!ret)
        return FALSE;

    return TRUE;
}

static Bool vncRandROutputValidateMode(ScreenPtr pScreen,
                                       RROutputPtr output, RRModePtr mode)
{
    /* We have no hardware so any mode works */
    return TRUE;
}

static void vncRandRModeDestroy(ScreenPtr pScreen, RRModePtr mode)
{
    /* We haven't allocated anything so nothing to destroy */
}

static const int vncRandRWidths[] =  { 1920, 1920, 1600, 1680, 1400, 1360, 1280, 1280, 1280, 1280, 1024, 800, 640 };
static const int vncRandRHeights[] = { 1200, 1080, 1200, 1050, 1050,  768, 1024,  960,  800,  720,  768, 600, 480 };

static int vncRandRIndex = 0;

/* This is a global symbol since findRandRMode() also uses it */
void *vncRandRModeGet(int width, int height)
{
    xRRModeInfo	modeInfo;
    char name[100];
    RRModePtr mode;

    memset(&modeInfo, 0, sizeof(modeInfo));
    sprintf(name, "%dx%d", width, height);
    
    modeInfo.width = width;
    modeInfo.height = height;
    modeInfo.hTotal = width;
    modeInfo.vTotal = height;
    modeInfo.dotClock = ((CARD32)width * (CARD32)height * 60);
    modeInfo.nameLength = strlen(name);
    mode = RRModeGet(&modeInfo, name);
    if (mode == NULL)
        return NULL;

    return mode;
}

static RRCrtcPtr vncRandRCrtcCreate(ScreenPtr pScreen)
{
    RRCrtcPtr crtc;
    RROutputPtr output;
    RRModePtr mode;
    char name[100];

    RRModePtr *modes;
    int i, num_modes;

    /* First we create the CRTC... */
    crtc = RRCrtcCreate(pScreen, NULL);

    /* We don't actually support gamma, but xrandr complains when it is missing */
    RRCrtcGammaSetSize (crtc, 256);

    /* Then we create a dummy output for it... */
    sprintf(name, "VNC-%d", vncRandRIndex);
    vncRandRIndex++;

    output = RROutputCreate(pScreen, name, strlen(name), NULL);

    RROutputSetCrtcs(output, &crtc, 1);
    RROutputSetConnection(output, RR_Disconnected);

    /* Make sure the CRTC has this output set */
    vncRandRCrtcSet(pScreen, crtc, NULL, 0, 0, RR_Rotate_0, 1, &output);

    /* Populate a list of default modes */
    modes = malloc(sizeof(RRModePtr)*sizeof(vncRandRWidths)/sizeof(*vncRandRWidths));
    if (modes == NULL)
        return NULL;

    num_modes = 0;
    for (i = 0;i < sizeof(vncRandRWidths)/sizeof(*vncRandRWidths);i++) {
        mode = vncRandRModeGet(vncRandRWidths[i], vncRandRHeights[i]);
        if (mode != NULL) {
            modes[num_modes] = mode;
            num_modes++;
        }
    }

    RROutputSetModes(output, modes, num_modes, 0);

    free(modes);

    return crtc;
}

/* Used from XserverDesktop when it needs more outputs... */
int vncRandRCreateOutputs(int scrIdx, int extraOutputs)
{
    RRCrtcPtr crtc;

    while (extraOutputs > 0) {
        crtc = vncRandRCrtcCreate(screenInfo.screens[scrIdx]);
        if (crtc == NULL)
            return -1;
        extraOutputs--;
    }

    return 0;
}

static Bool vncRandRInit(ScreenPtr pScreen)
{
    RRCrtcPtr crtc;
    RRModePtr mode;
    Bool ret;

    if (!RRInit())
        return FALSE;

    /* These are completely arbitrary */
    RRScreenSetSizeRange(pScreen, 32, 32, 32768, 32768);

    /*
     * Start with a single CRTC with a single output. More will be
     * allocated as needed...
     */
    crtc = vncRandRCrtcCreate(pScreen);

    /* Make sure the current screen size is the active mode */
    mode = vncRandRModeGet(pScreen->width, pScreen->height);
    if (mode == NULL)
        return FALSE;

    ret = vncRandRCrtcSet(pScreen, crtc, mode, 0, 0, RR_Rotate_0,
                          crtc->numOutputs, crtc->outputs);
    RRModeDestroy(mode);
    if (!ret)
        return FALSE;

    return TRUE;
}

#endif

static Bool
#if XORG < 113
vfbCloseScreen(int index, ScreenPtr pScreen)
#else
vfbCloseScreen(ScreenPtr pScreen)
#endif
{
#if XORG < 113
    vfbScreenInfoPtr pvfb = &vfbScreens[index];
#else
    vfbScreenInfoPtr pvfb = &vfbScreens[pScreen->myNum];
#endif
    int i;
 
    pScreen->CloseScreen = pvfb->closeScreen;

    /*
     * XXX probably lots of stuff to clean.  For now,
     * clear installed colormaps so that server reset works correctly.
     */
#if XORG < 113
    for (i = 0; i < MAXSCREENS; i++)
	InstalledMaps[i] = NULL;

    return pScreen->CloseScreen(index, pScreen);
#else
    for (i = 0; i < screenInfo.numScreens; i++)
	SetInstalledColormap(screenInfo.screens[i], NULL);

    /*
     * fb overwrites miCloseScreen, so do this here
     */
    if (pScreen->devPrivate)
        (*pScreen->DestroyPixmap) ((PixmapPtr) pScreen->devPrivate);
    pScreen->devPrivate = NULL;

    return pScreen->CloseScreen(pScreen);
#endif
}

static Bool
#if XORG < 113
vfbScreenInit(int index, ScreenPtr pScreen, int argc, char **argv)
#else
vfbScreenInit(ScreenPtr pScreen, int argc, char **argv)
#endif
{
#if XORG < 113
    vfbScreenInfoPtr pvfb = &vfbScreens[index];
#else
    vfbScreenInfoPtr pvfb = &vfbScreens[pScreen->myNum];
#endif
    int dpi;
    int ret;
    void *pbits;

#ifdef RANDR
    rrScrPrivPtr rp;
#endif

#if XORG >= 113
    if (!dixRegisterPrivateKey(&cmapScrPrivateKeyRec, PRIVATE_SCREEN, 0))
	return FALSE;
#endif

    /* 96 is the default used by most other systems */
    dpi = 96;
    if (monitorResolution)
        dpi = monitorResolution;

    pbits = vfbAllocateFramebufferMemory(&pvfb->fb);
    if (!pbits) return FALSE;
#if XORG < 113
    vncFbptr[index] = pbits;
    vncFbstride[index] = pvfb->fb.paddedWidth;
#else
    vncFbptr[pScreen->myNum] = pbits;
    vncFbstride[pScreen->myNum] = pvfb->fb.paddedWidth;
#endif

    miSetPixmapDepths();

    switch (pvfb->fb.depth) {
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

    ret = fbScreenInit(pScreen, pbits, pvfb->fb.width, pvfb->fb.height,
		       dpi, dpi, pvfb->fb.paddedWidth, pvfb->fb.bitsPerPixel);
  
#ifdef RENDER
    if (ret && Render) 
	ret = fbPictureInit (pScreen, 0, 0);
#endif

    if (!ret) return FALSE;

#if XORG < 110
    miInitializeBackingStore(pScreen);
#endif

    /*
     * Circumvent the backing store that was just initialised.  This amounts
     * to a truely bizarre way of initialising SaveDoomedAreas and friends.
     */

    pScreen->InstallColormap = vfbInstallColormap;
    pScreen->UninstallColormap = vfbUninstallColormap;
    pScreen->ListInstalledColormaps = vfbListInstalledColormaps;

    pScreen->SaveScreen = vfbSaveScreen;
    
    miPointerInitialize(pScreen, &vfbPointerSpriteFuncs, &vfbPointerCursorFuncs,
			FALSE);
    
    pScreen->blackPixel = pvfb->blackPixel;
    pScreen->whitePixel = pvfb->whitePixel;

    if (!pvfb->pixelFormatDefined) {
	switch (pvfb->fb.depth) {
	case 16:
	    pvfb->pixelFormatDefined = TRUE;
	    pvfb->rgbNotBgr = TRUE;
	    pvfb->blueBits = pvfb->redBits = 5;
	    pvfb->greenBits = 6;
	    break;
	case 24:
	case 32:
	    pvfb->pixelFormatDefined = TRUE;
	    pvfb->rgbNotBgr = TRUE;
	    pvfb->blueBits = pvfb->redBits = pvfb->greenBits = 8;
	    break;
	}
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
    if (!ret) return FALSE;

    miSetZeroLineBias(pScreen, pvfb->lineBias);

    pvfb->closeScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = vfbCloseScreen;

#ifdef RANDR
    ret = RRScreenInit(pScreen);
    if (!ret) return FALSE;

    rp = rrGetScrPriv(pScreen);

    rp->rrGetInfo = vncRandRGetInfo;
    rp->rrSetConfig = NULL;
    rp->rrScreenSetSize = vncRandRScreenSetSize;
    rp->rrCrtcSet = vncRandRCrtcSet;
    rp->rrOutputValidateMode = vncRandROutputValidateMode;
    rp->rrModeDestroy = vncRandRModeDestroy;

    ret = vncRandRInit(pScreen);
    if (!ret) return FALSE;
#endif


  return TRUE;

} /* end vfbScreenInit */


static void vfbClientStateChange(CallbackListPtr *a, void *b, void *c) {
  dispatchException &= ~DE_RESET;
}
 
#if XORG >= 113
#ifdef GLXEXT
extern void GlxExtensionInit(void);

static ExtensionModule glxExt = {
    GlxExtensionInit,
    "GLX",
    &noGlxExtension
};
#endif
#endif

void
InitOutput(ScreenInfo *scrInfo, int argc, char **argv)
{
    int i;
    int NumFormats = 0;

  ErrorF("\nXvnc %s - built %s\n%s", XVNCVERSION, buildtime, XVNCCOPYRIGHT);
  ErrorF("Underlying X server release %d, %s\n\n", VENDOR_RELEASE,
         VENDOR_STRING);

#if XORG >= 113
#ifdef GLXEXT
    if (serverGeneration == 1)
#if XORG >= 116
        LoadExtensionList(&glxExt, 1, TRUE);
#else
        LoadExtension(&glxExt, TRUE);
#endif
#endif
#endif

    /* initialize pixmap formats */

    /* must have a pixmap depth to match every screen depth */
    for (i = 0; i < vfbNumScreens; i++)
    {
	vfbPixmapDepths[vfbScreens[i].fb.depth] = TRUE;
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
	    scrInfo->formats[NumFormats].depth = i;
	    scrInfo->formats[NumFormats].bitsPerPixel = vfbBitsPerPixel(i);
	    scrInfo->formats[NumFormats].scanlinePad = BITMAP_SCANLINE_PAD;
	    NumFormats++;
	}
    }

    scrInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    scrInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    scrInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    scrInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
    scrInfo->numPixmapFormats = NumFormats;

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

void ProcessInputEvents(void)
{
  mieqProcessInputEvents();
#if XORG == 15
  miPointerUpdate();
#endif
}

// InitInput is called after InitExtensions, so we're guaranteed that
// vncExtensionInit() has already been called.

void InitInput(int argc, char *argv[])
{
  mieqInit ();
}

#if XORG > 17
void CloseInput(void)
{
}
#endif

void vncClientGone(int fd)
{
  if (fd == vncInetdSock) {
    ErrorF("inetdSock client gone\n");
    GiveUp(0);
  }
}
