/* Copyright (C) 2026 Gaurav Ujjwal. All Rights Reserved.
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

#ifdef HAVE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/alternative.h>
#include <avahi-common/domain.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/watch.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

#include <core/Exception.h>
#include <core/LogWriter.h>
#include <core/Timer.h>
#include <core/string.h>
#include <core/time.h>
#include <network/DnsSD.h>
#include <network/TcpSocket.h>

using namespace network;
using namespace network::dnssd;

#ifdef HAVE_AVAHI

// Service to be advertised through Avahi
struct Service {
  std::string name;
  int port;
  int iface;
  int protocol;
  AvahiEntryGroup* group;
};

// Avahi uses a polling API to watch over FDs and manage timeouts.
// In AdaptivePoll:
// - Watches are implemented by delegating actual polling to WatchAdapter
// - Timeouts are implemented by using core::Timer
struct AdaptivePoll {
  struct AvahiPoll api;
  std::list<AvahiWatch*> watches;
  std::list<AvahiTimeout*> timeouts;
  WatchAdapter* adapter;
};

struct AvahiWatch {
  int fd;
  AvahiWatchEvent event;
  AvahiWatchEvent revent;
  AvahiWatchCallback callback;
  void* userdata;
  AdaptivePoll* poll;
};

struct AvahiTimeout : core::Timer::Callback {
  AvahiTimeoutCallback callback;
  void* userdata;
  AdaptivePoll* poll;
  core::Timer timer;

  AvahiTimeout(AvahiTimeoutCallback callback_, void* userdata_, AdaptivePoll* poll_)
      : callback(callback_), userdata(userdata_), poll(poll_), timer(this)
  {
  }

  void handleTimeout(core::Timer* t) override
  {
    (void)t;
    callback(this, userdata);
  }
};

static core::LogWriter vlog("DnsSD");
static std::list<Service*> services;
static AvahiClient* avahiClient = NULL;
static AdaptivePoll* aPoll = NULL;

static void clientCallback(AvahiClient*, AvahiClientState, void*);
static void entryGroupCallback(AvahiEntryGroup*, AvahiEntryGroupState, void*);
static void handleNameCollision(Service*);

/******************************************************************************/

static bool isReadEvent(AvahiWatchEvent event)
{
  return event & (POLLIN | POLLERR | POLLHUP);
}

static bool isWriteEvent(AvahiWatchEvent event) { return event & POLLOUT; }

static void createAdaptivePoll(WatchAdapter* adapter)
{
  assert(!aPoll);
  aPoll = new AdaptivePoll();
  aPoll->api.userdata = aPoll;
  aPoll->adapter = adapter;

  // Watch API
  aPoll->api.watch_new = [](const AvahiPoll* api, int fd, AvahiWatchEvent event,
                            AvahiWatchCallback callback, void* userdata) {
    auto poll = (AdaptivePoll*)api->userdata;
    auto watch =
        new AvahiWatch{fd, event, (AvahiWatchEvent)0, callback, userdata, poll};

    poll->watches.push_back(watch);
    poll->adapter->addWatch(fd, isReadEvent(event), isWriteEvent(event));

    return watch;
  };

  aPoll->api.watch_update = [](AvahiWatch* w, AvahiWatchEvent event) {
    w->event = event;
    w->poll->adapter->removeWatch(w->fd);
    w->poll->adapter->addWatch(w->fd, isReadEvent(event), isWriteEvent(event));
  };

  aPoll->api.watch_get_events = [](AvahiWatch* w) { return w->revent; };

  aPoll->api.watch_free = [](AvahiWatch* w) {
    w->poll->adapter->removeWatch(w->fd);
    w->poll->watches.remove(w);
    delete w;
  };

  // Timeout API
  aPoll->api.timeout_new = [](const AvahiPoll* api, const struct timeval* tv,
                              AvahiTimeoutCallback callback, void* userdata) {
    auto poll = (AdaptivePoll*)api->userdata;
    auto timeout = new AvahiTimeout(callback, userdata, poll);

    poll->timeouts.push_back(timeout);
    if (tv)
      timeout->timer.start(core::msUntil(tv));

    return timeout;
  };

  aPoll->api.timeout_update = [](AvahiTimeout* t, const struct timeval* tv) {
    if (tv)
      t->timer.start(core::msUntil(tv));
    else
      t->timer.stop();
  };

  aPoll->api.timeout_free = [](AvahiTimeout* t) {
    t->poll->timeouts.remove(t);
    delete t;
  };
}

static void handleReadyWatch(int fd)
{
  if (!aPoll)
    return;

  for (auto w : aPoll->watches) {
    if (fd == w->fd) {
      // Hack: Avahi expect callback in terms of POLL* events
      //       So use poll() with 0 timeout to extract required events
      pollfd pfd;
      pfd.fd = w->fd;
      pfd.events = w->event;
      if (poll(&pfd, 1, 0) >= 0) {
        w->revent = (AvahiWatchEvent)pfd.revents;
        w->callback(w, w->fd, w->revent, w->userdata);
      }
    }
  }
}

static void createAvahiClient(AvahiPoll* api)
{
  assert(api);
  assert(!avahiClient);

  int error;
  avahiClient = avahi_client_new(
      api,
      AvahiClientFlags::AVAHI_CLIENT_NO_FAIL, // Wait for daemon if not running
      clientCallback, api, &error);

  if (!avahiClient)
    throw std::runtime_error(
        core::format("Unable to create Avahi client: %s", avahi_strerror(error)));
}

static void destroyAvahiClient()
{
  if (avahiClient) {
    avahi_client_free(avahiClient);
    avahiClient = NULL;

    // Groups are automatically freed by avahi_client_free()
    for (auto service : services)
      service->group = NULL;
  }
}

/******************************************************************************/

static void publishService(Service* service)
{
  assert(avahiClient);
  assert(service);

  if (!service->group) {
    service->group = avahi_entry_group_new(avahiClient, entryGroupCallback, service);
    if (!service->group) {
      vlog.error("Cannot create Avahi entry group: %s",
                 avahi_strerror(avahi_client_errno(avahiClient)));
      return;
    }
  }

  if (avahi_entry_group_is_empty(service->group)) {
    vlog.debug("Publishing service '%s', on port %d", service->name.c_str(),
               service->port);

    int e;
    e = avahi_entry_group_add_service(
        service->group, service->iface, service->protocol, (AvahiPublishFlags)0,
        service->name.c_str(), "_rfb._tcp", NULL, NULL, service->port, NULL);

    if (e < 0) {
      if (e == AVAHI_ERR_COLLISION)
        handleNameCollision(service);
      else
        vlog.error("Unable to add service: %s", avahi_strerror(e));
      return;
    }

    e = avahi_entry_group_commit(service->group);
    if (e < 0)
      vlog.error("Unable to commit entry group: %s", avahi_strerror(e));
  }
}

static void publishAll()
{
  for (auto service : services)
    publishService(service);
}

static void unpublishAll()
{
  for (auto service : services)
    if (service->group)
      avahi_entry_group_reset(service->group);
}

static void handleNameCollision(Service* service)
{
  assert(service);
  vlog.debug("Collision detected for name '%s'", service->name.c_str());

  char* newName = avahi_alternative_service_name(service->name.c_str());
  service->name = newName;
  avahi_free(newName);

  // On name collision, Avahi automatically removes published services
  // So re-publish with new name
  publishService(service);
}

static void entryGroupCallback(AvahiEntryGroup* group, AvahiEntryGroupState state,
                               void* data)
{
  assert(data);
  Service* service = (Service*)data;

  assert(!service->group || service->group == group);
  service->group = group; // In case callback is invoked during group creation

  switch (state) {
  case AVAHI_ENTRY_GROUP_ESTABLISHED:
    vlog.debug("Entry group established for '%s'", service->name.c_str());
    break;

  case AVAHI_ENTRY_GROUP_COLLISION:
    handleNameCollision(service);
    break;

  case AVAHI_ENTRY_GROUP_FAILURE:
    vlog.error("Entry group failure: %s",
               avahi_strerror(avahi_client_errno(avahiClient)));
    break;

  default:
    break;
  }
}

static void clientCallback(AvahiClient* client, AvahiClientState state, void* data)
{
  assert(data);
  assert(!avahiClient || avahiClient == client);
  avahiClient = client; // In case callback is invoked during client creation

  switch (state) {
  case AVAHI_CLIENT_S_RUNNING:
    publishAll();
    break;

  case AVAHI_CLIENT_S_REGISTERING:
  case AVAHI_CLIENT_S_COLLISION:
    unpublishAll();
    break;

  case AVAHI_CLIENT_FAILURE:
    if (avahi_client_errno(client) == AVAHI_ERR_DISCONNECTED) {
      vlog.debug("Avahi daemon disconnected, restarting client to wait for it");
      destroyAvahiClient();
      createAvahiClient((AvahiPoll*)data);
    } else
      vlog.error("Client failure: %s", avahi_strerror(avahi_client_errno(client)));
    break;

  default:
    break;
  }
}

/******************************************************************************/

// Returns the interface to which given address is bound.
// Returns AVAHI_IF_UNSPEC if addr is wildcard address, and 0 on errors.
static int getInterfaceForAddress(const vnc_sockaddr& addr)
{
  if ((addr.u.sa.sa_family == AF_INET && addr.u.sin.sin_addr.s_addr == INADDR_ANY) ||
      (addr.u.sa.sa_family == AF_INET6 &&
       IN6_IS_ADDR_UNSPECIFIED(&addr.u.sin6.sin6_addr)))
    return AVAHI_IF_UNSPEC;

  ifaddrs* head = NULL;
  if (getifaddrs(&head) != 0)
    return 0;

  char* ifname = NULL;
  for (const ifaddrs* ifa = head; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != addr.u.sa.sa_family)
      continue;

    if (addr.u.sa.sa_family == AF_INET &&
        addr.u.sin.sin_addr.s_addr ==
            ((sockaddr_in*)(ifa->ifa_addr))->sin_addr.s_addr) {
      ifname = ifa->ifa_name;
      break;
    }

    if (addr.u.sa.sa_family == AF_INET6 &&
        IN6_ARE_ADDR_EQUAL(&addr.u.sin6.sin6_addr,
                           &((sockaddr_in6*)(ifa->ifa_addr))->sin6_addr)) {
      ifname = ifa->ifa_name;
      break;
    }
  }

  if (head)
    freeifaddrs(head);

  if (ifname)
    return if_nametoindex(ifname);

  return 0;
}

static std::list<Service*> createServices(std::string name,
                                          std::list<SocketListener*>& listeners)
{
  if (!avahi_is_valid_service_name(name.c_str())) {
    const char* fallback = "TigerVNC Server";
    vlog.error("Invalid service name, falling back to: '%s'", fallback);
    name = fallback;
  }

  std::list<Service*> result;
  for (auto listener : listeners) {
    int port = listener->getMyPort();
    if (port == 0)
      continue;

    vnc_sockaddr_t sa;
    socklen_t sa_size = sizeof(sa);
    if (getsockname(listener->getFd(), &sa.u.sa, &sa_size) < 0)
      continue;

    if (sa.u.sa.sa_family != AF_INET && sa.u.sa.sa_family != AF_INET6)
      continue;

    int protocol =
        (sa.u.sa.sa_family == AF_INET) ? AVAHI_PROTO_INET : AVAHI_PROTO_INET6;

    int iface = getInterfaceForAddress(sa);
    if (!iface) {
      vlog.error("Cannot get interface id for listener [errno: %d]", errno);
      continue;
    }

    // Usually there are just two listeners, one for IPv4 & one for IPv6.
    // Optimize a little by de-duplicating these
    if (!result.empty()) {
      auto s = result.back();
      if (s->port == port && s->iface == iface && s->protocol != protocol) {
        s->protocol = AVAHI_PROTO_UNSPEC;
        continue;
      }
    }

    result.push_back(new Service{.name = name,
                                 .port = port,
                                 .iface = iface,
                                 .protocol = protocol,
                                 .group = NULL});
  }

  return result;
}

#endif

/******************************************************************************/

void dnssd::initialize(WatchAdapter* adapter)
{
#ifdef HAVE_AVAHI

  assert(adapter);
  createAdaptivePoll(adapter);
  createAvahiClient(&aPoll->api);
  adapter->handleReadyWatch = &handleReadyWatch;

#else
  (void)adapter;
#endif
}

void dnssd::advertise(std::string name, std::list<SocketListener*>& listeners)
{
#ifdef HAVE_AVAHI

  assert(avahiClient);

  std::list<Service*> newServices = createServices(name, listeners);
  services.splice(services.end(), newServices);

  if (avahi_client_get_state(avahiClient) == AVAHI_CLIENT_S_RUNNING)
    publishAll(); // If already running, publish right away

#else
  (void)name;
  (void)listeners;
#endif
}

void dnssd::shutdown()
{
#ifdef HAVE_AVAHI

  destroyAvahiClient();
  delete aPoll;
  aPoll = NULL;

  for (auto service : services)
    delete service;
  services.clear();

#endif
}
