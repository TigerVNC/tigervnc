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
#include <winsock2.h>
#include <vncviewer/CViewManager.h>
#include <vncviewer/CView.h>
#include <vncviewer/ConnectionDialog.h>
#include <vncviewer/ConnectingDialog.h>
#include <rfb/Hostname.h>
#include <rfb/util.h>
#include <rfb/LogWriter.h>
#include <rfb/vncAuth.h>
#include <rdr/HexInStream.h>
#include <network/TcpSocket.h>

using namespace rfb;
using namespace win32;

static LogWriter vlog("CViewManager");


// -=- Custom thread class used internally

class CViewThread : public Thread {
public:
  CViewThread(network::Socket* s, CViewManager& cvm);
  CViewThread(const char* conninfo, CViewManager& cvm, bool infoIsConfigFile);
  virtual ~CViewThread();

  virtual void run();
protected:
  void setSocket(network::Socket* sock);

  network::Socket* sock;
  CharArray hostname;
  CViewManager& manager;

  bool useConfigFile;
};


CViewThread::CViewThread(network::Socket* s, CViewManager& cvm)
: Thread("CView"), sock(s), manager(cvm) {
  setDeleteAfterRun();
}

CViewThread::CViewThread(const char* h, CViewManager& cvm, bool hIsConfigFile)
: Thread("CView"), sock(0), manager(cvm), useConfigFile(hIsConfigFile) {
  setDeleteAfterRun();
  if (h) hostname.buf = strDup(h);
}


CViewThread::~CViewThread() {
  vlog.debug("~CViewThread");
  manager.remThread(this);
  delete sock;
}


void CViewThread::run() {
  try {
    CView view;
    view.setManager(&manager);

    if (!sock) {
      try {
        // If the hostname is actually a config filename then read it
        if (useConfigFile) {
          CharArray filename = hostname.takeBuf();
          CViewOptions options;
          options.readFromFile(filename.buf);
          if (options.host.buf)
            hostname.buf = strDup(options.host.buf);
          view.applyOptions(options);
        }

        // If there is no hostname then present the connection
        // dialog
        if (!hostname.buf) {
          ConnectionDialog conn(&view);
          if (!conn.showDialog())
            return;
          hostname.buf = strDup(conn.hostname.buf);

          // *** hack - Tell the view object the hostname
          CViewOptions opt(view.getOptions());
          opt.setHost(hostname.buf);
          view.applyOptions(opt);
        }

        // Parse the host name & port
        CharArray host;
        int port;
        getHostAndPort(hostname.buf, &host.buf, &port);

        // Attempt to connect
        ConnectingDialog dlg;
        // this is a nasty hack to get round a Win2K and later "feature" which
        // puts your second window in the background unless the first window
        // you put up actually gets some input.  Just generate a fake shift
        // event, which seems to do the trick.
        keybd_event(VK_SHIFT, MapVirtualKey(VK_SHIFT, 0), 0, 0);
        keybd_event(VK_SHIFT, MapVirtualKey(VK_SHIFT, 0), KEYEVENTF_KEYUP, 0);
        sock = new network::TcpSocket(host.buf, port);
      } catch(rdr::Exception& e) {
        vlog.error("unable to connect to %s (%s)", hostname, e.str());
        MsgBox(NULL, TStr(e.str()), MB_ICONERROR | MB_OK);
        return;
      }

      // Try to add the caller to the MRU
      MRU::addToMRU(hostname.buf);
    }

    view.initialise(sock);
    try {
      view.postQuitOnDestroy(true);
      while (true) {
        // - processMsg is designed to be callable in response to select().
        //   As a result, it can be called when FdInStream data is available,
        //   BUT there may be no actual RFB data available.  This is the case
        //   for example when reading data over an encrypted stream - an
        //   entire block must be read from the FdInStream before any data
        //   becomes available through the top-level encrypted stream.
        //   Since we are using blockCallback and not doing a select() here,
        //   we simply check() for some data on the top-level RFB stream.
        //   This ensures that processMsg will only be called when there is
        //   actually something to do.  In the meantime, blockCallback()
        //   will be called, keeping the user interface responsive.
        view.getInStream()->check(1,1);
        view.processMsg();
      }
    } catch (CView::QuitMessage& e) {
      // - Cope silently with WM_QUIT messages
      vlog.debug("QuitMessage received (wParam=%d)", e.wParam);
    } catch (rdr::EndOfStream& e) {
      // - Copy silently with disconnection if in NORMAL state
      if (view.state() == CConnection::RFBSTATE_NORMAL)
        vlog.debug(e.str());
      else {
        view.postQuitOnDestroy(false);
        throw rfb::Exception("server closed connection unexpectedly");
      }
    } catch (rdr::Exception&) {
      // - We MUST do this, otherwise ~CView will cause a
      //   PostQuitMessage and any MessageBox call will quit immediately.
      view.postQuitOnDestroy(false);
      throw;
    }
  } catch(rdr::Exception& e) {
    // - Something went wrong - display the error
    vlog.error("error: %s", e.str());
    MsgBox(NULL, TStr(e.str()), MB_ICONERROR | MB_OK);
  }
}


// -=- CViewManager itself

CViewManager::CViewManager()
: MsgWindow(_T("CViewManager")), threadsSig(threadsMutex),
  mainThread(Thread::self()) {
}

CViewManager::~CViewManager() {
  while (!socks.empty()) {
    network::SocketListener* sock = socks.front();
    delete sock;
    socks.pop_front();
  }
  awaitEmpty();
}


void CViewManager::awaitEmpty() {
  Lock l(threadsMutex);
  while (!threads.empty()) {
    threadsSig.wait(true);
  }
}


void CViewManager::addThread(Thread* t) {
  Lock l(threadsMutex);
  threads.push_front(t);
}

void CViewManager::remThread(Thread* t) {
  Lock l(threadsMutex);
  threads.remove(t);
  threadsSig.signal();

  // If there are no listening sockets then post a quit message when the
  // last client disconnects
  if (socks.empty())
    PostThreadMessage(mainThread->getThreadId(), WM_QUIT, 0, 0);
}


bool CViewManager::addClient(const char* hostinfo, bool isConfigFile) {
  CViewThread* thread = new CViewThread(hostinfo, *this, isConfigFile);
  addThread(thread);
  thread->start();
  return true;
}

bool CViewManager::addListener(network::SocketListener* sock) {
  socks.push_back(sock);
  WSAAsyncSelect(sock->getFd(), getHandle(), WM_USER, FD_ACCEPT);
  return true;
}

bool CViewManager::addDefaultTCPListener(int port) {
  return addListener(new network::TcpListener(port));
}


LRESULT CViewManager::processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_USER:
    std::list<network::SocketListener*>::iterator i;
    for (i=socks.begin(); i!=socks.end(); i++) {
      if (wParam == (*i)->getFd()) {
        network::Socket* new_sock = (*i)->accept();
        CharArray connname;
        connname.buf = new_sock->getPeerEndpoint();
        vlog.debug("accepted connection: %s", connname);
        CViewThread* thread = new CViewThread(new_sock, *this);
        addThread(thread);
        thread->start();
        break;
      }
    }
    break;
  }
  return MsgWindow::processMessage(msg, wParam, lParam);
}
