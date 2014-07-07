/* Copyright (C) 2011 TigerVNC Team.  All Rights Reserved.
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

#ifndef OS_W32TIGER_H
#define OS_W32TIGER_H

#ifdef WIN32 

#include <windows.h>
#include <wininet.h>
#include <shlobj.h>
#include <shlguid.h>
#include <wininet.h>


/* Windows has different names for these */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp


/* MSLLHOOKSTRUCT structure*/
#ifndef LLMHF_INJECTED
#define LLMHF_INJECTED          0x00000001
#endif


/* IActiveDesktop. As of 2011-10-12, MinGW does not define
   IActiveDesktop in any way (see tracker 2877129), while MinGW64 is
   broken: has the headers but not the lib symbols. */
#ifndef HAVE_ACTIVE_DESKTOP_H
extern const GUID CLSID_ActiveDesktop;
extern const GUID IID_IActiveDesktop;

/* IActiveDesktop::AddUrl */
#define ADDURL_SILENT		0x0001

/* IActiveDesktop::AddDesktopItemWithUI */
#define DTI_ADDUI_DEFAULT	0x00000000
#define DTI_ADDUI_DISPSUBWIZARD	0x00000001
#define DTI_ADDUI_POSITIONITEM	0x00000002

/* IActiveDesktop::ModifyDesktopItem */
#define COMP_ELEM_TYPE		0x00000001
#define COMP_ELEM_CHECKED	0x00000002
#define COMP_ELEM_DIRTY		0x00000004
#define COMP_ELEM_NOSCROLL	0x00000008
#define COMP_ELEM_POS_LEFT	0x00000010
#define COMP_ELEM_POS_TOP	0x00000020
#define COMP_ELEM_SIZE_WIDTH	0x00000040
#define COMP_ELEM_SIZE_HEIGHT	0x00000080
#define COMP_ELEM_POS_ZINDEX	0x00000100
#define COMP_ELEM_SOURCE	0x00000200
#define COMP_ELEM_FRIENDLYNAME	0x00000400
#define COMP_ELEM_SUBSCRIBEDURL	0x00000800
#define COMP_ELEM_ORIGINAL_CSI	0x00001000
#define COMP_ELEM_RESTORED_CSI	0x00002000
#define COMP_ELEM_CURITEMSTATE	0x00004000
#define COMP_ELEM_ALL		0x00007FFF /* OR-ed all COMP_ELEM_ */

/* IActiveDesktop::GetWallpaper */
#define AD_GETWP_BMP		0x00000000
#define AD_GETWP_IMAGE		0x00000001
#define AD_GETWP_LAST_APPLIED	0x00000002

/* IActiveDesktop::ApplyChanges */
#define AD_APPLY_SAVE		0x00000001
#define AD_APPLY_HTMLGEN	0x00000002
#define AD_APPLY_REFRESH	0x00000004
#define AD_APPLY_ALL		0x00000007 /* OR-ed three AD_APPLY_ above */
#define AD_APPLY_FORCE		0x00000008
#define AD_APPLY_BUFFERED_REFRESH 0x00000010
#define AD_APPLY_DYNAMICREFRESH	0x00000020

/* Structures for IActiveDesktop */
typedef struct {
	DWORD dwSize;
	int iLeft;
	int iTop;
	DWORD dwWidth;
	DWORD dwHeight;
	DWORD dwItemState;
} COMPSTATEINFO, *LPCOMPSTATEINFO;
typedef const COMPSTATEINFO *LPCCOMPSTATEINFO;

typedef struct {
	DWORD dwSize;
	int iLeft;
	int iTop;
	DWORD dwWidth;
	DWORD dwHeight;
	int izIndex;
	BOOL fCanResize;
	BOOL fCanResizeX;
	BOOL fCanResizeY;
	int iPreferredLeftPercent;
	int iPreferredTopPercent;
} COMPPOS, *LPCOMPPOS;
typedef const COMPPOS *LPCCOMPPOS;

typedef struct {
	DWORD dwSize;
	DWORD dwID;
	int iComponentType;
	BOOL fChecked;
	BOOL fDirty;
	BOOL fNoScroll;
	COMPPOS cpPos;
	WCHAR wszFriendlyName[MAX_PATH];
	WCHAR wszSource[INTERNET_MAX_URL_LENGTH];
	WCHAR wszSubscribedURL[INTERNET_MAX_URL_LENGTH];
	DWORD dwCurItemState;
	COMPSTATEINFO csiOriginal;
	COMPSTATEINFO csiRestored;
} COMPONENT, *LPCOMPONENT;
typedef const COMPONENT *LPCCOMPONENT;

typedef struct {
	DWORD dwSize;
	BOOL fEnableComponents;
	BOOL fActiveDesktop;
} COMPONENTSOPT, *LPCOMPONENTSOPT;
typedef const COMPONENTSOPT *LPCCOMPONENTSOPT;

typedef struct {
    DWORD dwSize;
    DWORD dwStyle;
} WALLPAPEROPT, *LPWALLPAPEROPT;
typedef const WALLPAPEROPT *LPCWALLPAPEROPT;

/* WALLPAPEROPT styles */
#define WPSTYLE_CENTER		0x0
#define WPSTYLE_TILE		0x1
#define WPSTYLE_STRETCH		0x2
#define WPSTYLE_MAX		0x3

/* Those two are defined in Windows 7 and newer, we don't need them now */
#if 0
#define WPSTYLE_KEEPASPECT	0x3
#define WPSTYLE_CROPTOFIT	0x4
#endif

#define INTERFACE IActiveDesktop
DECLARE_INTERFACE_(IActiveDesktop, IUnknown)
{
	STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;
	STDMETHOD(AddDesktopItem)(THIS_ LPCOMPONENT,DWORD) PURE;
	STDMETHOD(AddDesktopItemWithUI)(THIS_ HWND,LPCOMPONENT,DWORD) PURE;
	STDMETHOD(AddUrl)(THIS_ HWND,LPCWSTR,LPCOMPONENT,DWORD) PURE;
	STDMETHOD(ApplyChanges)(THIS_ DWORD) PURE;
	STDMETHOD(GenerateDesktopItemHtml)(THIS_ LPCWSTR,LPCOMPONENT,DWORD) PURE;
	STDMETHOD(GetDesktopItem)(THIS_ int,LPCOMPONENT,DWORD) PURE;
	STDMETHOD(GetDesktopItemByID)(THIS_ DWORD,LPCOMPONENT,DWORD) PURE;
	STDMETHOD(GetDesktopItemBySource)(THIS_ LPCWSTR,LPCOMPONENT,DWORD) PURE;
	STDMETHOD(GetDesktopItemCount)(THIS_ LPINT,DWORD) PURE;
	STDMETHOD(GetDesktopItemOptions)(THIS_ LPCOMPONENTSOPT,DWORD) PURE;
	STDMETHOD(GetPattern)(THIS_ LPWSTR,UINT,DWORD) PURE;
	STDMETHOD(GetWallpaper)(THIS_ LPWSTR,UINT,DWORD) PURE;
	STDMETHOD(GetWallpaperOptions)(THIS_ LPWALLPAPEROPT,DWORD) PURE;
	STDMETHOD(ModifyDesktopItem)(THIS_ LPCCOMPONENT,DWORD) PURE;
	STDMETHOD(RemoveDesktopItem)(THIS_ LPCCOMPONENT,DWORD) PURE;
	STDMETHOD(SetDesktopItemOptions)(THIS_ LPCCOMPONENTSOPT,DWORD) PURE;
	STDMETHOD(SetPattern)(THIS_ LPCWSTR,DWORD) PURE;
	STDMETHOD(SetWallpaper)(THIS_ LPCWSTR,DWORD) PURE;
	STDMETHOD(SetWallpaperOptions)(THIS_ LPCWALLPAPEROPT,DWORD) PURE;
};
#undef INTERFACE
#endif /* HAVE_ACTIVE_DESKTOP_H */

#endif /* WIN32 */
#endif /* OS_W32TIGER_H */
