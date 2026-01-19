/* Copyright 2025 Adam Halim for Cendio AB
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
#include <sys/mman.h>

#include <string.h>

#include <xkbcommon/xkbcommon.h>
#include <wayland-client-protocol.h>

#include <core/LogWriter.h>
#include <rfb/ledStates.h>

#include "../../w0vncserver.h"
#include "Display.h"
#include "Seat.h"
#include "Keyboard.h"

using namespace wayland;

extern const unsigned int code_map_xkb_to_qnum_len;
extern const struct _code_map_xkb_to_qnum {
  const char* from;
  const unsigned short to;
} code_map_xkb_to_qnum[];

static core::LogWriter vlog("Keyboard");

const wl_keyboard_listener Keyboard::listener = {
  .keymap = [](void* data, wl_keyboard*, uint32_t format,
               int32_t fd, uint32_t size) {
    ((Keyboard*)data)->handleKeyMap(format, fd, size);
  },
  .enter = [](void*, wl_keyboard*, uint32_t, wl_surface*,
              wl_array*) {},
  .leave = [](void*, wl_keyboard*, uint32_t, wl_surface*) {},
  .key = [](void*, wl_keyboard*, uint32_t, uint32_t, uint32_t,
            uint32_t) {},
  .modifiers = [](void* data, struct wl_keyboard*,
                  uint32_t serial, uint32_t modsDepressed,
                  uint32_t modsLatched, uint32_t modsLocked,
                  uint32_t group) {
    ((Keyboard*)data)->handleModifiers(serial, modsDepressed,
                                       modsLatched,
                                       modsLocked, group);
  },
  .repeat_info = [](void*, wl_keyboard*, int32_t, int32_t) {}
};

struct XkbContext {
  xkb_context* ctx;
  xkb_state* state;
  xkb_keymap* keymap;
};

Keyboard::Keyboard(Display* display, Seat* seat,
                   std::function<void(unsigned int)> setLEDstate_)
  : keyboardFormat(0), keyboardFd(0), keyboardSize(0),
    keyboard(nullptr), keyMap(nullptr), context(nullptr),
    setLEDstate(setLEDstate_)
{
  xkb_context* ctx;

  ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (ctx)  {
    context = new XkbContext();
    context->ctx = ctx;
  } else {
    // FIXME: fallback?
    vlog.error("Failed to create xkb context - keyboard will not work");
  }

  keyboard = wl_seat_get_keyboard(seat->getSeat());
  wl_keyboard_add_listener(keyboard, &listener, this);
  display->roundtrip();

  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));
}

Keyboard::~Keyboard()
{
  if (keyboard)
    wl_keyboard_destroy(keyboard);
  clearKeyMap();
  delete context;
}

bool Keyboard::updateState(uint32_t keycode, bool down,
                           uint32_t* modsDepressed,
                           uint32_t* modsLatched, uint32_t* modsLocked,
                           uint32_t* group)
{
  xkb_state_component changed;

  if (!context || !context->state)
    return false;

  changed = xkb_state_update_key(context->state, keycode,
                                 down ? XKB_KEY_DOWN : XKB_KEY_UP);

  *modsDepressed = xkb_state_serialize_mods(context->state, XKB_STATE_MODS_DEPRESSED);
  *modsLatched = xkb_state_serialize_mods(context->state, XKB_STATE_MODS_LATCHED);
  *modsLocked = xkb_state_serialize_mods(context->state, XKB_STATE_MODS_LOCKED);
  *group = xkb_state_serialize_mods(context->state, XKB_STATE_LAYOUT_EFFECTIVE);

  if (changed)
    setLEDstate(getLEDState());

  return changed != 0;
}

void Keyboard::handleKeyMap(uint32_t format, int32_t fd, uint32_t size)
{
  if (!context || !context->ctx)
    return;

  // https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format
  // Format is either:
  //  no_keymap 0, or
  //  xkb_v1    1
  if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    xkb_keymap* map;
    xkb_state* state;
    char* newKeyMap;

    newKeyMap = (char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (!newKeyMap) {
      vlog.error("Failed to map keymap");
      clearKeyMap();
      return;
    }

    if (keyMap && strcmp(keyMap, newKeyMap) == 0) {
      // Keymap unchanged, no need to update anything.
      // This is a workaround for where zwp_virtual_keyboard_v1_keymap()
      // will trigger a keymap event with the same keymap as before
      return;
    }

    if (keyMap)
      munmap(keyMap, keyboardSize);

    keyMap = newKeyMap;

    map = xkb_keymap_new_from_buffer(context->ctx, keyMap,
                                     size,
                                     XKB_KEYMAP_FORMAT_TEXT_V1,
                                     XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!map) {
      vlog.error("Failed to create xkb keymap");
      clearKeyMap();
      return;
    }

    if (context->keymap)
      xkb_keymap_unref(context->keymap);
    context->keymap = map;

    state = xkb_state_new(context->keymap);
    if (!state) {
      vlog.error("Failed to create xkb state");
      clearKeyMap();
      return;
    }

    if (context->state)
      xkb_state_unref(context->state);
    context->state = state;

    vlog.debug("Keymap updated");
    keyboardFormat = format;
    keyboardFd = fd;
    keyboardSize = size;
    generateKeycodeMap();
  } else {
    vlog.error("Unsupported keymap format");
    clearKeyMap();
  }
}

uint32_t Keyboard::keysymToKeycode(int keysym)
{
  xkb_keycode_t min;
  xkb_keycode_t max;

  if (!context || !context->keymap)
    return XKB_KEYCODE_INVALID;

  min = xkb_keymap_min_keycode(context->keymap);
  max = xkb_keymap_max_keycode(context->keymap);

  for (xkb_keycode_t keycode = min; keycode <= max; keycode++) {
    xkb_keysym_t sym;
    sym = xkb_state_key_get_one_sym(context->state, keycode);
    if (sym == (xkb_keysym_t)keysym)
      return keycode;
  }

  return XKB_KEYCODE_INVALID;
}

uint32_t Keyboard::rfbcodeToKeycode(uint32_t rfbcode)
{
  if (!context)
    return XKB_KEYCODE_INVALID;
  if (rfbcode >= sizeof(codeMapQnumToKeyCode))
    return XKB_KEYCODE_INVALID;
  if (codeMapQnumToKeyCode[rfbcode] == 0)
    return XKB_KEYCODE_INVALID;
  return codeMapQnumToKeyCode[rfbcode];
}

void Keyboard::handleModifiers(uint32_t /* serial */,
                               uint32_t modsDepressed,
                               uint32_t modsLatched,
                               uint32_t modsLocked, uint32_t group)
{
  xkb_state_component changed;

  if (!context || !context->state)
    return;

  // FIXME: What do we set the latched/locked layouts to?
  changed = xkb_state_update_mask(context->state, modsDepressed, modsLatched,
                        modsLocked, group, 0, 0);

  if (changed & XKB_STATE_LEDS)
    setLEDstate(getLEDState());
}

unsigned int Keyboard::getLEDState()
{
  unsigned int ledState;

  if (!context || !context->state)
    return 0;

  ledState = 0;
  if (xkb_state_led_name_is_active(context->state, XKB_LED_NAME_SCROLL))
    ledState |= rfb::ledScrollLock;
  if (xkb_state_led_name_is_active(context->state, XKB_LED_NAME_NUM))
    ledState |= rfb::ledNumLock;
  if (xkb_state_led_name_is_active(context->state, XKB_LED_NAME_CAPS))
    ledState |= rfb::ledCapsLock;

  return ledState;
}

void Keyboard::generateKeycodeMap()
{
  if (!context || !context->keymap)
    return;

  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));

  /* Here, we generate a mapping from qnumcode (RFB) to our keyboard's
   * configured keycodes. We iterate through code_map_xkb_to_qnum in
   * reverse and try to find matches between the keycodemap and our
   * keymap.
   *
   * Note that we have to iterate through the xkb_to_qnum map in reverse
   * (instead of using a qnum_to_xkb map) since the mapping between
   * keycodes and keynames is one-to-many. This means that the same
   * keycode may be mapped by multiple keynames (such as TLDE and AB00
   * both mapping to 0x29).
   *
   * The only exception to this rule is SYRQ, which for historical
   * reasons is mapped by both 0x54 and 0xb7.
   */

  for (unsigned int rfbcode = 1; rfbcode < 255; rfbcode++) {
    unsigned int xkbKeyCode;
    bool rfbFound;

    xkbKeyCode = 0;
    rfbFound = false;
    for (unsigned int j = 0; j < code_map_xkb_to_qnum_len; j++) {
      const char* keyName;

      if (code_map_xkb_to_qnum[j].to != rfbcode)
        continue;

      rfbFound = true;
      keyName = code_map_xkb_to_qnum[j].from;
      assert(keyName);

      xkbKeyCode = xkb_keymap_key_by_name(context->keymap, keyName);
      if (xkbKeyCode != XKB_KEYCODE_INVALID)
        break;
    }

    if (xkbKeyCode != 0)
      codeMapQnumToKeyCode[rfbcode] = xkbKeyCode;
    else if (rfbFound)
      vlog.debug("No mapping found for key 0x%04x", rfbcode);
  }
}

void Keyboard::clearKeyMap()
{
  memset(codeMapQnumToKeyCode, 0, sizeof(codeMapQnumToKeyCode));

  if (keyMap) {
    munmap(keyMap, keyboardSize);
    keyMap = nullptr;
    keyboardFd = -1;
    keyboardSize = 0;
  }

  if (context) {
    if (context->state)
      xkb_state_unref(context->state);
    context->state = nullptr;
    if (context->keymap)
      xkb_keymap_unref(context->keymap);
    context->keymap = nullptr;
    if (context->ctx)
      xkb_context_unref(context->ctx);
    context->ctx = nullptr;
  }
}
