/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
#ifndef __RFB_MSGTYPES_H__
#define __RFB_MSGTYPES_H__

namespace rfb {
  // server to client

  const int msgTypeFramebufferUpdate = 0;
  const int msgTypeSetColourMapEntries = 1;
  const int msgTypeBell = 2;
  const int msgTypeServerCutText = 3;

  const int msgTypeFileListData = 130;
  const int msgTypeFileDownloadData = 131;
  const int msgTypeFileUploadCancel = 132;
  const int msgTypeFileDownloadFailed = 133;
  const int msgTypeFileDirSizeData = 134;
  const int msgTypeFileLastRequestFailed = 135;

  // client to server

  const int msgTypeSetPixelFormat = 0;
  const int msgTypeFixColourMapEntries = 1;
  const int msgTypeSetEncodings = 2;
  const int msgTypeFramebufferUpdateRequest = 3;
  const int msgTypeKeyEvent = 4;
  const int msgTypePointerEvent = 5;
  const int msgTypeClientCutText = 6;

  const int msgTypeFileListRequest = 130;
  const int msgTypeFileDownloadRequest = 131;
  const int msgTypeFileUploadRequest = 132;
  const int msgTypeFileUploadData = 133;
  const int msgTypeFileDownloadCancel = 134;
  const int msgTypeFileUploadFailed = 135;
  const int msgTypeFileCreateDirRequest = 136;
  const int msgTypeFileDirSizeRequest = 137;
  const int	msgTypeFileRenameRequest = 138;
  const int msgTypeFileDeleteRequest = 139;
}
#endif
