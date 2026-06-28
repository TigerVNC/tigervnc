/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2009-2014 Pierre Ossman for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <fstream>
#include <vector>

#ifdef HAVE_GNUTLS
#include <gnutls/x509.h>
#endif

#include <core/Exception.h>
#include <core/LogWriter.h>
#include <core/Timer.h>
#include <core/i18n.h>
#include <core/string.h>
#include <core/time.h>
#include <core/xdgdirs.h>

#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>
#include <rdr/TLSException.h>

#include <rfb/CMsgWriter.h>
#include <rfb/CSecurity.h>
#include <rfb/Exception.h>
#include <rfb/Security.h>
#include <rfb/fenceTypes.h>
#include <rfb/obfuscate.h>
#include <rfb/screenTypes.h>

#include <network/TcpSocket.h>
#ifndef WIN32
#include <network/UnixSocket.h>
#endif

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "fltk/layout.h"
#include "fltk/util.h"
#include "AuthDialog.h"
#include "CConn.h"
#include "OptionsDialog.h"
#include "DesktopWindow.h"
#include "PlatformPixelBuffer.h"
#include "parameters.h"
#include "vncviewer.h"

std::string CConn::savedUsername;
std::string CConn::savedPassword;

#ifdef WIN32
#include "win32.h"
#endif

static core::LogWriter vlog("CConn");

// Persistent per-server password storage. Passwords are stored
// obfuscated (the same weak scheme used by PasswordFile) under the VNC
// config directory, keyed by server host and port. This is convenience,
// not strong security: anyone with read access to the files can recover
// the password.

static std::string credentialKey(const std::string& host, int port)
{
  std::string key;

  for (char c : host) {
    if (isalnum((unsigned char)c) || (c == '.') || (c == '-'))
      key += c;
    else
      key += '_';
  }
  if (port != 0)
    key += core::format("_%d", port);

  return key;
}

static std::string credentialBase(const std::string& host, int port)
{
  const char* configDir = core::getvncconfigdir();
  if (configDir == nullptr)
    return "";
  return std::string(configDir) + "/saved-passwords/" +
         credentialKey(host, port);
}

// rfb::obfuscate() only handles the 8 bytes used by classic VNC auth.
// Process the password in 8-byte blocks so passwords of any length (e.g.
// for VeNCrypt/RSA-AES username+password auth) survive a round-trip. A
// single 8-byte file (the previous format) still decodes correctly.

static std::vector<uint8_t> obfuscatePassword(const std::string& password)
{
  std::vector<uint8_t> out;

  for (size_t i = 0; i < password.size(); i += 8) {
    std::vector<uint8_t> block = rfb::obfuscate(password.c_str() + i);
    out.insert(out.end(), block.begin(), block.end());
  }

  return out;
}

static std::string deobfuscatePassword(const std::vector<uint8_t>& data)
{
  std::string result;

  // Each block deobfuscates to up to 8 characters, terminating early on
  // the zero padding of the final block (passwords contain no NUL bytes).
  for (size_t i = 0; i < data.size(); i += 8)
    result += rfb::deobfuscate(data.data() + i, 8);

  return result;
}

static bool loadSavedCredentials(const std::string& host, int port,
                                 bool wantUser, std::string* user,
                                 std::string* password)
{
  std::string base;
  FILE* fp;
  std::vector<uint8_t> obfPwd;
  uint8_t buf[8];
  size_t n;

  base = credentialBase(host, port);
  if (base.empty())
    return false;

  fp = fopen((base + ".passwd").c_str(), "rb");
  if (!fp)
    return false;
  while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
    obfPwd.insert(obfPwd.end(), buf, buf + n);
  fclose(fp);

  if ((obfPwd.size() == 0) || ((obfPwd.size() % 8) != 0))
    return false;

  try {
    *password = deobfuscatePassword(obfPwd);
  } catch (std::exception&) {
    return false;
  }

  if (wantUser && user) {
    fp = fopen((base + ".user").c_str(), "r");
    if (fp) {
      char userBuf[256];
      if (fgets(userBuf, sizeof(userBuf), fp)) {
        size_t len = strlen(userBuf);
        while ((len > 0) &&
               ((userBuf[len-1] == '\n') || (userBuf[len-1] == '\r')))
          userBuf[--len] = '\0';
        *user = userBuf;
      }
      fclose(fp);
    }
  }

  return true;
}

static void clearSavedCredentials(const std::string& host, int port)
{
  std::string base;

  base = credentialBase(host, port);
  if (base.empty())
    return;

  remove((base + ".passwd").c_str());
  remove((base + ".user").c_str());
}

static void saveCredentials(const std::string& host, int port,
                            const std::string* user,
                            const std::string& password)
{
  const char* configDir;
  std::string dir, base;
  std::vector<uint8_t> obfPwd;
  FILE* fp;

  configDir = core::getvncconfigdir();
  if (configDir == nullptr) {
    vlog.error(_("Could not determine VNC config directory path"));
    return;
  }

  dir = std::string(configDir) + "/saved-passwords";
  if ((core::mkdir_p(dir.c_str(), 0700) == -1) && (errno != EEXIST)) {
    vlog.error(_("Failed to create directory \"%s\": %s"),
               dir.c_str(), strerror(errno));
    return;
  }

  base = dir + "/" + credentialKey(host, port);

  obfPwd = obfuscatePassword(password);
  fp = fopen((base + ".passwd").c_str(), "wb");
  if (!fp) {
    vlog.error(_("Failed to open \"%s\": %s"),
               (base + ".passwd").c_str(), strerror(errno));
    return;
  }
  fwrite(obfPwd.data(), 1, obfPwd.size(), fp);
  fclose(fp);
  chmod((base + ".passwd").c_str(), 0600);

  if (user && !user->empty()) {
    fp = fopen((base + ".user").c_str(), "w");
    if (fp) {
      fprintf(fp, "%s\n", user->c_str());
      fclose(fp);
      chmod((base + ".user").c_str(), 0600);
    }
  } else {
    // No username for this server; make sure a stale one doesn't linger
    remove((base + ".user").c_str());
  }
}

// 8 colours (1 bit per component)
static const rfb::PixelFormat verylowColourPF(8, 3,false, true,
                                              1, 1, 1, 2, 1, 0);
// 64 colours (2 bits per component)
static const rfb::PixelFormat lowColourPF(8, 6, false, true,
                                          3, 3, 3, 4, 2, 0);
// 256 colours (2-3 bits per component)
static const rfb::PixelFormat mediumColourPF(8, 8, false, true,
                                             7, 7, 3, 5, 2, 0);

// Time new bandwidth estimates are weighted against (in ms)
static const unsigned bpsEstimateWindow = 1000;

CConn::CConn()
  : serverPort(0), sock(nullptr),
    msgTimer(this, &CConn::processNextMsg), desktop(nullptr),
    updateCount(0), pixelCount(0),
    lastServerEncoding((unsigned int)-1), bpsEstimate(20000000)
{
  setShared(::shared);

  supportsLocalCursor = true;
  supportsCursorPosition = true;
  supportsDesktopResize = true;
  supportsLEDState = true;

  if (customCompressLevel)
    setCompressLevel(::compressLevel);

  setQualityLevel(::qualityLevel);

  OptionsDialog::addCallback(handleOptions, this);
}

CConn::~CConn()
{
  close();

  OptionsDialog::removeCallback(handleOptions);
  Fl::remove_timeout(handleUpdateTimeout, this);

  if (desktop)
    delete desktop;

  if (sock) {
    struct timeval now;

    sock->shutdownWrite();

    // Do a graceful close by waiting for the peer (up to 250 ms)
    // FIXME: should do this asynchronously
    gettimeofday(&now, nullptr);
    while (core::msSince(&now) < 250) {
      bool done;

      done = false;
      while (true) {
        try {
          sock->inStream().skip(sock->inStream().avail());
          if (!sock->inStream().hasData(1))
            break;
        } catch (std::exception&) {
          done = true;
          break;
        }
      }

      if (done)
        break;

  #ifdef WIN32
      Sleep(10);
  #else
      usleep(10000);
  #endif
    }

    Fl::remove_fd(sock->getFd());

    delete sock;
  }
}

void CConn::connect(const char* vncServerName, network::Socket* socket)
{
  sock = socket;
  if(sock == nullptr) {
    try {
#ifndef WIN32
      if (strchr(vncServerName, '/') != nullptr) {
        sock = new network::UnixSocket(vncServerName);
        serverHost = sock->getPeerAddress();
        vlog.info(_("Connected to socket %s"), serverHost.c_str());
      } else
#endif
      {
        network::getHostAndPort(vncServerName, &serverHost, &serverPort);

        sock = new network::TcpSocket(serverHost.c_str(), serverPort);
        vlog.info(_("Connected to host %s port %d"),
                  serverHost.c_str(), serverPort);
      }
    } catch (std::exception& e) {
      vlog.error("%s", e.what());
      abort_connection(_("Failed to connect to \"%s\":\n\n%s"),
                       vncServerName, e.what());
      return;
    }
  }

  Fl::add_fd(sock->getFd(), FL_READ | FL_EXCEPT, socketEvent, this);

  setServerName(serverHost.c_str());
  setStreams(&sock->inStream(), &sock->outStream());

  initialiseProtocol();
}

std::string CConn::connectionInfo()
{
  std::string infoText;

  char pfStr[100];

  infoText += core::format(_("Desktop name: %.80s"), server.name());
  infoText += "\n";

  infoText += core::format(_("Host: %.80s port: %d"),
                           serverHost.c_str(), serverPort);
  infoText += "\n";

  infoText += core::format(_("Size: %d x %d"),
                           server.width(), server.height());
  infoText += "\n";

  // TRANSLATORS: Will be filled in with a string describing the
  // protocol pixel format in a fairly language neutral way
  server.pf().print(pfStr, 100);
  infoText += core::format(_("Pixel format: %s"), pfStr);
  infoText += "\n";

  infoText += core::format(_("Requested encoding: %s"),
                           rfb::encodingName(getPreferredEncoding()));
  infoText += "\n";

  infoText += core::format(_("Last used encoding: %s"),
                           rfb::encodingName(lastServerEncoding));
  infoText += "\n";

  infoText += core::format(_("Line speed estimate: %d kbit/s"),
                           (int)(bpsEstimate / 1000));
  infoText += "\n";

  infoText += core::format(_("Protocol version: %d.%d"),
                           server.majorVersion, server.minorVersion);
  infoText += "\n";

  infoText += core::format(_("Security method: %s"),
                           rfb::secTypeName(csecurity->getType()));
  infoText += "\n";

  return infoText;
}

unsigned CConn::getUpdateCount()
{
  return updateCount;
}

unsigned CConn::getPixelCount()
{
  return pixelCount;
}

unsigned CConn::getPosition()
{
  return sock->inStream().pos();
}

void CConn::socketEvent(FL_SOCKET fd, void *data)
{
  CConn *cc;

  assert(data);
  cc = (CConn*)data;

  // Stop monitoring the socket for now and start processing incoming
  // data asynchronously
  Fl::remove_fd(fd);
  cc->msgTimer.start(0);

  // Coalesce data until we're fully done processing things
  cc->getOutStream()->cork(true);
}

void CConn::processNextMsg(core::Timer*)
{
  static bool recursing = false;
  bool again;
  int when;

  // I don't think processMsg() is recursion safe, so add this check
  assert(!recursing);

  recursing = true;

  again = false;
  try {
    // We might have been called to flush unwritten socket data
    sock->outStream().flush();

    again = processMsg();
  } catch (rdr::end_of_stream& e) {
    vlog.info("%s", e.what());
    if (!desktop) {
      vlog.error(_("The connection was dropped by the server before "
                   "the session could be established."));
      abort_connection(_("The connection was dropped by the server "
                       "before the session could be established."));
    } else {
      disconnect();
    }
  } catch (rfb::auth_cancelled& e) {
    vlog.info("%s", e.what());
    disconnect();
  } catch (rfb::auth_error& e) {
    savedUsername.clear();
    savedPassword.clear();
    // The stored password (if any) was rejected, so discard it to avoid
    // getting stuck reusing bad credentials on the next connection
    clearSavedCredentials(serverHost, serverPort);
    vlog.error(_("Authentication failed: %s"), e.what());
    abort_connection(_("Failed to authenticate with the server. Reason "
                       "given by the server:\n\n%s"), e.what());
  } catch (std::exception& e) {
    vlog.error("%s", e.what());
    abort_connection_with_unexpected_error(e);
  }

  recursing = false;

  if (again) {
    msgTimer.repeat();
    return;
  }

  // Out of data, go back to waiting

  getOutStream()->cork(false);

  when = FL_READ | FL_EXCEPT;
  if (sock->outStream().hasBufferedData())
    when |= FL_WRITE;

  Fl::add_fd(sock->getFd(), when, socketEvent, this);
}

////////////////////// CConnection callback methods //////////////////////

void CConn::getUserPasswd(bool secure, std::string *user,
                          std::string *password)
{
  const char *passwordFileName(passwordFile);
  int ret_val;

  assert(password);
  char *envUsername = getenv("VNC_USERNAME");
  char *envPassword = getenv("VNC_PASSWORD");

  if(user && envUsername && envPassword) {
    *user = envUsername;
    *password = envPassword;
    return;
  }

  if (!user && envPassword) {
    *password = envPassword;
    return;
  }

  if (user && !savedUsername.empty() && !savedPassword.empty()) {
    *user = savedUsername;
    *password = savedPassword;
    return;
  }

  if (!user && !savedPassword.empty()) {
    *password = savedPassword;
    return;
  }

  if (!user && passwordFileName[0]) {
    std::vector<uint8_t> obfPwd(8);
    FILE* fp;

    fp = fopen(passwordFileName, "rb");
    if (!fp)
      throw core::posix_error(
        core::format(_("Failed to open \"%s\""), passwordFileName),
        errno);

    obfPwd.resize(fread(obfPwd.data(), 1, obfPwd.size(), fp));
    fclose(fp);

    *password = rfb::deobfuscate(obfPwd.data(), obfPwd.size());

    return;
  }

  // Saved to disk during a previous session for this specific server?
  {
    std::string diskUser, diskPwd;
    if (loadSavedCredentials(serverHost, serverPort,
                             user != nullptr, &diskUser, &diskPwd)) {
      // Only reuse silently if we have everything the server needs
      if (!user || !diskUser.empty()) {
        if (user)
          *user = diskUser;
        *password = diskPwd;
        return;
      }
    }
  }

  // Pre-check the "remember" box if we already have something saved for
  // this server (e.g. we're re-prompting after a password change)
  std::string tmpPwd;
  bool hadSaved = loadSavedCredentials(serverHost, serverPort, false,
                                       nullptr, &tmpPwd);

  AuthDialog d(secure, user != nullptr, password != nullptr, hadSaved);
  d.show();
  while (d.shown())
    Fl::wait();
  ret_val = d.result();

  if (ret_val == 1) {
    bool keepPasswd;

    if (reconnectOnError)
      keepPasswd = d.getKeepPassword();
    else
      keepPasswd = false;

    if (user) {
      *user = d.getUser();
      if (keepPasswd)
        savedUsername = d.getUser();
    }
    *password = d.getPassword();
    if (keepPasswd)
      savedPassword = d.getPassword();

    if (d.getSavePassword()) {
      std::string savedUser = user ? *user : "";
      saveCredentials(serverHost, serverPort,
                      user ? &savedUser : nullptr, *password);
    } else {
      // Unchecked: drop any credentials saved for this server before
      clearSavedCredentials(serverHost, serverPort);
    }
  }

  if (ret_val != 1)
    throw rfb::auth_cancelled();
}

// initDone() is called when the serverInit message has been received.  At
// this point we create the desktop window and display it.  We also tell the
// server the pixel format and encodings to use and request the first update.
void CConn::initDone()
{
  // If using AutoSelect with old servers, start in FullColor
  // mode. See comment in autoSelectFormatAndEncoding. 
  if (server.beforeVersion(3, 8) && autoSelect)
    fullColour.setParam(true);

  desktop = new DesktopWindow(server.width(), server.height(), this);
  fullColourPF = desktop->getPreferredPF();

  // Force a switch to the format and encoding we'd like
  updateEncoding();
  updatePixelFormat();
}

bool CConn::verifyCertificate(unsigned int status,
                              const uint8_t* certificate, size_t length)
{
#ifndef HAVE_GNUTLS
  (void)status;
  (void)certificate;
  (void)length;
  throw std::logic_error("TLS support not enabled");
#else
  const unsigned allowed_errors =
    GNUTLS_CERT_INVALID |
    GNUTLS_CERT_SIGNER_NOT_FOUND |
    GNUTLS_CERT_SIGNER_NOT_CA |
    GNUTLS_CERT_NOT_ACTIVATED |
    GNUTLS_CERT_EXPIRED |
    GNUTLS_CERT_INSECURE_ALGORITHM |
    GNUTLS_CERT_UNEXPECTED_OWNER;
  gnutls_datum_t status_str;
  unsigned int fatal_status;

  gnutls_datum_t cert_datum;
  gnutls_x509_crt_t crt;
  int err, known;

  const char *hostsDir;
  gnutls_datum_t info_datum;
  std::string info;
  size_t len;

  assert(status != 0);

  fatal_status = status & (~allowed_errors);

  if (fatal_status != 0) {
    err = gnutls_certificate_verification_status_print(fatal_status,
                                                       GNUTLS_CRT_X509,
                                                       &status_str,
                                                       0);
    if (err != GNUTLS_E_SUCCESS)
      throw rdr::tls_error(_("Failed to get certificate problem "
                             "description"), err);

    abort_connection(_("Invalid server certificate: %s"),
                     status_str.data);

    gnutls_free(status_str.data);

    return false;
  }

  err = gnutls_certificate_verification_status_print(status,
                                                     GNUTLS_CRT_X509,
                                                     &status_str,
                                                     0);
  if (err != GNUTLS_E_SUCCESS)
    throw rdr::tls_error(_("Failed to get certificate problem "
                           "description"), err);

  vlog.info(_("Server certificate problems: %s"), status_str.data);

  gnutls_free(status_str.data);

  /* Certificate has some user overridable problems, so TOFU time */

  hostsDir = core::getvncstatedir();
  if (hostsDir == nullptr) {
    throw std::runtime_error(_("Could not determine VNC state "
                               "directory path"));
  }

  std::string dbPath;
  dbPath = (std::string)hostsDir + "/x509_known_hosts";

  cert_datum.data = (uint8_t*)certificate;
  cert_datum.size = length;

  known = gnutls_verify_stored_pubkey(dbPath.c_str(), nullptr,
                                      getServerName(), nullptr,
                                      GNUTLS_CRT_X509, &cert_datum, 0);

  /* Previously known? */
  if (known == GNUTLS_E_SUCCESS) {
    vlog.info(_("Server has an existing security exception"));
    return true;
  }

  if ((known != GNUTLS_E_NO_CERTIFICATE_FOUND) &&
      (known != GNUTLS_E_CERTIFICATE_KEY_MISMATCH))
    throw rdr::tls_error(_("Failed to load list of servers with a "
                           "security exception"), known);

  gnutls_x509_crt_init(&crt);
  err = gnutls_x509_crt_import(crt, &cert_datum, GNUTLS_X509_FMT_DER);
  if (err != GNUTLS_E_SUCCESS)
    throw rdr::tls_error(_("Failed to decode server certificate"), err);

  err = gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE,
                              &info_datum);
  gnutls_x509_crt_deinit(crt);
  if (err != GNUTLS_E_SUCCESS)
    throw rdr::tls_error(_("Failed to format server certificate for "
                           "display"), err);

  len = strlen((char*)info_datum.data);
  for (size_t i = 0; i < len - 1; i++) {
    if (info_datum.data[i] == ',' && info_datum.data[i + 1] == ' ')
      info_datum.data[i] = '\n';
  }

  info = (const char*)info_datum.data;

  gnutls_free(info_datum.data);

  /* New host */
  if (known == GNUTLS_E_NO_CERTIFICATE_FOUND) {
    std::string text;

    vlog.info(_("Server host is not previously known"));
    vlog.info("%s", info.c_str());

    if (status & (GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA)) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This certificate has been signed by an unknown authority:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Unknown certificate issuer"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~(GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA);
    }

    if (status & GNUTLS_CERT_NOT_ACTIVATED) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This certificate is not yet valid:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Certificate is not yet valid"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_NOT_ACTIVATED;
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This certificate has expired:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Expired certificate"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_EXPIRED;
    }

    if (status & GNUTLS_CERT_INSECURE_ALGORITHM) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This certificate uses an insecure algorithm:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Insecure certificate algorithm"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_INSECURE_ALGORITHM;
    }

    if (status & GNUTLS_CERT_UNEXPECTED_OWNER) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        core::format(
          _("The specified hostname \"%s\" does not match the "
            "certificate provided by the server:"),
          getServerName()).c_str(),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Certificate hostname mismatch"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_UNEXPECTED_OWNER;
    }

    if (status != 0) {
      vlog.error("Unhandled server certificate problems: 0x%x",
                 status);
      throw std::logic_error("Unhandled server certificate problems");
    }
  } else if (known == GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
    std::string text;

    vlog.info(_("Server host certificate has changed"));
    vlog.info("%s", info.c_str());

    if (status & (GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA)) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This host is previously known with a different certificate, "
          "and the new certificate has been signed by an unknown "
          "authority:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Unexpected server certificate"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~(GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA);
    }

    if (status & GNUTLS_CERT_NOT_ACTIVATED) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This host is previously known with a different certificate, "
          "and the new certificate is not yet valid:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Unexpected server certificate"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_NOT_ACTIVATED;
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This host is previously known with a different certificate, "
          "and the new certificate has expired:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Unexpected server certificate"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_EXPIRED;
    }

    if (status & GNUTLS_CERT_INSECURE_ALGORITHM) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        _("This host is previously known with a different certificate, "
          "and the new certificate uses an insecure algorithm:"),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Unexpected server certificate"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_INSECURE_ALGORITHM;
    }

    if (status & GNUTLS_CERT_UNEXPECTED_OWNER) {
      text = core::format(
        "%s\n\n%s\n\n%s\n\n%s",
        core::format(
          _("This host is previously known with a different "
            "certificate, and the specified hostname \"%s\" does not "
            "match the new certificate provided by the server:"),
          getServerName()).c_str(),
        info.c_str(),
        _("Someone could be trying to impersonate the site and you "
          "should not continue."),
        _("Do you want to make an exception for this server?"));

      fl_message_title(_("Unexpected server certificate"));
      if (fl_choice("%s", nullptr, fl_cancel, _("Add exception"),
                    fltk_escape(text).c_str()) != 2)
        return false;

      status &= ~GNUTLS_CERT_UNEXPECTED_OWNER;
    }

    if (status != 0) {
      vlog.error("Unhandled server certificate problems: 0x%x",
                 status);
      throw std::logic_error("Unhandled server certificate problems");
    }
  }

  if (gnutls_store_pubkey(dbPath.c_str(), nullptr,
                          getServerName(), nullptr,
                          GNUTLS_CRT_X509, &cert_datum, 0, 0))
    vlog.error(_("Failed to store server certificate to list of "
                 "servers with a security exception"));

  vlog.info(_("Security exception added for server host"));

  return true;
#endif
}

// Trust-on-first-use storage for non-X.509 server keys (e.g. the RSA key
// used by the RSA-AES security types). The accepted key is remembered so
// the user isn't asked to verify the same fingerprint on every connection,
// much like SSH's known_hosts. Stored as "<servername> <hexkey>" lines.

static std::string hostKeyPath()
{
  const char* stateDir = core::getvncstatedir();
  if (stateDir == nullptr)
    return "";
  return std::string(stateDir) + "/known_host_keys";
}

static std::string hexEncode(const uint8_t* data, size_t length)
{
  static const char digits[] = "0123456789abcdef";
  std::string out;

  out.reserve(length * 2);
  for (size_t i = 0; i < length; i++) {
    out += digits[(data[i] >> 4) & 0xf];
    out += digits[data[i] & 0xf];
  }

  return out;
}

// 0 = host not known, 1 = known and key matches, -1 = known but key differs
static int lookupHostKey(const std::string& serverName,
                         const std::string& keyHex)
{
  std::string path;
  std::ifstream f;
  std::string line;

  path = hostKeyPath();
  if (path.empty())
    return 0;

  f.open(path);
  if (!f.is_open())
    return 0;

  while (std::getline(f, line)) {
    size_t sep = line.find(' ');
    if (sep == std::string::npos)
      continue;
    if (line.substr(0, sep) == serverName)
      return (line.substr(sep + 1) == keyHex) ? 1 : -1;
  }

  return 0;
}

static void storeHostKey(const std::string& serverName,
                         const std::string& keyHex)
{
  const char* stateDir;
  std::string path;
  std::vector<std::string> lines;

  stateDir = core::getvncstatedir();
  if (stateDir == nullptr) {
    vlog.error(_("Could not determine VNC state directory path"));
    return;
  }

  if ((core::mkdir_p(stateDir, 0700) == -1) && (errno != EEXIST)) {
    vlog.error(_("Failed to create directory \"%s\": %s"),
               stateDir, strerror(errno));
    return;
  }

  path = std::string(stateDir) + "/known_host_keys";

  // Preserve other hosts, dropping any previous entry for this server
  {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
      size_t sep = line.find(' ');
      if (sep == std::string::npos)
        continue;
      if (line.substr(0, sep) == serverName)
        continue;
      lines.push_back(line);
    }
  }
  lines.push_back(serverName + " " + keyHex);

  std::ofstream out(path, std::ios::trunc);
  if (!out.is_open()) {
    vlog.error(_("Failed to open \"%s\": %s"), path.c_str(),
               strerror(errno));
    return;
  }
  for (const std::string& line : lines)
    out << line << "\n";
  out.close();

  chmod(path.c_str(), 0600);
}

bool CConn::verifyHostKey(const uint8_t* key, size_t length,
                          const char* fingerprint)
{
  std::string keyHex;
  std::string text;
  int known;

  keyHex = hexEncode(key, length);
  known = lookupHostKey(getServerName(), keyHex);

  /* Previously accepted and unchanged? */
  if (known == 1) {
    vlog.info(_("Server has an existing security exception"));
    return true;
  }

  if (known == -1) {
    /* Key has changed since last time - this could be an attack */
    text = core::format(
      _("The server has changed its key since you last connected:\n"
        "\n"
        "Fingerprint: %s\n"
        "\n"
        "Someone could be trying to impersonate the server and you "
        "should not continue unless you know the key was changed "
        "intentionally.\n"
        "\n"
        "Do you want to trust the new key and continue connecting?"),
      fingerprint);
    fl_message_title(_("Unexpected server key"));
  } else {
    text = core::format(
      _("The server has provided the following identifying information:\n"
        "\n"
        "Fingerprint: %s\n"
        "\n"
        "Do you want to continue connecting to this server?"),
      fingerprint);
    fl_message_title(_("Verify server key"));
  }

  if (fl_choice("%s", nullptr, fl_cancel, _("Continue"),
                fltk_escape(text).c_str()) != 2)
    return false;

  // Trust on first use: remember the key so we don't ask again
  storeHostKey(getServerName(), keyHex);
  vlog.info(_("Security exception added for server host"));

  return true;
}

void CConn::setExtendedDesktopSize(unsigned reason, unsigned result,
                                   int w, int h,
                                   const rfb::ScreenSet& layout)
{
  CConnection::setExtendedDesktopSize(reason, result, w, h, layout);

  if (reason == rfb::reasonClient)
    desktop->setDesktopSizeDone(result);
}

// setName() is called when the desktop name changes
void CConn::setName(const char* name)
{
  CConnection::setName(name);
  desktop->updateCaption();
}

// framebufferUpdateStart() is called at the beginning of an update.
// Here we try to send out a new framebuffer update request so that the
// next update can be sent out in parallel with us decoding the current
// one.
void CConn::framebufferUpdateStart()
{
  CConnection::framebufferUpdateStart();

  // For bandwidth estimate
  gettimeofday(&updateStartTime, nullptr);
  updateStartPos = sock->inStream().pos();

  // Update the screen prematurely for very slow updates
  Fl::add_timeout(1.0, handleUpdateTimeout, this);
}

// framebufferUpdateEnd() is called at the end of an update.
// For each rectangle, the FdInStream will have timed the speed
// of the connection, allowing us to select format and encoding
// appropriately, and then request another incremental update.
void CConn::framebufferUpdateEnd()
{
  unsigned long long elapsed, bps, weight;
  struct timeval now;

  CConnection::framebufferUpdateEnd();

  updateCount++;

  // Calculate bandwidth everything managed to maintain during this update
  gettimeofday(&now, nullptr);
  elapsed = (now.tv_sec - updateStartTime.tv_sec) * 1000000;
  elapsed += now.tv_usec - updateStartTime.tv_usec;
  if (elapsed == 0)
    elapsed = 1;
  bps = (unsigned long long)(sock->inStream().pos() -
                             updateStartPos) * 8 *
                            1000000 / elapsed;
  // Allow this update to influence things more the longer it took, to a
  // maximum of 20% of the new value.
  weight = elapsed * 1000 / bpsEstimateWindow;
  if (weight > 200000)
    weight = 200000;
  bpsEstimate = ((bpsEstimate * (1000000 - weight)) +
                 (bps * weight)) / 1000000;

  Fl::remove_timeout(handleUpdateTimeout, this);
  desktop->updateWindow();

  // Compute new settings based on updated bandwidth values
  if (autoSelect) {
    updateEncoding();
    updateQualityLevel();
    updatePixelFormat();
  }
}

// The rest of the callbacks are fairly self-explanatory...

void CConn::bell()
{
  fl_beep();
}

bool CConn::dataRect(const core::Rect& r, int encoding)
{
  bool ret;

  if (encoding != rfb::encodingCopyRect)
    lastServerEncoding = encoding;

  ret = CConnection::dataRect(r, encoding);

  if (ret)
    pixelCount += r.area();

  return ret;
}

void CConn::setCursor(int width, int height, const core::Point& hotspot,
                      const uint8_t* data)
{
  CConnection::setCursor(width, height, hotspot, data);

  desktop->setCursor();
}

void CConn::setCursorPos(const core::Point& pos)
{
  desktop->setCursorPos(pos);
}

void CConn::setLEDState(unsigned int state)
{
  CConnection::setLEDState(state);

  desktop->setLEDState(state);
}

void CConn::handleClipboardRequest()
{
  desktop->handleClipboardRequest();
}

void CConn::handleClipboardAnnounce(bool available)
{
  desktop->handleClipboardAnnounce(available);
}

void CConn::handleClipboardData(const char* data)
{
  desktop->handleClipboardData(data);
}


////////////////////// Internal methods //////////////////////

void CConn::resizeFramebuffer()
{
  desktop->resizeFramebuffer(server.width(), server.height());
}

void CConn::updateEncoding()
{
  int encNum;

  if (autoSelect)
    encNum = rfb::encodingTight;
  else
    encNum = rfb::encodingNum(::preferredEncoding.getValueStr().c_str());

  if (encNum != -1)
    setPreferredEncoding(encNum);
}

void CConn::updateCompressLevel()
{
  if (customCompressLevel)
    setCompressLevel(::compressLevel);
  else
    setCompressLevel(-1);
}

void CConn::updateQualityLevel()
{
  int newQualityLevel;

  if (!autoSelect)
    newQualityLevel = ::qualityLevel;
  else {
    // Above 16Mbps (i.e. LAN), we choose the second highest JPEG
    // quality, which should be perceptually lossless. If the bandwidth
    // is below that, we choose a more lossy JPEG quality.

    if (bpsEstimate > 16000000)
      newQualityLevel = 8;
    else
      newQualityLevel = 6;

    if (newQualityLevel != getQualityLevel()) {
      vlog.info(_("Throughput %d kbit/s - changing to quality %d"),
                (int)(bpsEstimate/1000), newQualityLevel);
    }
  }

  setQualityLevel(newQualityLevel);
}

void CConn::updatePixelFormat()
{
  bool useFullColour;
  rfb::PixelFormat pf;

  if (server.beforeVersion(3, 8)) {
    // Xvnc from TightVNC 1.2.9 sends out FramebufferUpdates with
    // cursors "asynchronously". If this happens in the middle of a
    // pixel format change, the server will encode the cursor with
    // the old format, but the client will try to decode it
    // according to the new format. This will lead to a
    // crash. Therefore, we do not allow automatic format change for
    // old servers.
    return;
  }

  useFullColour = fullColour;

  // If the bandwidth drops below 256 Kbps, we switch to palette mode.
  if (autoSelect) {
    useFullColour = (bpsEstimate > 256000);
    if (useFullColour != (server.pf() == fullColourPF)) {
      if (useFullColour)
        vlog.info(_("Throughput %d kbit/s - full color is now enabled"),
                  (int)(bpsEstimate/1000));
      else
        vlog.info(_("Throughput %d kbit/s - full color is now disabled"),
                  (int)(bpsEstimate/1000));
    }
  }

  if (useFullColour) {
    pf = fullColourPF;
  } else {
    if (lowColourLevel == 0)
      pf = verylowColourPF;
    else if (lowColourLevel == 1)
      pf = lowColourPF;
    else
      pf = mediumColourPF;
  }

  if (pf != server.pf()) {
    char str[256];
    pf.print(str, 256);
    vlog.info(_("Using pixel format %s"),str);
    setPF(pf);
  }
}

void CConn::handleOptions(void *data)
{
  CConn *self = (CConn*)data;

  self->updateEncoding();
  self->updateCompressLevel();
  self->updateQualityLevel();
  self->updatePixelFormat();
}

void CConn::handleUpdateTimeout(void *data)
{
  CConn *self = (CConn *)data;

  assert(self);

  self->desktop->updateWindow();

  Fl::repeat_timeout(1.0, handleUpdateTimeout, data);
}
