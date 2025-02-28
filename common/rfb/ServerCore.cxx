/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

// -=- ServerCore.cxx

// This header will define the Server interface, from which ServerMT and
// ServerST will be derived.

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <rfb/ServerCore.h>

core::IntParameter rfb::Server::idleTimeout
("IdleTimeout",
 "The number of seconds after which an idle VNC connection will be dropped "
 "(zero means no timeout)",
 0, 0, INT_MAX);
core::IntParameter rfb::Server::maxDisconnectionTime
("MaxDisconnectionTime",
 "Terminate when no client has been connected for s seconds", 
 0, 0, INT_MAX);
core::IntParameter rfb::Server::maxConnectionTime
("MaxConnectionTime",
 "Terminate when a client has been connected for s seconds", 
 0, 0, INT_MAX);
core::IntParameter rfb::Server::maxIdleTime
("MaxIdleTime",
 "Terminate after s seconds of user inactivity", 
 0, 0, INT_MAX);
core::IntParameter rfb::Server::compareFB
("CompareFB",
 "Perform pixel comparison on framebuffer to reduce unnecessary updates "
 "(0: never, 1: always, 2: auto)",
 2, 0, 2);
core::IntParameter rfb::Server::frameRate
("FrameRate",
 "The maximum number of updates per second sent to each client",
 60, 0, INT_MAX);
core::BoolParameter rfb::Server::protocol3_3
("Protocol3.3",
 "Always use protocol version 3.3 for backwards compatibility with "
 "badly-behaved clients",
 false);
core::BoolParameter rfb::Server::alwaysShared
("AlwaysShared",
 "Always treat incoming connections as shared, regardless of the client-"
 "specified setting",
 false);
core::BoolParameter rfb::Server::neverShared
("NeverShared",
 "Never treat incoming connections as shared, regardless of the client-"
 "specified setting",
 false);
core::BoolParameter rfb::Server::disconnectClients
("DisconnectClients",
 "Disconnect existing clients if an incoming connection is non-shared. "
 "If combined with NeverShared then new connections will be refused "
 "while there is a client active",
 true);
core::BoolParameter rfb::Server::acceptKeyEvents
("AcceptKeyEvents",
 "Accept key press and release events from clients.",
 true);
core::BoolParameter rfb::Server::acceptPointerEvents
("AcceptPointerEvents",
 "Accept pointer movement and button events from clients.",
 true);
core::BoolParameter rfb::Server::acceptCutText
("AcceptCutText",
 "Accept clipboard updates from clients.",
 true);
core::BoolParameter rfb::Server::sendCutText
("SendCutText",
 "Send clipboard changes to clients.",
 true);
core::BoolParameter rfb::Server::acceptSetDesktopSize
("AcceptSetDesktopSize",
 "Accept set desktop size events from clients.",
 true);
core::BoolParameter rfb::Server::queryConnect
("QueryConnect",
 "Prompt the local user to accept or reject incoming connections.",
 false);
