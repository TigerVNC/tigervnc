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

#define XKB_KEY_NAME_MAX_LEN 4

using namespace wayland;

extern const unsigned int code_map_qnum_to_xkb_len;
extern const char* code_map_qnum_to_xkb[];

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
    keyboard(nullptr), keyMap(nullptr), context(new XkbContext()),
    setLEDstate(setLEDstate_)
{
  xkb_context* ctx;

  ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!ctx)  {
    // FIXME: fallback?
    fatal_error("Failed to create xkb context");
    return;
  }
  context->ctx = ctx;

  keyboard = wl_seat_get_keyboard(seat->getSeat());
  wl_keyboard_add_listener(keyboard, &listener, this);
  display->roundtrip();

  memset(code_map_qnum_to_keycode, 0, sizeof(code_map_qnum_to_keycode));
}

Keyboard::~Keyboard()
{
  if (keyboard)
    wl_keyboard_destroy(keyboard);
  if(keyMap)
    munmap(keyMap, keyboardSize);

  if (context) {
    xkb_context_unref(context->ctx);
    xkb_state_unref(context->state);
    xkb_keymap_unref(context->keymap);
    delete context;
  }
}

bool Keyboard::updateState(uint32_t keycode, bool down,
                           uint32_t* modsDepressed,
                           uint32_t* modsLatched, uint32_t* modsLocked,
                           uint32_t* group)
{
  xkb_state_component changed;

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
  assert(context->ctx);

  memset(code_map_qnum_to_keycode, 0, sizeof(code_map_qnum_to_keycode));

  if (keyMap)
    munmap(keyMap, keyboardSize);
  keyboardFormat = format;
  keyboardFd = fd;
  keyboardSize = size;

  // https://wayland.app/protocols/wayland#wl_keyboard:enum:keymap_format
  // Format is either:
  //  no_keymap 0, or
  //  xkb_v1    1
  if (keyboardFormat == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    xkb_keymap* map;
    xkb_state* state;

    keyMap = (char*)mmap(nullptr, keyboardSize, PROT_READ, MAP_PRIVATE,
                         keyboardFd, 0);
    if (!keyMap) {
      vlog.error("Failed to map keymap");
      return;
    }

    map = xkb_keymap_new_from_buffer(context->ctx, keyMap,
                                     keyboardSize,
                                     XKB_KEYMAP_FORMAT_TEXT_V1,
                                     XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!map) {
      vlog.error("Failed to create xkb keymap");
      munmap(keyMap, keyboardSize);
      keyMap = nullptr;
      return;
    }

    if (context->keymap)
      xkb_keymap_unref(context->keymap);
    context->keymap = map;

    state = xkb_state_new(context->keymap);
    if (!state) {
      vlog.error("Failed to create xkb state");
      munmap(keyMap, keyboardSize);
      keyMap = nullptr;
      return;
    }

    if (context->state)
      xkb_state_unref(context->state);
    context->state = state;

    vlog.debug("Keymap updated");
    generateKeycodeMap();
  } else {
    vlog.error("Unsupported keymap format");
    if (keyMap) {
      munmap(keyMap, keyboardSize);
      keyMap = nullptr;
    }
  }
}

uint32_t Keyboard::keysymToKeycode(int keysym)
{
  xkb_keycode_t min;
  xkb_keycode_t max;

  if (keyMap == nullptr)
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

int Keyboard::rfbcodeToKeycode(int rfbcode)
{
  return code_map_qnum_to_keycode[rfbcode];
}

void Keyboard::handleModifiers(uint32_t /* serial */,
                               uint32_t modsDepressed,
                               uint32_t modsLatched,
                               uint32_t modsLocked, uint32_t group)
{
  xkb_state_component changed;
  // FIXME: What do we set the latched/locked layouts to?
  changed = xkb_state_update_mask(context->state, modsDepressed, modsLatched,
                        modsLocked, group, 0, 0);

  if (changed & XKB_STATE_LEDS)
    setLEDstate(getLEDState());
}

unsigned int Keyboard::getLEDState()
{
  unsigned int ledState;

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
  xkb_keycode_t min;
  xkb_keycode_t max;

  assert(sizeof(code_map_qnum_to_keycode) >= code_map_qnum_to_xkb_len);

  memset(code_map_qnum_to_keycode, 0, sizeof(code_map_qnum_to_keycode));

  min = xkb_keymap_min_keycode(context->keymap);
  max = xkb_keymap_max_keycode(context->keymap);

  for (unsigned int i = 0; i < code_map_qnum_to_xkb_len; i++) {
    xkb_keycode_t keycode;
    bool match;
    const char* qnumKeyName;

    match = false;
    qnumKeyName = code_map_qnum_to_xkb[i];
    if (!qnumKeyName)
      continue;

    keycode = 0;
    for (keycode = min; keycode <= max; keycode++) {
      const char* xkbKeyName;

      xkbKeyName = xkb_keymap_key_get_name(context->keymap, keycode);
      if (xkbKeyName && !strncmp(xkbKeyName, qnumKeyName, XKB_KEY_NAME_MAX_LEN)) {
        match = true;
        break;
      }
    }

    if (match)
      code_map_qnum_to_keycode[i] = keycode;
    else
      vlog.debug("No mapping for key %.4s ", qnumKeyName);
  }
}
