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
#include "RandrGlue.h"
#include "xorg-version.h"

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xos.h>
#include "scrnintstr.h"
#include "servermd.h"
#include "fb.h"
#include "mi.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "micmap.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/param.h>
#include "dix.h"
#include "os.h"
#include "miline.h"
#include "glx_extinit.h"
#include "inputstr.h"
#include "randrstr.h"
#ifdef DPMSExtension
#include "dpmsproc.h"
#endif
#include <X11/keysym.h>
extern char buildtime[];

#undef VENDOR_RELEASE
#undef VENDOR_STRING
#include "version-config.h"

#define XVNCVERSION "TigerVNC 1.12.80"
#define XVNCCOPYRIGHT ("Copyright (C) 1999-2022 TigerVNC Team and many others (see README.rst)\n" \
                       "See https://www.tigervnc.org for information on TigerVNC.\n")

#define VNC_DEFAULT_WIDTH  1024
#define VNC_DEFAULT_HEIGHT 768
#define VNC_DEFAULT_DEPTH  24

typedef struct {
    int width;
    int paddedBytesWidth;
    int paddedWidth;
    int height;
    int depth;
    int bitsPerPixel;
    void *pfbMemory;
} VncFramebufferInfo, *VncFramebufferInfoPtr;

typedef struct {
    VncFramebufferInfo fb;

    Bool pixelFormatDefined;
    Bool rgbNotBgr;
    int redBits, greenBits, blueBits;
} VncScreenInfo, *VncScreenInfoPtr;

static VncScreenInfo vncScreenInfo = {
    .fb.width = VNC_DEFAULT_WIDTH,
    .fb.height = VNC_DEFAULT_HEIGHT,
    .fb.depth = VNC_DEFAULT_DEPTH,
    .fb.pfbMemory = NULL,
    .pixelFormatDefined = FALSE,
};

static Bool vncPixmapDepths[33];
static Bool Render = TRUE;

static Bool displaySpecified = FALSE;
static char displayNumStr[16];

static int vncVerbose = 0;

static void
vncPrintBanner(void)
{
    ErrorF("\nXvnc %s - built %s\n%s", XVNCVERSION, buildtime, XVNCCOPYRIGHT);
    ErrorF("Underlying X server release %d\n\n", VENDOR_RELEASE);
}

static void
vncInitializePixmapDepths(void)
{
    int i;

    vncPixmapDepths[1] = TRUE;  /* always need bitmaps */
    for (i = 2; i <= 32; i++)
        vncPixmapDepths[i] = FALSE;
}

static int
vncBitsPerPixel(int depth)
{
    if (depth == 1)
        return 1;
    else if (depth <= 8)
        return 8;
    else if (depth <= 16)
        return 16;
    else
        return 32;
}

static void vncFreeFramebufferMemory(VncFramebufferInfoPtr pfb);

#ifdef DPMSExtension
#if XORG_OLDER_THAN(1, 20, 0)
    /* Why support DPMS? Because stupid modern desktop environments
       such as Unity 2D on Ubuntu 11.10 crashes if DPMS is not
       available. (DPMSSet is called by dpms.c, but the return value
       is ignored.) */
int
DPMSSet(ClientPtr client, int level)
{
    return Success;
}

Bool
DPMSSupported(void)
{
    /* Causes DPMSCapable to return false, meaning no devices are DPMS
       capable */
    return FALSE;
}
#endif
#endif

void
ddxGiveUp(enum ExitCode error)
{
    /* clean up the framebuffers */
    vncFreeFramebufferMemory(&vncScreenInfo.fb);
}

void
AbortDDX(enum ExitCode error)
{
    ddxGiveUp(error);
}

void
OsVendorInit(void)
{
}

void
OsVendorFatalError(const char *f, va_list args)
{
}

#ifdef DDXBEFORERESET
void
ddxBeforeReset(void)
{
    return;
}
#endif

#if INPUTTHREAD
#if XORG_AT_LEAST(1, 20, 7)
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void)
{
}
#endif
#endif

void
ddxUseMsg(void)
{
    vncPrintBanner();

    ErrorF("-pixdepths list-of-int support given pixmap depths\n");
    ErrorF("+/-render		   turn on/off RENDER extension support"
           "(default on)\n");

    ErrorF("-geometry WxH          set screen 0's width, height\n");
    ErrorF("-depth D               set screen 0's depth\n");
    ErrorF("-pixelformat fmt       set pixel format (rgbNNN or bgrNNN)\n");
    ErrorF("-inetd                 has been launched from inetd\n");
    ErrorF
        ("-noclipboard           disable clipboard settings modification via vncconfig utility\n");
    ErrorF("-verbose [n]           verbose startup messages\n");
    ErrorF("-quiet                 minimal startup messages\n");
    ErrorF("-version               show the server version\n");
    ErrorF("\nVNC parameters:\n");

    fprintf(stderr, "\n"
            "Parameters can be turned on with -<param> or off with -<param>=0\n"
            "Parameters which take a value can be specified as "
            "-<param> <value>\n"
            "Other valid forms are <param>=<value> -<param>=<value> "
            "--<param>=<value>\n"
            "Parameter names are case-insensitive.  The parameters are:\n\n");
    vncListParams(79, 14);
}

static Bool
displayNumFree(int num)
{
    char file[256];

    if (vncIsTCPPortUsed(6000 + num))
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

    if (firstTime) {
        /* Force -noreset as default until we properly handle resets */
        dispatchExceptionAtReset = 0;

        vncInitializePixmapDepths();
        firstTime = FALSE;
        vncInitRFB();
    }

    if (argv[i][0] == ':')
        displaySpecified = TRUE;

#define CHECK_FOR_REQUIRED_ARGUMENTS(num) \
    if (((i + num) >= argc) || (!argv[i + num])) {                      \
      ErrorF("Required argument to %s not specified\n", argv[i]);       \
      UseMsg();                                                         \
      FatalError("Required argument to %s not specified\n", argv[i]);   \
    }

    if (strcmp(argv[i], "-pixdepths") == 0) {   /* -pixdepths list-of-depth */
        int depth, ret = 1;

        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        ++i;
        while ((i < argc) && (depth = atoi(argv[i++])) != 0) {
            if (depth < 0 || depth > 32) {
                ErrorF("Invalid pixmap depth %d\n", depth);
                UseMsg();
                FatalError("Invalid pixmap depth %d passed to -pixdepths\n",
                           depth);
            }
            vncPixmapDepths[depth] = TRUE;
            ret++;
        }
        return ret;
    }

    if (strcmp(argv[i], "+render") == 0) {      /* +render */
        Render = TRUE;
        return 1;
    }

    if (strcmp(argv[i], "-render") == 0) {      /* -render */
        Render = FALSE;
        return 1;
    }

    if (strcmp(argv[i], "-geometry") == 0) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        ++i;
        if (sscanf(argv[i], "%dx%d", &vncScreenInfo.fb.width,
                   &vncScreenInfo.fb.height) != 2) {
            ErrorF("Invalid geometry %s\n", argv[i]);
            UseMsg();
            FatalError("Invalid geometry %s passed to -geometry\n",
                       argv[i]);
        }
        return 2;
    }

    if (strcmp(argv[i], "-depth") == 0) {
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        ++i;
        vncScreenInfo.fb.depth = atoi(argv[i]);
        return 2;
    }

    if (strcmp(argv[i], "-pixelformat") == 0) {
        char rgbbgr[4];
        int bits1, bits2, bits3;

        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        ++i;
        if (sscanf(argv[i], "%3s%1d%1d%1d", rgbbgr, &bits1, &bits2, &bits3) < 4) {
            ErrorF("Invalid pixel format %s\n", argv[i]);
            UseMsg();
            FatalError("Invalid pixel format %s passed to -pixelformat\n",
                       argv[i]);
        }

        vncScreenInfo.pixelFormatDefined = TRUE;
        vncScreenInfo.fb.depth = bits1 + bits2 + bits3;
        vncScreenInfo.greenBits = bits2;
        if (strcasecmp(rgbbgr, "bgr") == 0) {
            vncScreenInfo.rgbNotBgr = FALSE;
            vncScreenInfo.redBits = bits3;
            vncScreenInfo.blueBits = bits1;
        } else if (strcasecmp(rgbbgr, "rgb") == 0) {
            vncScreenInfo.rgbNotBgr = TRUE;
            vncScreenInfo.redBits = bits1;
            vncScreenInfo.blueBits = bits3;
        } else {
            ErrorF("Invalid pixel format %s\n", argv[i]);
            UseMsg();
            FatalError("Invalid pixel format %s passed to -pixelformat\n",
                       argv[i]);
        }

        return 2;
    }

    if (strcmp(argv[i], "-inetd") == 0) {
        int nullfd;

        dup2(0, 3);
        vncInetdSock = 3;

        /* Avoid xserver >= 1.19's epoll-fd becoming fd 2 / stderr only to be
           replaced by /dev/null by OsInit() because the pollfd is not
           writable, breaking ospoll_wait(). */
        nullfd = open("/dev/null", O_WRONLY);
        dup2(nullfd, 2);
        close(nullfd);

        if (!displaySpecified) {
            int port = vncGetSocketPort(vncInetdSock);
            int displayNum = port - 5900;

            if (displayNum < 0 || displayNum > 99 ||
                !displayNumFree(displayNum)) {
                for (displayNum = 1; displayNum < 100; displayNum++)
                    if (displayNumFree(displayNum))
                        break;

                if (displayNum == 100)
                    FatalError
                        ("Xvnc error: no free display number for -inetd\n");
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

    if (!strcmp(argv[i], "-showconfig") || !strcmp(argv[i], "-version")) {
        vncPrintBanner();
        exit(0);
    }

    /* We need to resolve an ambiguity for booleans */
    if (argv[i][0] == '-' && i + 1 < argc && vncIsParamBool(&argv[i][1])) {
        if ((strcasecmp(argv[i + 1], "0") == 0) ||
            (strcasecmp(argv[i + 1], "1") == 0) ||
            (strcasecmp(argv[i + 1], "true") == 0) ||
            (strcasecmp(argv[i + 1], "false") == 0) ||
            (strcasecmp(argv[i + 1], "yes") == 0) ||
            (strcasecmp(argv[i + 1], "no") == 0)) {
            vncSetParam(&argv[i][1], argv[i + 1]);
            return 2;
        }
    }

    if (vncSetParamSimple(argv[i]))
        return 1;

    if (argv[i][0] == '-' && i + 1 < argc) {
        if (vncSetParam(&argv[i][1], argv[i + 1]))
            return 2;
    }

    return 0;
}

static Bool
vncSaveScreen(ScreenPtr pScreen, int on)
{
    return TRUE;
}

static void *
vncAllocateFramebufferMemory(VncFramebufferInfoPtr pfb)
{
    size_t sizeInBytes;

    if (pfb->pfbMemory != NULL)
        return pfb->pfbMemory;  /* already done */

    /* Compute memory layout */
    pfb->paddedBytesWidth = PixmapBytePad(pfb->width, pfb->depth);
    pfb->bitsPerPixel = vncBitsPerPixel(pfb->depth);
    pfb->paddedWidth = pfb->paddedBytesWidth * 8 / pfb->bitsPerPixel;

    /* And allocate buffer */
    sizeInBytes = pfb->paddedBytesWidth * pfb->height;
    pfb->pfbMemory = malloc(sizeInBytes);

    /* This will be NULL if the above failed */
    return pfb->pfbMemory;
}

static void
vncFreeFramebufferMemory(VncFramebufferInfoPtr pfb)
{
    if ((pfb == NULL) || (pfb->pfbMemory == NULL))
        return;

    free(pfb->pfbMemory);
    pfb->pfbMemory = NULL;
}

static Bool
vncCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y)
{
    return FALSE;
}

static void
vncCrossScreen(ScreenPtr pScreen, Bool entering)
{
}

static Bool
vncRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

static Bool
vncUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

static void
vncSetCursor(DeviceIntPtr pDev,
             ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
}

static void
vncMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
}

static Bool
vncDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen)
{
    return TRUE;
}

static void
vncDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen)
{
}

static miPointerSpriteFuncRec vncPointerSpriteFuncs = {
    vncRealizeCursor,
    vncUnrealizeCursor,
    vncSetCursor,
    vncMoveCursor,
    vncDeviceCursorInitialize,
    vncDeviceCursorCleanup
};

static miPointerScreenFuncRec vncPointerCursorFuncs = {
    vncCursorOffScreen,
    vncCrossScreen,
    miPointerWarpCursor
};

static Bool
vncRandRGetInfo(ScreenPtr pScreen, Rotation * rotations)
{
    // We update all information right away, so there is nothing to
    // do here.
    return TRUE;
}

static Bool vncRandRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode,
                            int x, int y, Rotation rotation, int num_outputs,
                            RROutputPtr * outputs);
static RRModePtr vncRandRModeGet(int width, int height);

static Bool
vncRandRScreenSetSize(ScreenPtr pScreen,
                      CARD16 width, CARD16 height,
                      CARD32 mmWidth, CARD32 mmHeight)
{
    VncFramebufferInfo fb;
    rrScrPrivPtr rp = rrGetScrPriv(pScreen);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    void *pbits;
    Bool ret;
    int oldwidth, oldheight, oldmmWidth, oldmmHeight;

    /* Prevent updates while we fiddle */
    SetRootClip(pScreen, ROOT_CLIP_NONE);

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
    memset(&fb, 0, sizeof(VncFramebufferInfo));

    fb.width = pScreen->width;
    fb.height = pScreen->height;
    fb.depth = vncScreenInfo.fb.depth;

    pbits = vncAllocateFramebufferMemory(&fb);
    if (!pbits) {
        /* Allocation failed. Restore old state */
        pScreen->width = oldwidth;
        pScreen->height = oldheight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;

        SetRootClip(pScreen, ROOT_CLIP_FULL);

        return FALSE;
    }

    /* Update root pixmap with the new dimensions and buffer */
    ret = pScreen->ModifyPixmapHeader(rootPixmap, fb.width, fb.height,
                                      -1, -1, fb.paddedBytesWidth, pbits);
    if (!ret) {
        /* Update failed. Free the new framebuffer and restore old state */
        vncFreeFramebufferMemory(&fb);

        pScreen->width = oldwidth;
        pScreen->height = oldheight;
        pScreen->mmWidth = oldmmWidth;
        pScreen->mmHeight = oldmmHeight;

        SetRootClip(pScreen, ROOT_CLIP_FULL);

        return FALSE;
    }

    /* Free the old framebuffer and keep the info about the new one */
    vncFreeFramebufferMemory(&vncScreenInfo.fb);
    vncScreenInfo.fb = fb;

    /* Let VNC get the new framebuffer (actual update is in vncHooks.cc) */
    vncFbptr[pScreen->myNum] = pbits;
    vncFbstride[pScreen->myNum] = fb.paddedWidth;

    /* Restore ability to update screen, now with new dimensions */
    SetRootClip(pScreen, ROOT_CLIP_FULL);

    /*
     * Let RandR know we changed something (it doesn't assume that
     * TRUE means something changed for some reason...).
     */
    RRScreenSizeNotify(pScreen);

    /* Crop all CRTCs to the new screen */
    for (int i = 0; i < rp->numCrtcs; i++) {
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

        /* Just needs to be resized to a temporary mode */
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

static Bool
vncRandRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode,
                int x, int y, Rotation rotation, int num_outputs,
                RROutputPtr * outputs)
{
    Bool ret;
    int i;

    /*
     * Some applications get confused by a connected output without a
     * mode or CRTC, so we need to fiddle with the connection state as well.
     */
    for (i = 0; i < crtc->numOutputs; i++)
        RROutputSetConnection(crtc->outputs[i], RR_Disconnected);

    for (i = 0; i < num_outputs; i++) {
        if (mode != NULL)
            RROutputSetConnection(outputs[i], RR_Connected);
        else
            RROutputSetConnection(outputs[i], RR_Disconnected);
    }

    /* Let RandR know we approve, and let it update its internal state */
    ret = RRCrtcNotify(crtc, mode, x, y, rotation, NULL, num_outputs, outputs);
    if (!ret)
        return FALSE;

    return TRUE;
}

static Bool
vncRandROutputValidateMode(ScreenPtr pScreen,
                           RROutputPtr output, RRModePtr mode)
{
    /* We have no hardware so any mode works */
    return TRUE;
}

static void
vncRandRModeDestroy(ScreenPtr pScreen, RRModePtr mode)
{
    /* We haven't allocated anything so nothing to destroy */
}

static const int vncRandRWidths[] =
    { 1920, 1920, 1600, 1680, 1400, 1360, 1280, 1280, 1280, 1280, 1024, 800, 640 };
static const int vncRandRHeights[] =
    { 1200, 1080, 1200, 1050, 1050,  768, 1024,  960,  800,  720,  768, 600, 480 };

static int vncRandRIndex = 0;

static RRModePtr
vncRandRModeGet(int width, int height)
{
    xRRModeInfo modeInfo;
    char name[100];
    RRModePtr mode;

    memset(&modeInfo, 0, sizeof(modeInfo));
    sprintf(name, "%dx%d", width, height);

    modeInfo.width = width;
    modeInfo.height = height;
    modeInfo.hTotal = width;
    modeInfo.vTotal = height;
    modeInfo.dotClock = ((CARD32) width * (CARD32) height * 60);
    modeInfo.nameLength = strlen(name);
    mode = RRModeGet(&modeInfo, name);
    if (mode == NULL)
        return NULL;

    return mode;
}

static void
vncRandRSetModes(RROutputPtr output, int pref_width, int pref_height)
{
    RRModePtr mode;
    RRModePtr *modes;
    int i, num_modes, num_pref;

    num_modes = sizeof(vncRandRWidths) / sizeof(*vncRandRWidths) + 1;
    modes = malloc(sizeof(RRModePtr) * num_modes);
    if (modes == NULL)
        return;

    num_modes = 0;
    num_pref = 0;

    if ((pref_width > 0) && (pref_height > 0)) {
        mode = vncRandRModeGet(pref_width, pref_height);
        if (mode != NULL) {
            modes[num_modes] = mode;
            num_modes++;
            num_pref++;
        }
    }

    for (i = 0; i < sizeof(vncRandRWidths) / sizeof(*vncRandRWidths); i++) {
        if ((vncRandRWidths[i] == pref_width) &&
            (vncRandRHeights[i] == pref_height))
            continue;
        mode = vncRandRModeGet(vncRandRWidths[i], vncRandRHeights[i]);
        if (mode != NULL) {
            modes[num_modes] = mode;
            num_modes++;
        }
    }

    RROutputSetModes(output, modes, num_modes, num_pref);

    free(modes);
}

static RRCrtcPtr
vncRandRCrtcCreate(ScreenPtr pScreen)
{
    RRCrtcPtr crtc;
    RROutputPtr output;
    char name[100];

    /* First we create the CRTC... */
    crtc = RRCrtcCreate(pScreen, NULL);

    /* We don't actually support gamma, but xrandr complains when it is missing */
    RRCrtcGammaSetSize(crtc, 256);

    /* Then we create a dummy output for it... */
    sprintf(name, "VNC-%d", vncRandRIndex);
    vncRandRIndex++;

    output = RROutputCreate(pScreen, name, strlen(name), NULL);

    RROutputSetCrtcs(output, &crtc, 1);
    RROutputSetConnection(output, RR_Disconnected);

    /* Make sure the CRTC has this output set */
    vncRandRCrtcSet(pScreen, crtc, NULL, 0, 0, RR_Rotate_0, 1, &output);

    /* Populate a list of default modes */
    vncRandRSetModes(output, -1, -1);

    return crtc;
}

/* Used from XserverDesktop when it needs more outputs... */

int
vncRandRCanCreateScreenOutputs(int scrIdx, int extraOutputs)
{
    return 1;
}

int
vncRandRCreateScreenOutputs(int scrIdx, int extraOutputs)
{
    RRCrtcPtr crtc;

    while (extraOutputs > 0) {
        crtc = vncRandRCrtcCreate(screenInfo.screens[scrIdx]);
        if (crtc == NULL)
            return 0;
        extraOutputs--;
    }

    return 1;
}

/* Creating and modifying modes, used by XserverDesktop and init here */

int
vncRandRCanCreateModes()
{
    return 1;
}

void *
vncRandRCreateMode(void *out, int width, int height)
{
    RROutputPtr output;

    output = out;

    /* Do we already have the mode? */
    for (int i = 0; i < output->numModes; i++) {
        if ((output->modes[i]->mode.width == width) &&
            (output->modes[i]->mode.height == height))
            return output->modes[i];
    }

    /* Just recreate the entire list */
    vncRandRSetModes(output, width, height);

    /* Find the new mode */
    for (int i = 0; i < output->numModes; i++) {
        if ((output->modes[i]->mode.width == width) &&
            (output->modes[i]->mode.height == height))
            return output->modes[i];
    }

    /* Something went horribly wrong */
    return NULL;
}

void *
vncRandRSetPreferredMode(void *out, void *m)
{
    RRModePtr mode;
    RROutputPtr output;
    int width, height;

    mode = m;
    output = out;

    width = mode->mode.width;
    height = mode->mode.height;

    /* Already the preferred mode? */
    if ((output->numModes >= 1) && (output->numPreferred == 1) &&
        (output->modes[0] == mode))
        return mode;

    /* Recreate the list, with the mode we want as preferred */
    vncRandRSetModes(output, width, height);

    /* Sanity check */
    if ((output->numModes >= 1) && (output->numPreferred == 1) &&
        (output->modes[0]->mode.width == width) &&
        (output->modes[0]->mode.height == height))
        return output->modes[0];

    /* Something went horribly wrong */
    return NULL;
}

static Bool
vncRandRInit(ScreenPtr pScreen)
{
    rrScrPrivPtr pScrPriv;
    RRCrtcPtr crtc;
    RRModePtr mode;
    Bool ret;

    if (!RRScreenInit(pScreen))
        return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);

    pScrPriv->rrGetInfo = vncRandRGetInfo;
    pScrPriv->rrSetConfig = NULL;
    pScrPriv->rrScreenSetSize = vncRandRScreenSetSize;
    pScrPriv->rrCrtcSet = vncRandRCrtcSet;
    pScrPriv->rrOutputValidateMode = vncRandROutputValidateMode;
    pScrPriv->rrModeDestroy = vncRandRModeDestroy;

    /* These are completely arbitrary */
    RRScreenSetSizeRange(pScreen, 32, 32, 32768, 32768);

    /*
     * Start with a single CRTC with a single output. More will be
     * allocated as needed...
     */
    crtc = vncRandRCrtcCreate(pScreen);

    /* Make sure the current screen size is the active mode */
    mode = vncRandRCreateMode(crtc->outputs[0],
                              pScreen->width, pScreen->height);
    if (mode == NULL)
        return FALSE;
    mode = vncRandRSetPreferredMode(crtc->outputs[0], mode);
    if (mode == NULL)
        return FALSE;

    ret = vncRandRCrtcSet(pScreen, crtc, mode, 0, 0, RR_Rotate_0,
                          crtc->numOutputs, crtc->outputs);
    if (!ret)
        return FALSE;

    return TRUE;
}

static Bool
vncScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    int dpi;
    int ret;
    void *pbits;

    /* 96 is the default used by most other systems */
    dpi = 96;
    if (monitorResolution)
        dpi = monitorResolution;

    pbits = vncAllocateFramebufferMemory(&vncScreenInfo.fb);
    if (!pbits)
        return FALSE;
    vncFbptr[0] = pbits;
    vncFbstride[0] = vncScreenInfo.fb.paddedWidth;

    miSetPixmapDepths();

    switch (vncScreenInfo.fb.depth) {
    case 16:
        miSetVisualTypesAndMasks(16,
                                 ((1 << TrueColor) |
                                  (1 << DirectColor)),
                                 8, TrueColor, 0xf800, 0x07e0, 0x001f);
        break;
    case 24:
        miSetVisualTypesAndMasks(24,
                                 ((1 << TrueColor) |
                                  (1 << DirectColor)),
                                 8, TrueColor, 0xff0000, 0x00ff00, 0x0000ff);
        break;
    case 32:
        miSetVisualTypesAndMasks(32,
                                 ((1 << TrueColor) |
                                  (1 << DirectColor)),
                                 8, TrueColor, 0xff000000, 0x00ff0000,
                                 0x0000ff00);
        break;
    default:
        return FALSE;
    }

    ret = fbScreenInit(pScreen, pbits,
                       vncScreenInfo.fb.width, vncScreenInfo.fb.height,
                       dpi, dpi, vncScreenInfo.fb.paddedWidth,
                       vncScreenInfo.fb.bitsPerPixel);

    if (ret && Render)
        ret = fbPictureInit(pScreen, 0, 0);

    if (!ret)
        return FALSE;

    ret = vncRandRInit(pScreen);
    if (!ret)
        return FALSE;

    pScreen->SaveScreen = vncSaveScreen;

    miPointerInitialize(pScreen, &vncPointerSpriteFuncs, &vncPointerCursorFuncs,
                        FALSE);

    if (!vncScreenInfo.pixelFormatDefined) {
        switch (vncScreenInfo.fb.depth) {
        case 16:
            vncScreenInfo.pixelFormatDefined = TRUE;
            vncScreenInfo.rgbNotBgr = TRUE;
            vncScreenInfo.blueBits = vncScreenInfo.redBits = 5;
            vncScreenInfo.greenBits = 6;
            break;
        case 24:
        case 32:
            vncScreenInfo.pixelFormatDefined = TRUE;
            vncScreenInfo.rgbNotBgr = TRUE;
            vncScreenInfo.blueBits = vncScreenInfo.redBits = vncScreenInfo.greenBits = 8;
            break;
        }
    }

    if (vncScreenInfo.pixelFormatDefined) {
        VisualPtr vis = pScreen->visuals;

        for (int i = 0; i < pScreen->numVisuals; i++) {
            if (vncScreenInfo.rgbNotBgr) {
                vis->offsetBlue = 0;
                vis->blueMask = (1 << vncScreenInfo.blueBits) - 1;
                vis->offsetGreen = vncScreenInfo.blueBits;
                vis->greenMask =
                    ((1 << vncScreenInfo.greenBits) - 1) << vis->offsetGreen;
                vis->offsetRed = vis->offsetGreen + vncScreenInfo.greenBits;
                vis->redMask = ((1 << vncScreenInfo.redBits) - 1) << vis->offsetRed;
            }
            else {
                vis->offsetRed = 0;
                vis->redMask = (1 << vncScreenInfo.redBits) - 1;
                vis->offsetGreen = vncScreenInfo.redBits;
                vis->greenMask =
                    ((1 << vncScreenInfo.greenBits) - 1) << vis->offsetGreen;
                vis->offsetBlue = vis->offsetGreen + vncScreenInfo.greenBits;
                vis->blueMask = ((1 << vncScreenInfo.blueBits) - 1) << vis->offsetBlue;
            }
            vis++;
        }
    }

    ret = fbCreateDefColormap(pScreen);
    if (!ret)
        return FALSE;

    return TRUE;

}                               /* end vncScreenInit */

static void
vncClientStateChange(CallbackListPtr *a, void *b, void *c)
{
    if (dispatchException & DE_RESET) {
        ErrorF("Warning: VNC extension does not support -reset, terminating instead. Use -noreset to prevent termination.\n");

        dispatchException |= DE_TERMINATE;
        dispatchException &= ~DE_RESET;
    }
}

#ifdef GLXEXT
#if XORG_OLDER_THAN(1, 20, 0)
extern void GlxExtensionInit(void);

static ExtensionModule glxExt = {
    GlxExtensionInit,
    "GLX",
    &noGlxExtension
};
#endif
#endif

void
InitOutput(ScreenInfo * scrInfo, int argc, char **argv)
{
    int i;
    int NumFormats = 0;

    vncPrintBanner();

#if XORG_AT_LEAST(1, 20, 0)
    xorgGlxCreateVendor();
#else

#ifdef GLXEXT
    if (serverGeneration == 1)
        LoadExtensionList(&glxExt, 1, TRUE);
#endif

#endif

    /* initialize pixmap formats */

    /* must have a pixmap depth to match every screen depth */
    vncPixmapDepths[vncScreenInfo.fb.depth] = TRUE;

    /* RENDER needs a good set of pixmaps. */
    if (Render) {
        vncPixmapDepths[1] = TRUE;
        vncPixmapDepths[4] = TRUE;
        vncPixmapDepths[8] = TRUE;
/*	vncPixmapDepths[15] = TRUE; */
        vncPixmapDepths[16] = TRUE;
        vncPixmapDepths[24] = TRUE;
        vncPixmapDepths[32] = TRUE;
    }

    for (i = 1; i <= 32; i++) {
        if (vncPixmapDepths[i]) {
            if (NumFormats >= MAXFORMATS)
                FatalError("MAXFORMATS is too small for this server\n");
            scrInfo->formats[NumFormats].depth = i;
            scrInfo->formats[NumFormats].bitsPerPixel = vncBitsPerPixel(i);
            scrInfo->formats[NumFormats].scanlinePad = BITMAP_SCANLINE_PAD;
            NumFormats++;
        }
    }

    scrInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    scrInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    scrInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    scrInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
    scrInfo->numPixmapFormats = NumFormats;

    /* initialize screen */

    if (AddScreen(vncScreenInit, argc, argv) == -1) {
        FatalError("Couldn't add screen\n");
    }

    if (!AddCallback(&ClientStateCallback, vncClientStateChange, 0)) {
        FatalError("AddCallback failed\n");
    }
}                               /* end InitOutput */

void
DDXRingBell(int percent, int pitch, int duration)
{
    if (percent > 0)
        vncBell();
}

Bool
LegalModifier(unsigned int key, DeviceIntPtr pDev)
{
    return TRUE;
}

void
ProcessInputEvents(void)
{
    mieqProcessInputEvents();
}

void
InitInput(int argc, char *argv[])
{
    mieqInit();
}

void
CloseInput(void)
{
}

void
vncClientGone(int fd)
{
    if (fd == vncInetdSock) {
        ErrorF("inetdSock client gone\n");
        GiveUp(0);
    }
}
