#ifndef INPUTCOMMON_H
#define INPUTCOMMON_H

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>

#ifdef __cplusplus
extern "C" {
#endif
    KeyCode addKeysymToMap(KeySym keysym, XkbDescPtr xkb, XkbChangesPtr changes);
    void removeAddedKeysymsFromMap(XkbDescPtr xkb, XkbMapChangesPtr changes);

    /**
     * Should be called whenever client uses a key.
    */
    void onKeyUsed(KeyCode key);

#ifdef __cplusplus
}
#endif
#endif /* INPUTCOMMON_H */
