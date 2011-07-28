#
# "$Id: Makefile 8389 2011-02-06 14:08:08Z manolo $"
#
# Library makefile for the Fast Light Tool Kit (FLTK).
#
# Copyright 1998-2010 by Bill Spitzak and others.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA.
#
# Please report all bugs and problems on the following page:
#
#      http://www.fltk.org/str.php
#

CPPFILES = \
	Fl.cxx \
	Fl_Adjuster.cxx \
	Fl_Bitmap.cxx \
	Fl_Browser.cxx \
	Fl_Browser_.cxx \
	Fl_Browser_load.cxx \
	Fl_Box.cxx \
	Fl_Button.cxx \
	Fl_Chart.cxx \
	Fl_Check_Browser.cxx \
	Fl_Check_Button.cxx \
	Fl_Choice.cxx \
	Fl_Clock.cxx \
	Fl_Color_Chooser.cxx \
	Fl_Counter.cxx \
	Fl_Dial.cxx \
	Fl_Device.cxx \
	Fl_Double_Window.cxx \
	Fl_File_Browser.cxx \
	Fl_File_Chooser.cxx \
	Fl_File_Chooser2.cxx \
	Fl_File_Icon.cxx \
	Fl_File_Input.cxx \
	Fl_Group.cxx \
	Fl_Help_View.cxx \
	Fl_Image.cxx \
	Fl_Input.cxx \
	Fl_Input_.cxx \
	Fl_Light_Button.cxx \
	Fl_Menu.cxx \
	Fl_Menu_.cxx \
	Fl_Menu_Bar.cxx \
	Fl_Sys_Menu_Bar.cxx \
	Fl_Menu_Button.cxx \
	Fl_Menu_Window.cxx \
	Fl_Menu_add.cxx \
	Fl_Menu_global.cxx \
	Fl_Multi_Label.cxx \
	Fl_Native_File_Chooser.cxx \
	Fl_Overlay_Window.cxx \
	Fl_Pack.cxx \
	Fl_Paged_Device.cxx \
	Fl_Pixmap.cxx \
	Fl_Positioner.cxx \
	Fl_Preferences.cxx \
	Fl_Printer.cxx \
	Fl_Progress.cxx \
	Fl_Repeat_Button.cxx \
	Fl_Return_Button.cxx \
	Fl_Roller.cxx \
	Fl_Round_Button.cxx \
	Fl_Scroll.cxx \
	Fl_Scrollbar.cxx \
	Fl_Shared_Image.cxx \
	Fl_Single_Window.cxx \
	Fl_Slider.cxx \
	Fl_Table.cxx \
	Fl_Table_Row.cxx \
	Fl_Tabs.cxx \
	Fl_Text_Buffer.cxx \
	Fl_Text_Display.cxx \
	Fl_Text_Editor.cxx \
	Fl_Tile.cxx \
	Fl_Tiled_Image.cxx \
	Fl_Tree.cxx \
	Fl_Tree_Item.cxx \
	Fl_Tree_Item_Array.cxx \
	Fl_Tree_Prefs.cxx \
	Fl_Tooltip.cxx \
	Fl_Valuator.cxx \
	Fl_Value_Input.cxx \
	Fl_Value_Output.cxx \
	Fl_Value_Slider.cxx \
	Fl_Widget.cxx \
	Fl_Window.cxx \
	Fl_Window_fullscreen.cxx \
	Fl_Window_hotspot.cxx \
	Fl_Window_iconize.cxx \
	Fl_Wizard.cxx \
	Fl_XBM_Image.cxx \
	Fl_XPM_Image.cxx \
	Fl_abort.cxx \
	Fl_add_idle.cxx \
	Fl_arg.cxx \
	Fl_compose.cxx \
	Fl_display.cxx \
	Fl_get_key.cxx \
	Fl_get_system_colors.cxx \
	Fl_grab.cxx \
	Fl_lock.cxx \
	Fl_own_colormap.cxx \
	Fl_visual.cxx \
	Fl_x.cxx \
	filename_absolute.cxx \
	filename_expand.cxx \
	filename_ext.cxx \
	filename_isdir.cxx \
	filename_list.cxx \
	filename_match.cxx \
	filename_setext.cxx \
	fl_arc.cxx \
	fl_arci.cxx \
	fl_ask.cxx \
	fl_boxtype.cxx \
	fl_color.cxx \
	fl_cursor.cxx \
	fl_curve.cxx \
	fl_diamond_box.cxx \
	fl_dnd.cxx \
	fl_draw.cxx \
	fl_draw_image.cxx \
	fl_draw_pixmap.cxx \
	fl_encoding_latin1.cxx \
	fl_encoding_mac_roman.cxx \
	fl_engraved_label.cxx \
	fl_file_dir.cxx \
	fl_font.cxx \
	fl_gtk.cxx \
	fl_labeltype.cxx \
	fl_line_style.cxx \
	fl_open_uri.cxx \
	fl_oval_box.cxx \
	fl_overlay.cxx \
	fl_overlay_visual.cxx \
	fl_plastic.cxx \
	fl_read_image.cxx \
	fl_rect.cxx \
	fl_round_box.cxx \
	fl_rounded_box.cxx \
	fl_set_font.cxx \
	fl_set_fonts.cxx \
	fl_scroll_area.cxx \
	fl_shadow_box.cxx \
	fl_shortcut.cxx \
	fl_show_colormap.cxx \
	fl_symbols.cxx \
	fl_vertex.cxx \
	screen_xywh.cxx \
	fl_utf8.cxx \
	ps_image.cxx
	
OBJCPPFILES = \
    Fl_cocoa.mm \
    Fl_Quartz_Printer.mm \
    Fl_Native_File_Chooser_MAC.mm
	
FLCPPFILES = \
	forms_compatability.cxx \
	forms_bitmap.cxx \
	forms_free.cxx \
	forms_fselect.cxx \
	forms_pixmap.cxx \
	forms_timer.cxx

GLCPPFILES = \
	Fl_Gl_Choice.cxx \
	Fl_Gl_Overlay.cxx \
	Fl_Gl_Device_Plugin.cxx \
	Fl_Gl_Window.cxx \
	freeglut_geometry.cxx \
	freeglut_stroke_mono_roman.cxx \
	freeglut_stroke_roman.cxx \
	freeglut_teapot.cxx \
	gl_draw.cxx \
	gl_start.cxx \
	glut_compatability.cxx \
	glut_font.cxx

IMGCPPFILES = \
	fl_images_core.cxx \
	Fl_BMP_Image.cxx \
	Fl_File_Icon2.cxx \
	Fl_GIF_Image.cxx \
	Fl_Help_Dialog.cxx \
	Fl_JPEG_Image.cxx \
	Fl_PNG_Image.cxx \
	Fl_PNM_Image.cxx


CFILES = fl_call_main.c flstring.c scandir.c numericsort.c vsnprintf.c fl_utf.c

UTF8CFILES = \
	xutf8/case.c \
	xutf8/is_right2left.c \
	xutf8/is_spacing.c \
	xutf8/keysym2Ucs.c \
	xutf8/utf8Input.c \
	xutf8/utf8Utils.c \
	xutf8/utf8Wrap.c

################################################################
FLTKFLAGS = -DFL_LIBRARY
include ../makeinclude

MMFILES = $(shell if test $(USEMMFILES) = Yes; then echo $(OBJCPPFILES); fi)

OBJECTS =  $(MMFILES:.mm=.o) $(CPPFILES:.cxx=.o) $(CFILES:.c=.o) $(UTF8CFILES:.c=.o)
GLOBJECTS = $(GLCPPFILES:.cxx=.o)
FLOBJECTS = $(FLCPPFILES:.cxx=.o)
IMGOBJECTS = $(IMGCPPFILES:.cxx=.o)

all:	$(LIBNAME) $(DSONAME) \
	$(FLLIBNAME) $(FLDSONAME) \
	$(GLLIBNAME) $(GLDSONAME) \
	$(IMGLIBNAME) $(IMGDSONAME) 

$(LIBNAME): $(OBJECTS)
	echo $(LIBCOMMAND) $@ ...
	$(RM) $@
	$(LIBCOMMAND) $@ $(OBJECTS)
	$(RANLIB) $@

libfltk.so.1.3: $(OBJECTS)
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(OBJECTS)
	$(RM) libfltk.so
	$(LN) libfltk.so.1.3 libfltk.so

libfltk.sl.1.3: $(OBJECTS)
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(OBJECTS)
	$(RM) libfltk.sl
	$(LN) libfltk.sl.1.3 libfltk.sl

libfltk.1.3.dylib: $(OBJECTS)
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ \
		-install_name $(libdir)/$@ \
		-current_version 1.3.0 \
		-compatibility_version 1.3.0 \
		$(OBJECTS) $(LDLIBS)
	$(RM) libfltk.dylib
	$(LN) libfltk.1.3.dylib libfltk.dylib

libfltk_s.a: $(OBJECTS)
	echo $(DSOCOMMAND) libfltk_s.o ...
	$(DSOCOMMAND) libfltk_s.o $(OBJECTS) $(IMAGELIBS)
	echo $(LIBCOMMAND) libfltk_s.a libfltk_s.o
	$(RM) $@
	$(LIBCOMMAND) libfltk_s.a libfltk_s.o
	$(CHMOD) +x libfltk_s.a

$(FLLIBNAME): $(FLOBJECTS)
	echo $(LIBCOMMAND) $@ ...
	$(RM) $@
	$(LIBCOMMAND) $@ $(FLOBJECTS)
	$(RANLIB) $@

libfltk_forms.so.1.3: $(FLOBJECTS) libfltk.so.1.3
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(FLOBJECTS) -L. -lfltk
	$(RM) libfltk_forms.so
	$(LN) libfltk_forms.so.1.3 libfltk_forms.so

libfltk_forms.sl.1.3: $(FLOBJECTS) libfltk.sl.1.3
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(FLOBJECTS) -L. -lfltk
	$(RM) libfltk_forms.sl
	$(LN) libfltk_forms.sl.1.3 libfltk_forms.sl

libfltk_forms.1.3.dylib: $(FLOBJECTS) libfltk.1.3.dylib
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ \
		-install_name $(libdir)/$@ \
		-current_version 1.3.0 \
		-compatibility_version 1.3.0 \
		$(FLOBJECTS) -L. $(LDLIBS) -lfltk
	$(RM) libfltk_forms.dylib
	$(LN) libfltk_forms.1.3.dylib libfltk_forms.dylib

libfltk_forms_s.a: $(FLOBJECTS)
	echo $(DSOCOMMAND) libfltk_forms_s.o ...
	$(DSOCOMMAND) libfltk_forms_s.o $(FLOBJECTS)
	echo $(LIBCOMMAND) libfltk_forms_s.a libfltk_forms_s.o
	$(RM) $@
	$(LIBCOMMAND) libfltk_forms_s.a libfltk_forms_s.o
	$(CHMOD) +x libfltk_forms_s.a

$(GLLIBNAME): $(GLOBJECTS)
	echo $(LIBCOMMAND) $@ ...
	$(RM) $@
	$(LIBCOMMAND) $@ $(GLOBJECTS)
	$(RANLIB) $@

libfltk_gl.so.1.3: $(GLOBJECTS) libfltk.so.1.3
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(GLOBJECTS) -L. -lfltk
	$(RM) libfltk_gl.so
	$(LN) libfltk_gl.so.1.3 libfltk_gl.so

libfltk_gl.sl.1.3: $(GLOBJECTS) libfltk.sl.1.3
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(GLOBJECTS) -L. -lfltk
	$(RM) libfltk_gl.sl
	$(LN) libfltk_gl.sl.1.3 libfltk_gl.sl

libfltk_gl.1.3.dylib: $(GLOBJECTS) libfltk.1.3.dylib
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ \
		-install_name $(libdir)/$@ \
		-current_version 1.3.0 \
		-compatibility_version 1.3.0 \
		$(GLOBJECTS) -L. $(GLDLIBS) -lfltk
	$(RM) libfltk_gl.dylib
	$(LN) libfltk_gl.1.3.dylib libfltk_gl.dylib

libfltk_gl_s.a: $(GLOBJECTS)
	echo $(DSOCOMMAND) libfltk_gl_s.o ...
	$(DSOCOMMAND) libfltk_gl_s.o $(GLOBJECTS)
	echo $(LIBCOMMAND) libfltk_gl_s.a libfltk_gl_s.o
	$(RM) $@
	$(LIBCOMMAND) libfltk_gl_s.a libfltk_gl_s.o
	$(CHMOD) +x libfltk_gl_s.a

$(IMGLIBNAME): $(IMGOBJECTS)
	echo $(LIBCOMMAND) $@ ...
	$(RM) $@
	$(LIBCOMMAND) $@ $(IMGOBJECTS)
	$(RANLIB) $@

libfltk_images.so.1.3: $(IMGOBJECTS) libfltk.so.1.3
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(IMGOBJECTS) -L. $(IMAGELIBS) -lfltk
	$(RM) libfltk_images.so
	$(LN) libfltk_images.so.1.3 libfltk_images.so

libfltk_images.sl.1.3: $(IMGOBJECTS) libfltk.sl.1.3
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ $(IMGOBJECTS) -L. $(IMAGELIBS) -lfltk
	$(RM) libfltk_images.sl
	$(LN) libfltk_images.sl.1.3 libfltk_images.sl

libfltk_images.1.3.dylib: $(IMGOBJECTS) libfltk.1.3.dylib
	echo $(DSOCOMMAND) $@ ...
	$(DSOCOMMAND) $@ \
		-install_name $(libdir)/$@ \
		-current_version 1.3.0 \
		-compatibility_version 1.3.0 \
		$(IMGOBJECTS)  -L. $(LDLIBS) $(IMAGELIBS) -lfltk
	$(RM) libfltk_images.dylib
	$(LN) libfltk_images.1.3.dylib libfltk_images.dylib

libfltk_images_s.a: $(IMGOBJECTS)
	echo $(DSOCOMMAND) libfltk_images_s.o ...
	$(DSOCOMMAND) libfltk_images_s.o $(IMGOBJECTS)
	echo $(LIBCOMMAND) libfltk_images_s.a libfltk_images_s.o
	$(RM) $@
	$(LIBCOMMAND) libfltk_images_s.a libfltk_images_s.o
	$(CHMOD) +x libfltk_images_s.a

#-----------------------------------------------------------------
# - the import libraries libfltk*.dll.a and the .dll files
#   are created from the libfltk*.a files. They are built
#   into the src dir.
# - The _images, _gl, _forms and dlls must be linked
#   against the import libraries in the src dir.
#-----------------------------------------------------------------

#-----------------------------------------------------------------
# cygwin GDI shared libraries
#-----------------------------------------------------------------

cygfltknox-1.3.dll: $(LIBNAME)
	echo $(DSOCOMMAND) $(LIBNAME) ...
	$(DSOCOMMAND) $(LIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk.dll.a $(LDLIBS)

cygfltknox_forms-1.3.dll: $(FLLIBNAME) cygfltknox-1.3.dll
	echo $(DSOCOMMAND) $(FLLIBNAME) ...
	$(DSOCOMMAND) $(FLLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_forms.dll.a \
		-L. -lfltk $(LDLIBS)

cygfltknox_gl-1.3.dll: $(GLLIBNAME) cygfltknox-1.3.dll
	echo $(DSOCOMMAND) $(GLLIBNAME) ...
	$(DSOCOMMAND) $(GLLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_gl.dll.a \
		-L. -lfltk $(GLDLIBS)

cygfltknox_images-1.3.dll: $(IMGLIBNAME) cygfltknox-1.3.dll
	echo $(DSOCOMMAND) $(IMGLIBNAME) ...
	$(DSOCOMMAND) $(IMGLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_images.dll.a \
		-L. -lfltk -Wl,--exclude-libs -Wl,libfltk_png.a \
			$(IMAGELIBS) $(LDLIBS)

#-----------------------------------------------------------------
# cygwin X11 shared libraries
#-----------------------------------------------------------------

cygfltk-1.3.dll: $(LIBNAME)
	echo $(DSOCOMMAND) $(LIBNAME) ...
	$(DSOCOMMAND) $(LIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk.dll.a $(LDLIBS)

cygfltk_forms-1.3.dll: $(FLLIBNAME) cygfltk-1.3.dll
	echo $(DSOCOMMAND) $(FLLIBNAME) ...
	$(DSOCOMMAND) $(FLLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_forms.dll.a \
		-L. -lfltk $(LDLIBS)

cygfltk_gl-1.3.dll: $(GLLIBNAME) cygfltk-1.3.dll
	echo $(DSOCOMMAND) $(GLLIBNAME) ...
	$(DSOCOMMAND) $(GLLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_gl.dll.a \
		-L. -lfltk $(GLDLIBS)

cygfltk_images-1.3.dll: $(IMGLIBNAME) cygfltk-1.3.dll
	echo $(DSOCOMMAND) $(IMGLIBNAME) ...
	$(DSOCOMMAND) $(IMGLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_images.dll.a \
		-L. -lfltk -Wl,--exclude-libs -Wl,libfltk_png.a \
			$(IMAGELIBS) $(LDLIBS)

mgwfltknox-1.3.dll: $(LIBNAME)
	echo $(DSOCOMMAND) $(LIBNAME) ...
	$(DSOCOMMAND) $(LIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk.dll.a $(LDLIBS)

mgwfltknox_forms-1.3.dll: $(FLLIBNAME) mgwfltknox-1.3.dll
	echo $(DSOCOMMAND) $(FLLIBNAME) ...
	$(DSOCOMMAND) $(FLLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_forms.dll.a \
		-L. -lfltk $(LDLIBS)

mgwfltknox_gl-1.3.dll: $(GLLIBNAME) mgwfltknox-1.3.dll
	echo $(DSOCOMMAND) $(GLLIBNAME) ...
	$(DSOCOMMAND) $(GLLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_gl.dll.a \
		-L. -lfltk $(GLDLIBS)

#-----------------------------------------------------
# See STR #1585 for --exclude-libs
#-----------------------------------------------------

mgwfltknox_images-1.3.dll: $(IMGLIBNAME) mgwfltknox-1.3.dll
	echo $(DSOCOMMAND) $(IMGLIBNAME) ...
	$(DSOCOMMAND) $(IMGLIBNAME) -Wl,--no-whole-archive \
		-Wl,--out-implib=libfltk_images.dll.a \
		-L. -lfltk -Wl,--exclude-libs -Wl,libfltk_png.a \
			$(IMAGELIBS) $(LDLIBS)

clean:
	-$(RM) *.o xutf8/*.o *.dll.a core.* *~ *.bak *.bck
	-$(RM) $(DSONAME) $(FLDSONAME) $(GLDSONAME) $(IMGDSONAME) \
		$(LIBNAME) $(FLLIBNAME) $(GLLIBNAME) \
		$(IMGLIBNAME) \
		libfltk.so libfltk_forms.so libfltk_gl.so libfltk_images.so \
		libfltk.sl libfltk_forms.sl libfltk_gl.sl libfltk_images.sl \
		libfltk.dylib libfltk_forms.dylib \
		libfltk_gl.dylib libfltk_images.dylib \
		cmap core

depend:	$(CPPFILES) $(MMFILES) $(FLCPPFILES) $(GLCPPFILES) $(IMGCPPFILES) $(CFILES) $(UTF8CFILES)
	makedepend -Y -I.. -f makedepend $(CPPFILES) $(MMFILES) $(FLCPPFILES) \
		$(GLCPPFILES) $(IMGCPPFILES) $(CFILES) $(UTF8CFILES)

# Automatically generated dependencies... generated on a Linux/Unix host !
include makedepend

# These dependencies aren't part of the makedepend file since
# they are part of the WIN32 and MacOS code base...
# Please add only non-Linux/Unix files or such that are optional
# (like "*xft*") here:
Fl_get_key.o:	Fl_get_key_mac.cxx Fl_get_key_win32.cxx
Fl_Native_File_Chooser.o : Fl_Native_File_Chooser_WIN32.cxx
Fl_Native_File_Chooser_MAC.o:	Fl_Native_File_Chooser_MAC.mm
Fl.o:		Fl_win32.cxx
Fl_cocoa.o:	Fl_cocoa.mm
fl_color.o:	fl_color_mac.cxx fl_color_win32.cxx
fl_dnd.o:	fl_dnd_win32.cxx fl_dnd_x.cxx
fl_draw_image.o: fl_draw_image_mac.cxx fl_draw_image_win32.cxx
fl_font.o:	fl_font_mac.cxx fl_font_x.cxx fl_font_xft.cxx fl_font_win32.cxx
fl_read_image.o: fl_read_image_mac.cxx fl_read_image_win32.cxx
fl_set_fonts.o:	fl_set_fonts_mac.cxx fl_set_fonts_x.cxx \
		fl_set_fonts_xft.cxx fl_set_fonts_win32.cxx
Fl_Printer.o: Fl_GDI_Printer.cxx Fl_PostScript.cxx
Fl_Quartz_Printer.o:	Fl_Quartz_Printer.mm
fl_arci.o:	../FL/mac.H ../FL/win32.H
Fl_arg.o:	../FL/mac.H ../FL/win32.H
fl_ask.o:	../FL/mac.H ../FL/win32.H
Fl_Bitmap.o:	../FL/mac.H ../FL/win32.H
fl_color.o:	../FL/mac.H ../FL/win32.H
fl_cursor.o:	../FL/mac.H ../FL/win32.H
fl_dnd.o:	../FL/mac.H ../FL/win32.H
Fl.o:	../FL/mac.H ../FL/win32.H
Fl_Double_Window.o:	../FL/mac.H ../FL/win32.H
fl_draw_image.o:	../FL/mac.H ../FL/win32.H
fl_draw_image_mac.o:	../FL/mac.H ../FL/win32.H
fl_draw_image_win32.o:	../FL/mac.H ../FL/win32.H
fl_draw_pixmap.o:	../FL/mac.H ../FL/win32.H
fl_encoding_latin1.o:	../FL/mac.H ../FL/win32.H
fl_encoding_mac_roman.o:   ../FL/mac.H ../FL/win32.H
Fl_File_Chooser2.o:	../FL/mac.H ../FL/win32.H
fl_font.o:	../FL/mac.H ../FL/win32.H
Fl_get_key.o:	../FL/mac.H ../FL/win32.H
Fl_get_system_colors.o:	../FL/mac.H ../FL/win32.H
Fl_Gl_Choice.o:	../FL/mac.H ../FL/win32.H
Fl_Gl_Overlay.o:	../FL/mac.H ../FL/win32.H
Fl_Gl_Window.o:	../FL/mac.H ../FL/win32.H
Fl_grab.o:	../FL/mac.H ../FL/win32.H
Fl_Image.o:	../FL/mac.H ../FL/win32.H
fl_line_style.o:	../FL/mac.H ../FL/win32.H
Fl_Menu_Window.o:	../FL/mac.H ../FL/win32.H
fl_overlay.o:	../FL/mac.H ../FL/win32.H
fl_overlay_visual.o:	../FL/mac.H ../FL/win32.H
Fl_Overlay_Window.o:	../FL/mac.H ../FL/win32.H
Fl_own_colormap.o:	../FL/mac.H ../FL/win32.H
Fl_Pixmap.o:	../FL/mac.H ../FL/win32.H
Fl_Printer.o:  ../FL/mac.H ../FL/win32.H
fl_read_image.o:	../FL/mac.H ../FL/win32.H
fl_read_image_mac.o:	../FL/mac.H ../FL/win32.H
fl_read_image_win32.o:	../FL/mac.H ../FL/win32.H
fl_rect.o:	../FL/mac.H ../FL/win32.H
fl_scroll_area.o:	../FL/mac.H ../FL/win32.H
fl_set_font.o:	../FL/mac.H ../FL/win32.H
fl_set_fonts.o:	../FL/mac.H ../FL/win32.H
fl_shortcut.o:	../FL/mac.H ../FL/win32.H
fl_vertex.o:	../FL/mac.H ../FL/win32.H
Fl_visual.o:	../FL/mac.H ../FL/win32.H
Fl_Window_fullscreen.o:	../FL/mac.H ../FL/win32.H
Fl_Window_iconize.o:	../FL/mac.H ../FL/win32.H
Fl_x.o:	../FL/mac.H ../FL/win32.H
gl_draw.o:	../FL/mac.H ../FL/win32.H
gl_start.o:	../FL/mac.H ../FL/win32.H

################################################################

install: $(LIBNAME) $(DSONAME) \
	$(FLLIBNAME) $(FLDSONAME) \
	$(GLLIBNAME) $(GLDSONAME) \
	$(IMGLIBNAME) $(IMGDSONAME) 
	echo "Installing libraries in $(DESTDIR)$(libdir)..."
	-$(INSTALL_DIR) $(DESTDIR)$(libdir)
	-$(INSTALL_DIR) $(DESTDIR)$(bindir)
	$(RM) $(DESTDIR)$(libdir)/$(LIBBASENAME)
	$(INSTALL_LIB) $(LIBNAME) $(DESTDIR)$(libdir)
	$(INSTALL_LIB) $(FLLIBNAME) $(DESTDIR)$(libdir)
	$(INSTALL_LIB) $(IMGLIBNAME) $(DESTDIR)$(libdir)
	$(RANLIB) $(DESTDIR)$(libdir)/$(LIBBASENAME)
	$(RANLIB) $(DESTDIR)$(libdir)/$(FLLIBBASENAME)
	if test x$(GLLIBNAME) != x; then \
		$(INSTALL_LIB) $(GLLIBNAME) $(DESTDIR)$(libdir); \
		$(RANLIB) $(DESTDIR)$(libdir)/$(GLLIBBASENAME); \
	fi
	$(RANLIB) $(DESTDIR)$(libdir)/$(IMGLIBBASENAME)
	if test x$(DSONAME) = xlibfltk.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk.so*;\
		$(INSTALL_LIB) libfltk.so.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk.so.1.3 $(DESTDIR)$(libdir)/libfltk.so;\
	fi
	if test x$(DSONAME) = xlibfltk.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk.sl*;\
		$(INSTALL_LIB) libfltk.sl.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk.sl.1.3 $(DESTDIR)$(libdir)/libfltk.sl;\
	fi
	if test x$(DSONAME) = xlibfltk.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk.*dylib;\
		$(INSTALL_LIB) libfltk.1.3.dylib $(DESTDIR)$(libdir); \
		$(LN) libfltk.1.3.dylib $(DESTDIR)$(libdir)/libfltk.dylib;\
	fi
	if test x$(DSONAME) = xlibfltk_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_s.a;\
		$(INSTALL_LIB) libfltk_s.a $(DESTDIR)$(libdir); \
	fi
	if test x$(DSONAME) = xcygfltknox-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(DSONAME);\
		$(INSTALL_LIB) $(DSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk.dll.a;\
		$(INSTALL_LIB) libfltk.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(DSONAME) = xmgwfltknox-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(DSONAME);\
		$(INSTALL_LIB) $(DSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk.dll.a;\
		$(INSTALL_LIB) libfltk.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(FLDSONAME) = xlibfltk_forms.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.so*;\
		$(INSTALL_LIB) libfltk_forms.so.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk_forms.so.1.3 $(DESTDIR)$(libdir)/libfltk_forms.so;\
	fi
	if test x$(FLDSONAME) = xlibfltk_forms.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.sl*;\
		$(INSTALL_LIB) libfltk_forms.sl.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk_forms.sl.1.3 $(DESTDIR)$(libdir)/libfltk_forms.sl;\
	fi
	if test x$(FLDSONAME) = xlibfltk_forms.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.*dylib;\
		$(INSTALL_LIB) libfltk_forms.1.3.dylib $(DESTDIR)$(libdir); \
		$(LN) libfltk_forms.1.3.dylib $(DESTDIR)$(libdir)/libfltk_forms.dylib;\
	fi
	if test x$(FLDSONAME) = xlibfltk_forms_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms_s.a;\
		$(INSTALL_LIB) libfltk_forms_s.a $(DESTDIR)$(libdir); \
	fi
	if test x$(FLDSONAME) = xcygfltknox_forms-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(FLDSONAME);\
		$(INSTALL_LIB) $(FLDSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.dll.a;\
		$(INSTALL_LIB) libfltk_forms.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(FLDSONAME) = xmgwfltknox_forms-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(FLDSONAME);\
		$(INSTALL_LIB) $(FLDSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.dll.a;\
		$(INSTALL_LIB) libfltk_forms.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(GLDSONAME) = xlibfltk_gl.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.so*;\
		$(INSTALL_LIB) libfltk_gl.so.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk_gl.so.1.3 $(DESTDIR)$(libdir)/libfltk_gl.so;\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.sl*;\
		$(INSTALL_LIB) libfltk_gl.sl.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk_gl.sl.1.3 $(DESTDIR)$(libdir)/libfltk_gl.sl;\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.*dylib;\
		$(INSTALL_LIB) libfltk_gl.1.3.dylib $(DESTDIR)$(libdir); \
		$(LN) libfltk_gl.1.3.dylib $(DESTDIR)$(libdir)/libfltk_gl.dylib;\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl_s.a;\
		$(INSTALL_LIB) libfltk_gl_s.a $(DESTDIR)$(libdir); \
	fi
	if test x$(GLDSONAME) = xcygfltknox_gl-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(GLDSONAME);\
		$(INSTALL_LIB) $(GLDSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.dll.a;\
		$(INSTALL_LIB) libfltk_gl.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(GLDSONAME) = xmgwfltknox_gl-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(GLDSONAME);\
		$(INSTALL_LIB) $(GLDSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.dll.a;\
		$(INSTALL_LIB) libfltk_gl.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(IMGDSONAME) = xlibfltk_images.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.so*;\
		$(INSTALL_LIB) libfltk_images.so.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk_images.so.1.3 $(DESTDIR)$(libdir)/libfltk_images.so;\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.sl*;\
		$(INSTALL_LIB) libfltk_images.sl.1.3 $(DESTDIR)$(libdir); \
		$(LN) libfltk_images.sl.1.3 $(DESTDIR)$(libdir)/libfltk_images.sl;\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.*dylib;\
		$(INSTALL_LIB) libfltk_images.1.3.dylib $(DESTDIR)$(libdir); \
		$(LN) libfltk_images.1.3.dylib $(DESTDIR)$(libdir)/libfltk_images.dylib;\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images_s.a;\
		$(INSTALL_LIB) libfltk_images_s.a $(DESTDIR)$(libdir); \
	fi
	if test x$(IMGDSONAME) = xcygfltknox_images-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(IMGDSONAME); \
		$(INSTALL_LIB) $(IMGDSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.dll.a;\
		$(INSTALL_LIB) libfltk_images.dll.a $(DESTDIR)$(libdir); \
	fi
	if test x$(IMGDSONAME) = xmgwfltknox_images-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(IMGDSONAME); \
		$(INSTALL_LIB) $(IMGDSONAME) $(DESTDIR)$(bindir); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.dll.a;\
		$(INSTALL_LIB) libfltk_images.dll.a $(DESTDIR)$(libdir); \
	fi


uninstall:
	echo "Uninstalling libraries..."
	$(RM) $(DESTDIR)$(libdir)/$(LIBBASENAME)
	if test x$(DSONAME) = xlibfltk.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk.so*;\
	fi
	if test x$(DSONAME) = xlibfltk.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk.sl*;\
	fi
	if test x$(DSONAME) = xlibfltk.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk.*dylib;\
	fi
	if test x$(DSONAME) = xlibfltk_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_s.a;\
	fi
	if test x$(DSONAME) = xcygfltknox-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(DSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk.dll.a;\
	fi
	if test x$(DSONAME) = xcygfltk-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(DSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk.dll.a;\
	fi
	if test x$(DSONAME) = xmgwfltknox-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(DSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk.dll.a;\
	fi
	$(RM) $(DESTDIR)$(libdir)/$(FLLIBBASENAME);
	if test x$(FLDSONAME) = xlibfltk_forms.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.so*;\
	fi
	if test x$(FLDSONAME) = xlibfltk_forms.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.sl*;\
	fi
	if test x$(FLDSONAME) = xlibfltk_forms.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.*dylib;\
	fi
	if test x$(FLDSONAME) = xlibfltk_forms_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms_s.a;\
	fi
	if test x$(FLDSONAME) = xcygfltknox_forms-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(FLDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.dll.a;\
	fi
	if test x$(FLDSONAME) = xcygfltk_forms-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(FLDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.dll.a;\
	fi
	if test x$(FLDSONAME) = xmgwfltknox_forms-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(FLDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_forms.dll.a;\
	fi
	if test x$(GLLIBNAME) != x; then\
		$(RM) $(DESTDIR)$(libdir)/$(GLLIBBASENAME);\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.so*;\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.sl*;\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.*dylib;\
	fi
	if test x$(GLDSONAME) = xlibfltk_gl_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl_s.a;\
	fi
	if test x$(GLDSONAME) = xcygfltknox_gl-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(GLDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.dll.a;\
	fi
	if test x$(GLDSONAME) = xcygfltk_gl-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(GLDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.dll.a;\
	fi
	if test x$(GLDSONAME) = xmgwfltknox_gl-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(GLDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_gl.dll.a;\
	fi
	if test x$(IMGLIBNAME) != x; then\
		$(RM) $(DESTDIR)$(libdir)/$(IMGLIBBASENAME);\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images.so.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.so*;\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images.sl.1.3; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.sl*;\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images.1.3.dylib; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.*dylib;\
	fi
	if test x$(IMGDSONAME) = xlibfltk_images_s.a; then\
		$(RM) $(DESTDIR)$(libdir)/libfltk_images_s.a;\
	fi
	if test x$(IMGDSONAME) = xcygfltknox_images-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(IMGDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.dll.a;\
	fi
	if test x$(IMGDSONAME) = xcygfltk_images-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(IMGDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.dll.a;\
	fi
	if test x$(IMGDSONAME) = xmgwfltknox_images-1.3.dll; then\
		$(RM) $(DESTDIR)$(bindir)/$(IMGDSONAME); \
		$(RM) $(DESTDIR)$(libdir)/libfltk_images.dll.a;\
	fi

#
# End of "$Id: Makefile 8389 2011-02-06 14:08:08Z manolo $".
#
