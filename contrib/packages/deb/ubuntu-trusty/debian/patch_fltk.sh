#!/bin/bash
set -e
apply_patch()
{
    rm -f $2
    curl -OL http://www.fltk.org/strfiles/$1/$2
    patch -p1 < $2
}

# Export dead key information from FLTK to the apps
# http://www.fltk.org/str.php?L2599
apply_patch 2599 fltk-1_v4.3.x-keyboard-x11.patch
apply_patch 2599 fltk-1_v4.3.x-keyboard-win32.patch
apply_patch 2599 fltk-1_v6.3.x-keyboard-osx.patch

# Notify applications of changes to the clipboard
# http://www.fltk.org/str.php?L2636
apply_patch 2636 fltk-1.3.x-clipboard.patch
apply_patch 2636 fltk-1_v6.3.x-clipboard-x11.patch
apply_patch 2636 fltk-1_v3.3.x-clipboard-win32-fix.patch
apply_patch 2636 fltk-1_v2.3.x-clipboard-win32.patch
apply_patch 2636 fltk-1_v2.3.x-clipboard-osx.patch

# Ability to convert a Fl_Pixmap to a Fl_RGB_Image
# http://www.fltk.org/str.php?L2659
apply_patch 2659 pixmap_v2.patch

# Support for custom cursors
# http://www.fltk.org/str.php?L2660
apply_patch 2660 fltk-1_v5.3.x-cursor.patch

# Improve modality interaction with WM
# http://www.fltk.org/str.php?L2802
apply_patch 2802 fltk-1_v2.3.0-modal.patch

# Window icons
# http://www.fltk.org/str.php?L2816
apply_patch 2816 fltk-1_v3.3.0-icons.patch

# Multihead
# http://fltk.org/str.php?L2860
apply_patch 2860 fltk-1.3.x-screen_num.patch
apply_patch 2860 fltk-1_v3.3.x-multihead.patch

# Apply DRC's patches to FLTK
curl -L 'https://sourceforge.net/mailarchive/attachment.php?list_name=tigervnc-devel&message_id=512DD1FE.7090609%40users.sourceforge.net&counter=1' -o 0001-Add-BUILD_STATIC-feature-from-TigerVNC-to-optionally.patch 
curl -L 'https://sourceforge.net/mailarchive/attachment.php?list_name=tigervnc-devel&message_id=512DD1FE.7090609%40users.sourceforge.net&counter=2' -o 0002-Fl_cocoa.mm-depends-on-some-Carbon-functions-so-we-n.patch
curl -L 'https://sourceforge.net/mailarchive/attachment.php?list_name=tigervnc-devel&message_id=512DD1FE.7090609%40users.sourceforge.net&counter=3' -o 0003-We-need-to-unset-CMAKE_REQUIRED_LIBRARIES-after-chec.patch

patch -p1 -i 0001-Add-BUILD_STATIC-feature-from-TigerVNC-to-optionally.patch
patch -p1 -i 0002-Fl_cocoa.mm-depends-on-some-Carbon-functions-so-we-n.patch
patch -p1 -i 0003-We-need-to-unset-CMAKE_REQUIRED_LIBRARIES-after-chec.patch
