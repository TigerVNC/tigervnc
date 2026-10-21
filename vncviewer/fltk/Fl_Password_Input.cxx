/* Copyright 2025 Samuel Mannehed for Cendio AB
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Pixmap.H>

#include "Fl_Password_Input.h"

// xpm:s predate const, so they have invalid definitions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
// Use XPM images to build them into the binary
#include "open_eye.xpm"
#include "closed_eye.xpm"
#include "shift_lock.xpm"
#pragma GCC diagnostic pop

#if defined(WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#else
#include <X11/XKBlib.h>
#include <FL/x.H>
#endif

// Size of toggle-password-eye
#define EYE_W  15
#define EYE_H  15

// Size of caps lock warning
#define SHIFT_W 11
#define SHIFT_H 11

/* The input needs to be offset inside the group
   to not clip the rounded corners */
#define INPUT_OFFSET  2

const char* Fl_Password_Input::caps_warning_tooltip = "Caps lock is active";

Fl_Password_Input::Fl_Password_Input(
  int x, int y, int w, int h, const char *label)
: Fl_Group(x, y, w, h, label)
{
    Fl_Group::color(FL_BACKGROUND2_COLOR);
    Fl_Group::box(FL_DOWN_BOX);
    align(FL_ALIGN_LEFT); // Place label outside, on the left side

    eye_x_offset = w - EYE_W - 5;
    eye_y_offset = (h - EYE_H) / 2;
    warning_x_offset = w - SHIFT_W - 5 - EYE_W - 5;
    warning_y_offset = (h - SHIFT_H) / 2;

    // Leave space on the right side for the eye and the caps lock warning
    input = new Fl_Secret_Input(x + INPUT_OFFSET, y + INPUT_OFFSET,
                                w - INPUT_OFFSET - EYE_W - 5 - SHIFT_W - 5,
                                h - (INPUT_OFFSET * 2));
    input->box(FL_FLAT_BOX);
    input->color(FL_BACKGROUND2_COLOR);
    input->callback(text_changed_cb, this);
    input->when(FL_WHEN_CHANGED);

    /* FIXME: Transparency isn't supported in XPM. Thus, the background color
       in the XPM image is currently hard-coded to white. It should ideally be
       based on the FLTK background color. If we wan't to properly support
       dark-mode, the foreground colors should also be used instead of black in
       the images... */
    open_eye_pixmap = new Fl_Pixmap(open_eye);
    closed_eye_pixmap = new Fl_Pixmap(closed_eye);

    // Greyed out version of the open eye for when deactivated
    deactivated_eye_pixmap = new Fl_Pixmap(open_eye);
    deactivated_eye_pixmap->inactive();

    toggle_button = new Fl_Button(x + eye_x_offset, y + eye_y_offset,
                                  EYE_W, EYE_H);
    toggle_button->image(open_eye_pixmap);
    toggle_button->deimage(deactivated_eye_pixmap);
    toggle_button->hide(); // No eye image is shown by default
    toggle_button->box(FL_NO_BOX);
    toggle_button->color(FL_BACKGROUND2_COLOR);
    toggle_button->clear_visible_focus();
    toggle_button->callback(toggle_cb, this);

    shift_lock_pixmap = new Fl_Pixmap(shift_lock);

    caps_warning = new Fl_Box(x + warning_x_offset, y + warning_y_offset,
                              SHIFT_W, SHIFT_H);
    caps_warning->tooltip(caps_warning_tooltip);
    caps_warning->image(shift_lock_pixmap);
    caps_warning->box(FL_NO_BOX);
    caps_warning->hide();
    caps_warning_shown = false;

    end(); // Close Fl_Group
}

Fl_Password_Input::~Fl_Password_Input()
{
    if (open_eye_pixmap)
        delete open_eye_pixmap;
    if (closed_eye_pixmap)
        delete closed_eye_pixmap;
    if (deactivated_eye_pixmap)
        delete deactivated_eye_pixmap;
    if (shift_lock_pixmap)
        delete shift_lock_pixmap;
}

int Fl_Password_Input::handle(int event)
{
    switch (event) {
    case FL_ENTER: {
        // Show the eye on mouse hover
        toggle_button->show();
        break;
    }
    case FL_LEAVE: {
        determine_eye_visibility();
        break;
    }
    case FL_DEACTIVATE: {
        password_visible(false);
        break;
    }
    }

    determine_caps_warning_visibility();
    return Fl_Group::handle(event);
}

void Fl_Password_Input::determine_eye_visibility()
{
    /* Always show the eye if the password is toggled to be visible,
     * or if the input contains text. */
    if (password_visible() || strcmp(input->value(), "") != 0) {
        toggle_button->show();
    } else {
        toggle_button->hide();
    }
}

void Fl_Password_Input::determine_caps_warning_visibility()
{
    bool capslock_active;

    capslock_active = false;
#if defined(WIN32)
    if (GetKeyState(VK_CAPITAL) & 0x1)
        capslock_active = true;
#elif defined(__APPLE__)
    kern_return_t ret;
    io_connect_t ioc;
    io_service_t ios;
    CFMutableDictionaryRef mdict;
    bool on;

    mdict = IOServiceMatching(kIOHIDSystemClass);
    ios = IOServiceGetMatchingService(0, (CFDictionaryRef) mdict);

    if (!ios)
        return;

    ret = IOServiceOpen(ios, mach_task_self(), kIOHIDParamConnectType, &ioc);
    IOObjectRelease(ios);
    if (ret != KERN_SUCCESS)
        return;

    ret = IOHIDGetModifierLockState(ioc, kIOHIDCapsLockState, &on);
    IOServiceClose(ioc);
    if (ret != KERN_SUCCESS)
        return;

    capslock_active = on;
#else
    Status status;
    XkbStateRec xkbState;

    status = XkbGetState(fl_display, XkbUseCoreKbd, &xkbState);
    if (status != Success)
        return;

    if (xkbState.locked_mods & LockMask)
        capslock_active = true;
#endif

  caps_warning_visible(capslock_active);
}

bool Fl_Password_Input::password_visible() const
{
    return input->input_type() != FL_SECRET_INPUT;
}

void Fl_Password_Input::password_visible(bool visible)
{
    if (visible) {
        input->input_type(FL_NORMAL_INPUT);
        toggle_button->image(closed_eye_pixmap);
    } else {
        input->input_type(FL_SECRET_INPUT);
        toggle_button->image(open_eye_pixmap);
    }
    redraw();
}

void Fl_Password_Input::caps_warning_visible(bool visible)
{
    // Avoid redrawing if no change as it can cause flickering
    if (visible == caps_warning_shown)
        return;

    caps_warning_shown = visible;

    if (visible)
        caps_warning->show();
    else
        caps_warning->hide();

    redraw();
}

void Fl_Password_Input::toggle_password_visible()
{
    password_visible(!password_visible());
}

const char * Fl_Password_Input::value() const
{
    return input->value();
}

int Fl_Password_Input::value(const char * t)
{
    return input->value(t);
}

int Fl_Password_Input::insert(const char *t, int l)
{
    return input->insert(t, l);
}

void Fl_Password_Input::position(int x, int y)
{
    Fl_Widget::position(x, y);
    input->Fl_Widget::position(x + INPUT_OFFSET, y + INPUT_OFFSET);
    toggle_button->Fl_Widget::position(x + eye_x_offset, y + eye_y_offset);
    caps_warning->Fl_Widget::position(x + warning_x_offset, y + warning_y_offset);
}

int Fl_Password_Input::take_focus()
{
    return input->take_focus();
}

void Fl_Password_Input::text_changed_cb(Fl_Widget *, void * w)
{
    Fl_Password_Input * pw_inp = (Fl_Password_Input*) w;
    pw_inp->determine_eye_visibility();
}

void Fl_Password_Input::toggle_cb(Fl_Widget *, void * w)
{
    Fl_Password_Input * pw_inp = (Fl_Password_Input*) w;
    pw_inp->toggle_password_visible();
}

