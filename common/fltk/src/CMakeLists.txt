include_regular_expression("[.][cxx|c|h]$")

set(CPPFILES
  Fl.cxx
  Fl_Adjuster.cxx
  Fl_Bitmap.cxx
  Fl_Browser.cxx
  Fl_Browser_.cxx
  Fl_Browser_load.cxx
  Fl_Box.cxx
  Fl_Button.cxx
  Fl_Chart.cxx
  Fl_Check_Browser.cxx
  Fl_Check_Button.cxx
  Fl_Choice.cxx
  Fl_Clock.cxx
  Fl_Color_Chooser.cxx
  Fl_Counter.cxx
  Fl_Device.cxx
  Fl_Dial.cxx
  Fl_Double_Window.cxx
  Fl_File_Browser.cxx
  Fl_File_Chooser.cxx
  Fl_File_Chooser2.cxx
  Fl_File_Icon.cxx
  Fl_File_Input.cxx
  Fl_Group.cxx
  Fl_Help_View.cxx
  Fl_Image.cxx
  Fl_Input.cxx
  Fl_Input_.cxx
  Fl_Light_Button.cxx
  Fl_Menu.cxx
  Fl_Menu_.cxx
  Fl_Menu_Bar.cxx
  Fl_Sys_Menu_Bar.cxx
  Fl_Menu_Button.cxx
  Fl_Menu_Window.cxx
  Fl_Menu_add.cxx
  Fl_Menu_global.cxx
  Fl_Multi_Label.cxx
  Fl_Native_File_Chooser.cxx
  Fl_Overlay_Window.cxx
  Fl_Pack.cxx
  Fl_Paged_Device.cxx
  Fl_Pixmap.cxx
  Fl_PNG_Image.cxx
  Fl_Positioner.cxx
  Fl_Printer.cxx
  Fl_Preferences.cxx
  Fl_Progress.cxx
  Fl_Repeat_Button.cxx
  Fl_Return_Button.cxx
  Fl_Roller.cxx
  Fl_Round_Button.cxx
  Fl_Scroll.cxx
  Fl_Scrollbar.cxx
  Fl_Shared_Image.cxx
  Fl_Single_Window.cxx
  Fl_Slider.cxx
  Fl_Table.cxx
  Fl_Table_Row.cxx
  Fl_Tabs.cxx
  Fl_Text_Buffer.cxx
  Fl_Text_Display.cxx
  Fl_Text_Editor.cxx
  Fl_Tile.cxx
  Fl_Tiled_Image.cxx
  Fl_Tooltip.cxx
  Fl_Tree.cxx
  Fl_Tree_Item_Array.cxx
  Fl_Tree_Item.cxx
  Fl_Tree_Prefs.cxx
  Fl_Valuator.cxx
  Fl_Value_Input.cxx
  Fl_Value_Output.cxx
  Fl_Value_Slider.cxx
  Fl_Widget.cxx
  Fl_Window.cxx
  Fl_Window_fullscreen.cxx
  Fl_Window_hotspot.cxx
  Fl_Window_iconize.cxx
  Fl_Wizard.cxx
  Fl_XBM_Image.cxx
  Fl_XPM_Image.cxx
  Fl_abort.cxx
  Fl_add_idle.cxx
  Fl_arg.cxx
  Fl_compose.cxx
  Fl_display.cxx
  Fl_get_key.cxx
  Fl_get_system_colors.cxx
  Fl_grab.cxx
  Fl_lock.cxx
  Fl_own_colormap.cxx
  Fl_visual.cxx
  Fl_x.cxx
  filename_absolute.cxx
  filename_expand.cxx
  filename_ext.cxx
  filename_isdir.cxx
  filename_list.cxx
  filename_match.cxx
  filename_setext.cxx
  fl_arc.cxx
  fl_arci.cxx
  fl_ask.cxx
  fl_boxtype.cxx
  fl_color.cxx
  fl_cursor.cxx
  fl_curve.cxx
  fl_diamond_box.cxx
  fl_dnd.cxx
  fl_draw.cxx
  fl_draw_image.cxx
  fl_draw_pixmap.cxx
  fl_engraved_label.cxx
  fl_file_dir.cxx
  fl_font.cxx
  fl_gtk.cxx
  fl_labeltype.cxx
  fl_line_style.cxx
  fl_open_uri.cxx
  fl_oval_box.cxx
  fl_overlay.cxx
  fl_overlay_visual.cxx
  fl_plastic.cxx
  fl_read_image.cxx
  fl_rect.cxx
  fl_round_box.cxx
  fl_rounded_box.cxx
  fl_set_font.cxx
  fl_set_fonts.cxx
  fl_scroll_area.cxx
  fl_shadow_box.cxx
  fl_shortcut.cxx
  fl_show_colormap.cxx
  fl_symbols.cxx
  fl_vertex.cxx
  ps_image.cxx
  screen_xywh.cxx
  fl_utf8.cxx
  fl_encoding_latin1.cxx
  fl_encoding_mac_roman.cxx
)

set(CFILES
  flstring.c
  scandir.c
  numericsort.c
  vsnprintf.c
  xutf8/is_right2left.c
  xutf8/is_spacing.c
  xutf8/case.c
  xutf8/utf8Input.c
  xutf8/utf8Utils.c
  xutf8/utf8Wrap.c
  xutf8/keysym2Ucs.c
  fl_utf.c
)

add_definitions(-DFL_LIBRARY)
if(APPLE)
	set(MMFILES
		Fl_cocoa.mm
		Fl_Quartz_Printer.mm
		Fl_Native_File_Chooser_MAC.mm
		)
else()
   set(MMFILES
      )
endif(APPLE)

#######################################################################
add_library(fltk_static STATIC ${CPPFILES} ${MMFILES} ${CFILES} fl_call_main.c)
set_target_properties(fltk_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

if(USE_THREADS)
   target_link_libraries(fltk_static ${CMAKE_THREAD_LIBS_INIT})
endif(USE_THREADS)

if(USE_X11)
   target_link_libraries(fltk_static ${X11_LIBRARIES})
endif(USE_X11)

if(WIN32)
   target_link_libraries(fltk_static comctl32)
endif(WIN32)

if(HAVE_XINERAMA)
   target_link_libraries(fltk_static ${X11_Xinerama_LIB})
endif(HAVE_XINERAMA)

if(USE_XFT)
   target_link_libraries(fltk_static ${X11_Xft_LIB})
endif(USE_XFT)

if(HAVE_XFIXES)
   target_link_libraries(fltk_static ${X11_Xfixes_LIB})
endif(HAVE_XFIXES)

if(HAVE_XCURSOR)
   target_link_libraries(fltk_static ${X11_Xcursor_LIB})
endif(HAVE_XCURSOR)
