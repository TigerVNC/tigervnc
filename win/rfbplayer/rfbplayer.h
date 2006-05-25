/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- RfbPlayer.h

#include <windows.h>

#include <rfb_win32/DIBSectionBuffer.h>

#include <rfbplayer/resource.h>
#include <rfbplayer/PixelFormatList.h>
#include <rfbplayer/PlayerToolBar.h>
#include <rfbplayer/OptionsDialog.h>
#include <rfbplayer/rfbSessionReader.h>

using namespace rfb;
using namespace rfb::win32;

class PlayerToolBar;

class RfbPlayer : public RfbProto {
  public:
    RfbPlayer(char *fileName, PlayerOptions *options);
    ~RfbPlayer();

    // -=- Window Message handling

    LRESULT processMainMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT processFrameMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // -=- Window interface

    HWND getMainHandle() const {return mainHwnd;}
    HWND getFrameHandle() const {return frameHwnd;}
    void disableTBandMenuItems();
    void enableTBandMenuItems();
    void setFrameSize(int width, int height);
    void setVisible(bool visible);
    void setTitle(const char *title);
    void calculateScrollBars();
    void close(const char* reason=0);
    void init();

    // -=- Coordinate conversions

    inline Point bufferToClient(const Point& p) {
      Point pos = p;
      if (client_size.width() > buffer->width())
        pos.x += (client_size.width() - buffer->width()) / 2;
      else if (client_size.width() < buffer->width())
        pos.x -= scrolloffset.x;
      if (client_size.height() > buffer->height())
        pos.y += (client_size.height() - buffer->height()) / 2;
      else if (client_size.height() < buffer->height())
        pos.y -= scrolloffset.y;
      return pos;
    }
    inline Rect bufferToClient(const Rect& r) {
      return Rect(bufferToClient(r.tl), bufferToClient(r.br));
    }

    inline Point clientToBuffer(const Point& p) {
      Point pos = p;
      if (client_size.width() > buffer->width())
        pos.x -= (client_size.width() - buffer->width()) / 2;
      else if (client_size.width() < buffer->width())
        pos.x += scrolloffset.x;
      if (client_size.height() > buffer->height())
        pos.y -= (client_size.height() - buffer->height()) / 2;
      else if (client_size.height() < buffer->height())
        pos.y += scrolloffset.y;
      return pos;
    }
    inline Rect clientToBuffer(const Rect& r) {
      return Rect(clientToBuffer(r.tl), clientToBuffer(r.br));
    }

    bool setViewportOffset(const Point& tl);

    // -=- RfbProto interface overrides

    virtual void processMsg();
    virtual void serverInit();
    virtual void frameBufferUpdateEnd();
    virtual void setColourMapEntries(int first, int count, U16* rgbs);
    virtual void serverCutText(const char* str, int len);
    virtual void bell();

    virtual void beginRect(const Rect& r, unsigned int encoding);
    virtual void endRect(const Rect& r, unsigned int encoding);
    virtual void fillRect(const Rect& r, Pixel pix);
    virtual void imageRect(const Rect& r, void* pixels);
    virtual void copyRect(const Rect& r, int srcX, int srcY);

    // -=- Player functions

    // calculateSessionTime() calculates the full session time in sec
    long calculateSessionTime(char *fileName);

    // closeSessionFile() closes the session file and blanks the frame buffer
    void closeSessionFile();

    // openSessionFile() opens the new session file and starts play it
    void openSessionFile(char *fileName);

    // skipHandshaking() - is implemented to skip the initial handshaking when
    // perform backward seeking OR replaying.
    void skipHandshaking();

    void blankBuffer();
    void rewind();
    void setPaused(bool paused);
    void stopPlayback();
    long getTimeOffset();
    bool isSeekMode();
    bool isSeeking();
    bool isPaused();
    long getSeekOffset();
    void setPos(long pos);
    void setSpeed(double speed);
    double getSpeed();

  protected:
    bool seekMode;
    bool stopped;
    long lastPos;
    long sliderStepMs;
    char fullSessionTime[20];
    int time_pos_m;
    int time_pos_s;
    char *fileName;
    
    // rfbReader is a class which used to reading the rfb data from the file
    rfbSessionReader *rfbReader;

    // Returns true if part of the supplied rect is visible, false otherwise
    bool invalidateBufferRect(const Rect& crect);

    bool waitWhilePaused();

    // rewindFlag is a flag wich disable the update of the frame buffer window 
    // while the rewind is performing.
    bool rewindFlag;
    
    // Local window state
    HWND mainHwnd;
    HWND frameHwnd;
    HMENU hMenu;
    Rect window_size;
    Rect client_size;
    Point scrolloffset;
    Point maxscrolloffset;
    char *cutText;
    win32::DIBSectionBuffer* buffer;
    PlayerToolBar tb;

    // The player's parameters
    PlayerOptions options;
    PixelFormatList supportedPF;
    long imageDataStartTime;
    long sessionTimeMs;
    int currentEncoding;
};

// -=- sessionTerminateThread class

// It is a special thread class, wich is allow the rfbSessionReader class
// terminate itself.

class sessionTerminateThread : public rfb::Thread {
public:
  sessionTerminateThread(RfbPlayer *_player) : player(_player) {
    setDeleteAfterRun();
  }
  virtual void run() {
    player->closeSessionFile();
  }
protected:
  RfbPlayer *player;
};
