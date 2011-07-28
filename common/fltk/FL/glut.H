//
// "$Id: glut.H 8623 2011-04-24 17:09:41Z AlbrechtS $"
//
// GLUT emulation header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

// Emulation of GLUT using fltk.

// GLUT is Copyright (c) Mark J. Kilgard, 1994, 1995, 1996:
// "This program is freely distributable without licensing fees  and is
// provided without guarantee or warrantee expressed or  implied. This
// program is -not- in the public domain."

// Although I have copied the GLUT API, none of my code is based on
// any GLUT implementation details and is therefore covered by the LGPL.

// Commented out lines indicate parts of GLUT that are not emulated.

#ifndef Fl_glut_H
#  define Fl_glut_H

#  include "gl.h"


#  include "Fl.H"
#  include "Fl_Gl_Window.H"

/** 
  GLUT is emulated using this window class and these static variables
  (plus several more static variables hidden in glut_compatability.cxx):
*/
class FL_EXPORT Fl_Glut_Window : public Fl_Gl_Window {
  void _init();
  int mouse_down;
protected:
  void draw();
  void draw_overlay();
  int handle(int);
public: // so the inline functions work
  int number;
  int menu[3];
  void make_current();
  void (*display)();
  void (*overlaydisplay)();
  void (*reshape)(int w, int h);
  void (*keyboard)(uchar, int x, int y);
  void (*mouse)(int b, int state, int x, int y);
  void (*motion)(int x, int y);
  void (*passivemotion)(int x, int y);
  void (*entry)(int);
  void (*visibility)(int);
  void (*special)(int, int x, int y);
  Fl_Glut_Window(int w, int h, const char *);
  Fl_Glut_Window(int x, int y, int w, int h, const char *);
  ~Fl_Glut_Window();
};

extern FL_EXPORT Fl_Glut_Window *glut_window;	// the current window
extern FL_EXPORT int glut_menu;			// the current menu

// function pointers that are not per-window:
extern FL_EXPORT void (*glut_idle_function)();
extern FL_EXPORT void (*glut_menustate_function)(int);
extern FL_EXPORT void (*glut_menustatus_function)(int,int,int);

////////////////////////////////////////////////////////////////

//#  define GLUT_API_VERSION This does not match any version of GLUT exactly...

FL_EXPORT void glutInit(int *argcp, char **argv); // creates first window

FL_EXPORT void glutInitDisplayMode(unsigned int mode);
// the FL_ symbols have the same value as the GLUT ones:
#  define GLUT_RGB	FL_RGB
#  define GLUT_RGBA	FL_RGB
#  define GLUT_INDEX	FL_INDEX
#  define GLUT_SINGLE	FL_SINGLE
#  define GLUT_DOUBLE	FL_DOUBLE
#  define GLUT_ACCUM	FL_ACCUM
#  define GLUT_ALPHA	FL_ALPHA
#  define GLUT_DEPTH	FL_DEPTH
#  define GLUT_STENCIL	FL_STENCIL
#  define GLUT_MULTISAMPLE FL_MULTISAMPLE
#  define GLUT_STEREO	FL_STEREO
// #  define GLUT_LUMINANCE		512

FL_EXPORT void glutInitWindowPosition(int x, int y);

FL_EXPORT void glutInitWindowSize(int w, int h);

FL_EXPORT void glutMainLoop();

FL_EXPORT int glutCreateWindow(char *title);
FL_EXPORT int glutCreateWindow(const char *title);

FL_EXPORT int glutCreateSubWindow(int win, int x, int y, int width, int height);

FL_EXPORT void glutDestroyWindow(int win);

inline void glutPostRedisplay() {glut_window->redraw();}

FL_EXPORT void glutPostWindowRedisplay(int win);

FL_EXPORT void glutSwapBuffers();

inline int glutGetWindow() {return glut_window->number;}

FL_EXPORT void glutSetWindow(int win);

inline void glutSetWindowTitle(char *t) {glut_window->label(t);}

inline void glutSetIconTitle(char *t) {glut_window->iconlabel(t);}

inline void glutPositionWindow(int x, int y) {glut_window->position(x,y);}

inline void glutReshapeWindow(int w, int h) {glut_window->size(w,h);}

inline void glutPopWindow() {glut_window->show();}

inline void glutPushWindow() { /* do nothing */ }

inline void glutIconifyWindow() {glut_window->iconize();}

inline void glutShowWindow() {glut_window->show();}

inline void glutHideWindow() {glut_window->hide();}

inline void glutFullScreen() {glut_window->fullscreen();}

inline void glutSetCursor(Fl_Cursor cursor) {glut_window->cursor(cursor);}
// notice that the numeric values are different than glut:
#  define GLUT_CURSOR_RIGHT_ARROW		((Fl_Cursor)2)
#  define GLUT_CURSOR_LEFT_ARROW		((Fl_Cursor)67)
#  define GLUT_CURSOR_INFO			FL_CURSOR_HAND
#  define GLUT_CURSOR_DESTROY			((Fl_Cursor)45)
#  define GLUT_CURSOR_HELP			FL_CURSOR_HELP
#  define GLUT_CURSOR_CYCLE			((Fl_Cursor)26)
#  define GLUT_CURSOR_SPRAY			((Fl_Cursor)63)
#  define GLUT_CURSOR_WAIT			FL_CURSOR_WAIT
#  define GLUT_CURSOR_TEXT			FL_CURSOR_INSERT
#  define GLUT_CURSOR_CROSSHAIR			FL_CURSOR_CROSS
#  define GLUT_CURSOR_UP_DOWN			FL_CURSOR_NS
#  define GLUT_CURSOR_LEFT_RIGHT		FL_CURSOR_WE
#  define GLUT_CURSOR_TOP_SIDE			FL_CURSOR_N
#  define GLUT_CURSOR_BOTTOM_SIDE		FL_CURSOR_S
#  define GLUT_CURSOR_LEFT_SIDE			FL_CURSOR_W
#  define GLUT_CURSOR_RIGHT_SIDE		FL_CURSOR_E
#  define GLUT_CURSOR_TOP_LEFT_CORNER		FL_CURSOR_NW
#  define GLUT_CURSOR_TOP_RIGHT_CORNER		FL_CURSOR_NE
#  define GLUT_CURSOR_BOTTOM_RIGHT_CORNER	FL_CURSOR_SE
#  define GLUT_CURSOR_BOTTOM_LEFT_CORNER	FL_CURSOR_SW
#  define GLUT_CURSOR_INHERIT			FL_CURSOR_DEFAULT
#  define GLUT_CURSOR_NONE			FL_CURSOR_NONE
#  define GLUT_CURSOR_FULL_CROSSHAIR		FL_CURSOR_CROSS

inline void glutWarpPointer(int, int) { /* do nothing */ }

inline void glutEstablishOverlay() {glut_window->make_overlay_current();}

inline void glutRemoveOverlay() {glut_window->hide_overlay();}

inline void glutUseLayer(GLenum layer) {
  layer ? glut_window->make_overlay_current() : glut_window->make_current();}
enum {GLUT_NORMAL, GLUT_OVERLAY};

inline void glutPostOverlayRedisplay() {glut_window->redraw_overlay();}

inline void glutShowOverlay() {glut_window->redraw_overlay();}

inline void glutHideOverlay() {glut_window->hide_overlay();}

FL_EXPORT int glutCreateMenu(void (*)(int));

FL_EXPORT void glutDestroyMenu(int menu);

inline int glutGetMenu() {return glut_menu;}

inline void glutSetMenu(int m) {glut_menu = m;}

FL_EXPORT void glutAddMenuEntry(char *label, int value);

FL_EXPORT void glutAddSubMenu(char *label, int submenu);

FL_EXPORT void glutChangeToMenuEntry(int item, char *labela, int value);

FL_EXPORT void glutChangeToSubMenu(int item, char *label, int submenu);

FL_EXPORT void glutRemoveMenuItem(int item);

inline void glutAttachMenu(int b) {glut_window->menu[b] = glut_menu;}

inline void glutDetachMenu(int b) {glut_window->menu[b] = 0;}

inline void glutDisplayFunc(void (*f)()) {glut_window->display = f;}

inline void glutReshapeFunc(void (*f)(int w, int h)) {glut_window->reshape=f;}

inline void glutKeyboardFunc(void (*f)(uchar key, int x, int y)) {
  glut_window->keyboard = f;}

inline void glutMouseFunc(void (*f)(int b, int state, int x, int y)) {
  glut_window->mouse = f;}
#  define GLUT_LEFT_BUTTON		0
#  define GLUT_MIDDLE_BUTTON		1
#  define GLUT_RIGHT_BUTTON		2
#  define GLUT_DOWN			0
#  define GLUT_UP			1

inline void glutMotionFunc(void (*f)(int x, int y)) {glut_window->motion= f;}

inline void glutPassiveMotionFunc(void (*f)(int x, int y)) {
  glut_window->passivemotion= f;}

inline void glutEntryFunc(void (*f)(int s)) {glut_window->entry = f;}
enum {GLUT_LEFT, GLUT_ENTERED};

inline void glutVisibilityFunc(void (*f)(int s)) {glut_window->visibility=f;}
enum {GLUT_NOT_VISIBLE, GLUT_VISIBLE};

FL_EXPORT void glutIdleFunc(void (*f)());

// Warning: this cast may not work on all machines:
inline void glutTimerFunc(unsigned int msec, void (*f)(int), int value) {
  Fl::add_timeout(msec*.001, (void (*)(void *))f, (void *)value);
}

inline void glutMenuStateFunc(void (*f)(int state)) {
  glut_menustate_function = f;}

inline void glutMenuStatusFunc(void (*f)(int status, int x, int y)) {
  glut_menustatus_function = f;}
enum {GLUT_MENU_NOT_IN_USE, GLUT_MENU_IN_USE};

inline void glutSpecialFunc(void (*f)(int key, int x, int y)) {
  glut_window->special = f;}
#  define GLUT_KEY_F1			1
#  define GLUT_KEY_F2			2
#  define GLUT_KEY_F3			3
#  define GLUT_KEY_F4			4
#  define GLUT_KEY_F5			5
#  define GLUT_KEY_F6			6
#  define GLUT_KEY_F7			7
#  define GLUT_KEY_F8			8
#  define GLUT_KEY_F9			9
#  define GLUT_KEY_F10			10
#  define GLUT_KEY_F11			11
#  define GLUT_KEY_F12			12
// WARNING: Different values than GLUT uses:
#  define GLUT_KEY_LEFT			FL_Left
#  define GLUT_KEY_UP			FL_Up
#  define GLUT_KEY_RIGHT		FL_Right
#  define GLUT_KEY_DOWN			FL_Down
#  define GLUT_KEY_PAGE_UP		FL_Page_Up
#  define GLUT_KEY_PAGE_DOWN		FL_Page_Down
#  define GLUT_KEY_HOME			FL_Home
#  define GLUT_KEY_END			FL_End
#  define GLUT_KEY_INSERT		FL_Insert

//inline void glutSpaceballMotionFunc(void (*)(int x, int y, int z));

//inline void glutSpaceballRotateFunc(void (*)(int x, int y, int z));

//inline void glutSpaceballButtonFunc(void (*)(int button, int state));

//inline void glutButtonBoxFunc(void (*)(int button, int state));

//inline void glutDialsFunc(void (*)(int dial, int value));

//inline void glutTabletMotionFunc(void (*)(int x, int y));

//inline void glutTabletButtonFunc(void (*)(int button, int state, int x, int y));

inline void glutOverlayDisplayFunc(void (*f)()) {
  glut_window->overlaydisplay = f;}

//inline void glutWindowStatusFunc(void (*)(int state));
//enum {GLUT_HIDDEN, GLUT_FULLY_RETAINED, GLUT_PARTIALLY_RETAINED,
//	GLUT_FULLY_COVERED};

//inline void glutSetColor(int, GLfloat red, GLfloat green, GLfloat blue);

//inline GLfloat glutGetColor(int ndx, int component);
//#define GLUT_RED			0
//#define GLUT_GREEN			1
//#define GLUT_BLUE			2

//inline void glutCopyColormap(int win);

// Warning: values are changed from GLUT!
// Also relies on the GL_ symbols having values greater than 100
FL_EXPORT int glutGet(GLenum type);
enum {
  GLUT_RETURN_ZERO = 0,
  GLUT_WINDOW_X,
  GLUT_WINDOW_Y,
  GLUT_WINDOW_WIDTH,
  GLUT_WINDOW_HEIGHT,
  GLUT_WINDOW_PARENT,
  GLUT_SCREEN_WIDTH,
  GLUT_SCREEN_HEIGHT,
  GLUT_MENU_NUM_ITEMS,
  GLUT_DISPLAY_MODE_POSSIBLE,
  GLUT_INIT_WINDOW_X,
  GLUT_INIT_WINDOW_Y,
  GLUT_INIT_WINDOW_WIDTH,
  GLUT_INIT_WINDOW_HEIGHT,
  GLUT_INIT_DISPLAY_MODE,
  GLUT_WINDOW_BUFFER_SIZE,
  GLUT_VERSION
//GLUT_WINDOW_NUM_CHILDREN,
//GLUT_WINDOW_CURSOR,
//GLUT_SCREEN_WIDTH_MM,
//GLUT_SCREEN_HEIGHT_MM,
//GLUT_ELAPSED_TIME,
};

#  define GLUT_WINDOW_STENCIL_SIZE	GL_STENCIL_BITS
#  define GLUT_WINDOW_DEPTH_SIZE	GL_DEPTH_BITS
#  define GLUT_WINDOW_RED_SIZE		GL_RED_BITS
#  define GLUT_WINDOW_GREEN_SIZE	GL_GREEN_BITS
#  define GLUT_WINDOW_BLUE_SIZE		GL_BLUE_BITS
#  define GLUT_WINDOW_ALPHA_SIZE	GL_ALPHA_BITS
#  define GLUT_WINDOW_ACCUM_RED_SIZE	GL_ACCUM_RED_BITS
#  define GLUT_WINDOW_ACCUM_GREEN_SIZE	GL_ACCUM_GREEN_BITS
#  define GLUT_WINDOW_ACCUM_BLUE_SIZE	GL_ACCUM_BLUE_BITS
#  define GLUT_WINDOW_ACCUM_ALPHA_SIZE	GL_ACCUM_ALPHA_BITS
#  define GLUT_WINDOW_DOUBLEBUFFER	GL_DOUBLEBUFFER
#  define GLUT_WINDOW_RGBA		GL_RGBA
#  define GLUT_WINDOW_COLORMAP_SIZE	GL_INDEX_BITS
#  ifdef GL_SAMPLES_SGIS
#    define GLUT_WINDOW_NUM_SAMPLES	GL_SAMPLES_SGIS
#  else
#    define GLUT_WINDOW_NUM_SAMPLES	GLUT_RETURN_ZERO
#  endif
#  define GLUT_WINDOW_STEREO		GL_STEREO

#  define GLUT_HAS_KEYBOARD		600
#  define GLUT_HAS_MOUSE		601
#  define GLUT_HAS_SPACEBALL		602
#  define GLUT_HAS_DIAL_AND_BUTTON_BOX	603
#  define GLUT_HAS_TABLET		604
#  define GLUT_NUM_MOUSE_BUTTONS	605
#  define GLUT_NUM_SPACEBALL_BUTTONS	606
#  define GLUT_NUM_BUTTON_BOX_BUTTONS	607
#  define GLUT_NUM_DIALS		608
#  define GLUT_NUM_TABLET_BUTTONS	609
FL_EXPORT int glutDeviceGet(GLenum type);

// WARNING: these values are different than GLUT uses:
#  define GLUT_ACTIVE_SHIFT               FL_SHIFT
#  define GLUT_ACTIVE_CTRL                FL_CTRL
#  define GLUT_ACTIVE_ALT                 FL_ALT
inline int glutGetModifiers() {return Fl::event_state() & (GLUT_ACTIVE_SHIFT | GLUT_ACTIVE_CTRL | GLUT_ACTIVE_ALT);}

FL_EXPORT int glutLayerGet(GLenum);
#  define GLUT_OVERLAY_POSSIBLE		800
//#define GLUT_LAYER_IN_USE		801
//#define GLUT_HAS_OVERLAY		802
#  define GLUT_TRANSPARENT_INDEX		803
#  define GLUT_NORMAL_DAMAGED		804
#  define GLUT_OVERLAY_DAMAGED		805

extern "C" {
typedef void (*GLUTproc)();
}

FL_EXPORT GLUTproc glutGetProcAddress(const char *procName);

//inline int glutVideoResizeGet(GLenum param);
//#define GLUT_VIDEO_RESIZE_POSSIBLE	900
//#define GLUT_VIDEO_RESIZE_IN_USE	901
//#define GLUT_VIDEO_RESIZE_X_DELTA	902
//#define GLUT_VIDEO_RESIZE_Y_DELTA	903
//#define GLUT_VIDEO_RESIZE_WIDTH_DELTA	904
//#define GLUT_VIDEO_RESIZE_HEIGHT_DELTA 905
//#define GLUT_VIDEO_RESIZE_X		906
//#define GLUT_VIDEO_RESIZE_Y		907
//#define GLUT_VIDEO_RESIZE_WIDTH	908
//#define GLUT_VIDEO_RESIZE_HEIGHT	909

//inline void glutSetupVideoResizing();

//inline void glutStopVideoResizing();

//inline void glutVideoResize(int x, int y, int width, int height);

//inline void glutVideoPan(int x, int y, int width, int height);

// Font argument must be a void* for compatibility, so...
/** fltk glut font/size attributes used in the glutXXX functions */
struct Fl_Glut_Bitmap_Font {Fl_Font font; Fl_Fontsize size;};

extern FL_EXPORT struct Fl_Glut_Bitmap_Font
  glutBitmap9By15, glutBitmap8By13, glutBitmapTimesRoman10,
  glutBitmapTimesRoman24, glutBitmapHelvetica10, glutBitmapHelvetica12,
  glutBitmapHelvetica18;
#  define GLUT_BITMAP_9_BY_15             (&glutBitmap9By15)
#  define GLUT_BITMAP_8_BY_13             (&glutBitmap8By13)
#  define GLUT_BITMAP_TIMES_ROMAN_10      (&glutBitmapTimesRoman10)
#  define GLUT_BITMAP_TIMES_ROMAN_24      (&glutBitmapTimesRoman24)
#  define GLUT_BITMAP_HELVETICA_10        (&glutBitmapHelvetica10)
#  define GLUT_BITMAP_HELVETICA_12        (&glutBitmapHelvetica12)
#  define GLUT_BITMAP_HELVETICA_18        (&glutBitmapHelvetica18)

FL_EXPORT void glutBitmapCharacter(void *font, int character);
FL_EXPORT int glutBitmapHeight(void *font);
FL_EXPORT int glutBitmapLength(void *font, const unsigned char *string);
FL_EXPORT void glutBitmapString(void *font, const unsigned char *string);
FL_EXPORT int glutBitmapWidth(void *font, int character);

FL_EXPORT int glutExtensionSupported(char *name);

/* GLUT stroked font sub-API */
struct Fl_Glut_StrokeVertex {
  GLfloat X, Y;
};

struct Fl_Glut_StrokeStrip {
  int Number;
  const Fl_Glut_StrokeVertex* Vertices;
};

struct Fl_Glut_StrokeChar {
  GLfloat Right;
  int Number;
  const Fl_Glut_StrokeStrip* Strips;
};

struct Fl_Glut_StrokeFont {
  char* Name;				// The source font name
  int Quantity;				// Number of chars in font
  GLfloat Height;			// Height of the characters
  const Fl_Glut_StrokeChar** Characters;// The characters mapping
};
extern FL_EXPORT Fl_Glut_StrokeFont glutStrokeRoman;
extern FL_EXPORT Fl_Glut_StrokeFont glutStrokeMonoRoman;
#  define GLUT_STROKE_ROMAN		(&glutStrokeRoman)
#  define GLUT_STROKE_MONO_ROMAN	(&glutStrokeMonoRoman)

FL_EXPORT void glutStrokeCharacter(void *font, int character);
FL_EXPORT GLfloat glutStrokeHeight(void *font);
FL_EXPORT int glutStrokeLength(void *font, const unsigned char *string);
FL_EXPORT void glutStrokeString(void *font, const unsigned char *string);
FL_EXPORT int glutStrokeWidth(void *font, int character);

/* GLUT pre-built models sub-API */
FL_EXPORT void glutWireSphere(GLdouble radius, GLint slices, GLint stacks);
FL_EXPORT void glutSolidSphere(GLdouble radius, GLint slices, GLint stacks);
FL_EXPORT void glutWireCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);
FL_EXPORT void glutSolidCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);
FL_EXPORT void glutWireCube(GLdouble size);
FL_EXPORT void glutSolidCube(GLdouble size);
FL_EXPORT void glutWireTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
FL_EXPORT void glutSolidTorus(GLdouble innerRadius, GLdouble outerRadius, GLint sides, GLint rings);
FL_EXPORT void glutWireDodecahedron();
FL_EXPORT void glutSolidDodecahedron();
FL_EXPORT void glutWireTeapot(GLdouble size);
FL_EXPORT void glutSolidTeapot(GLdouble size);
FL_EXPORT void glutWireOctahedron();
FL_EXPORT void glutSolidOctahedron();
FL_EXPORT void glutWireTetrahedron();
FL_EXPORT void glutSolidTetrahedron();
FL_EXPORT void glutWireIcosahedron();
FL_EXPORT void glutSolidIcosahedron();

#endif // !Fl_glut_H

//
// End of "$Id: glut.H 8623 2011-04-24 17:09:41Z AlbrechtS $".
//
