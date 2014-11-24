/*
  hale: support for minimalist scientific visualization
  Copyright (C) 2014  University of Chicago

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software. Permission is granted to anyone to
  use this software for any purpose, including commercial applications, and
  to alter it and redistribute it freely, subject to the following
  restrictions:

  1. The origin of this software must not be misrepresented; you must not
  claim that you wrote the original software. If you use this software in a
  product, an acknowledgment in the product documentation would be
  appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef HALE_INCLUDED
#define HALE_INCLUDED

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#  define __gl_h_ /* to avoid: "warning: gl.h and gl3.h are both included" */
#else
#  include <GL/gl3.h>
#endif

#include <teem/meet.h> /* will include all other teem headers */

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

namespace Hale {

/*
** viewerMode* enum
**
** The GUI modes that the viewer can be in. In Fov and Depth (distance from
** near to far adjusted), the eye and look-at point are both fixed. The eye
** moves around a fixed look-at point in the Rotate and Dolly modes. The eye
** and look-at points move together in the Translate modes.
*/
enum {
  viewerModeUnknown,     /*  0 */
  viewerModeNone,        /*  1: buttons released => no camera interaction */
  viewerModeFov,         /*  2: standard "zoom" */
  viewerModeDepth,       /*  3: adjust distance between near and far planes */
  viewerModeRotateUV,    /*  4: usual rotate (around look-at point) */
  viewerModeRotateU,     /*  5: rotate around horizontal axis */
  viewerModeRotateV,     /*  6: rotate around vertical axis */
  viewerModeRotateN,     /*  7: in-plane rotate (around at point) */
  viewerModeDolly,       /*  8: fix at, move from, adjust fov: the effect is
                                direct control on the amount of perspective */
  viewerModeTranslateUV, /*  9: usual translate */
  viewerModeTranslateU,  /* 10: translate only horizontal */
  viewerModeTranslateV,  /* 11: translate only vertical */
  viewerModeTranslateN,  /* 12: translate from *and* at along view direction */
  viewerModeLast
};
#define HALE_VIEWER_MODE_MAX 12

enum {
  finishingStatusUnknown,      /* 0 */
  finishingStatusNot,          /* 1: we're still running */
  finishingStatusOkay,         /* 2: we're quitting gracefully */
  finishingStatusError,        /* 3: we're exiting with error */
  finishingStatusLast
};
#define HALE_FINISHING_STATUS_MAX 3

/* enums.cpp */
extern airEnum *viewerMode;
extern airEnum *finishingStatus;

/* utils.cpp */
extern bool finishing;
extern int init();
extern void done();
extern void errorGLFW(int errnum, const char *errstr);
extern GLuint limnToGLPrim(int type);

/* Viewer.cpp: Viewer contains and manages a GLFW window, including the
   camera that defines the view within the viewer.  We intercept all
   the events in order to handle how the camera is updated */
class Viewer {
 public:
  explicit Viewer(int width,  int height, const char *label);
  ~Viewer();

  int verbose();
  void verbose(int);

  int width();
  int height();
  void widthHeight(int ww, int hh);

  glm::vec3 from();
  void from(glm::vec3);
  glm::vec3 at();
  void at(glm::vec3);
  glm::vec3 up();
  void up(glm::vec3);

  double fov();
  void fov(double);

  bool upFix();
  void upFix(bool);

  bool orthographic();
  void orthographic(bool);

  void bufferSwap();
 private:
  bool _button[2];   // true iff button (left [0] or right [1]) is now down
  int _verbose, _width, _height;
  limnCamera *_camera;       // the camera we update, and own;
  bool _upFix;
  int _mode;                 // from Hale::viewerMode* enum

  GLFWwindow *_window;
  static void mouseButtonCB(GLFWwindow *gwin, int button, int action, int mods);

  void cameraUpdate();
};

} // namespace Hale

#endif /* HALE_INCLUDED */
