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

/*
** TODO:

map mouse motions to camera updates

change near and far back to being at-relative
change FOV to being in degrees
remove use of glm::lookat and glm::perspective
make _window private again

should the viewer do re-render by backback?  Or assume caller can re-render?
(e.g. changing from perspective to orthographic)

viewer capture/dump to image

generating hest options

interpreting hest options

*/

#include "Hale.h"
#include "privateHale.h"

/* the fraction of window width along its border that will be treated
   as its margin, to support the different kinds of interactions
   triggered there */
#define MARG 0.16

namespace Hale {

/* utility for mapping GLFW's button and modifier info into either
   "left button" (return 0) or "right button" (return 1). This
   is the one place to implement conventions for what things on a Mac
   should be interpreted as a right click */
static int buttonIdx(int button, int mods) {
  int ret;
  if (!mods) {
    /* with no modifiers, we interpret the button literally */
    ret = (button == GLFW_MOUSE_BUTTON_1
           ? 0 /* the first button, i.e. left */
           : 1 /* something other than first, i.e. right */);
  } else {
    /* with any modifier, we treat it as a right click;
       which is simplistic but workable on a mac, and
       weird but hopefully tolerable on linux */
    ret = 1;
  }
  return ret;
}

/*
** GLK believes that the window size callback is not actually needed: any
** window resize will also resize the framebuffer, and moving a window
** between display of different pixel densities will resize the framebuffer
** (but not the window)

void
Viewer::windowSizeCB(GLFWwindow *gwin, int newWidth, int newHeight) {
  static const char me[]="windowSizeCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose()) {
    printf("%s(%d,%d)\n", me, newWidth, newHeight);
  }
  vwr->_widthScreen = newWidth;
  vwr->_heightScreen = newHeight;
  glfwGetFramebufferSize(gwin, &(vwr->_widthBuffer), &(vwr->_heightBuffer));
  vwr->shapeUpdate();
  vwr->cameraUpdate();
  return;
}

*/

void
Viewer::framebufferSizeCB(GLFWwindow *gwin, int newWidth, int newHeight) {
  static const char me[]="framebufferSizeCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose()) {
    printf("%s(%d,%d)\n", me, newWidth, newHeight);
  }
  vwr->_widthBuffer = newWidth;
  vwr->_heightBuffer = newHeight;
  glfwGetWindowSize(gwin, &(vwr->_widthScreen), &(vwr->_heightScreen));
  vwr->shapeUpdate();
  vwr->cameraUpdate();
  return;
}

void
Viewer::keyCB(GLFWwindow *gwin, int key, int scancode, int action, int mods) {
  static const char me[]="keyCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose()) {
    printf("%s(%d,%d,%d,%d)\n", me, key, scancode, action, mods);
  }
  if (GLFW_KEY_Q == key && GLFW_PRESS == action && mods) {
    /* this is where we handle modifer+'q' as 'quit' on linux;
       actually *any* modifier will do. */
    finishing = true;
  }

  return;
}

void
Viewer::windowCloseCB(GLFWwindow *gwin) {
  static const char me[]="windowCloseCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose()) {
    printf("%s()\n", me);
  }
  finishing = true;

  return;
}

void
Viewer::mouseButtonCB(GLFWwindow *gwin, int button, int action, int mods) {
  static const char me[]="mouseButtonCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));
  double xpos, ypos, xf, yf;

  /* on Macs you can "right-click" downwards, via modifier keys, but then
     release the modifier keys before releasing the button, so tracking the
     logic of releasing the "right-click" get get messy.  Hence *any*
     release, is interpreted as a release of both buttons */
  if (GLFW_RELEASE == action) {
    vwr->_button[0] = vwr->_button[1] = false;
  } else { /* button press */
    vwr->_button[buttonIdx(button, mods)] = true;
  }
  if (vwr->verbose()) {
    printf("%s(%d,%s,%d) --> ( %s | %s )\n", me, button,
           (GLFW_PRESS == action
            ? " press "
            : "release"), mods,
           vwr->_button[0] ? "_" : "^", vwr->_button[1] ? "_" : "^");
  }
  glfwGetCursorPos(gwin, &xpos, &ypos);
  xf = xpos/vwr->_widthScreen;
  yf = ypos/vwr->_heightScreen;
  /* lots of conditions can lead to ceasing interactions with camera */
  if ( !(vwr->_button[0] || vwr->_button[1])
       /* both buttons up => we're done */
       ||
       (vwr->_button[0] && vwr->_button[1])
       /* weird; both buttons down: ambiguous => we're done */
       ||
       !( 0 < xf && xf < 1 && 0 < yf && yf < 1 )
       /* click didn't seem to be inside window; nothing to do */ ) {
    vwr->_mode = viewerModeNone;
  } else {
    /*
    ** else click was inside window; figure out new camera interaction mode.
    ** Diagram of how clicking in different parts of the window do
    ** different things with the camera.  "foo/bar" means that foo
    ** happens with left click, and bar happens with right click
    **     x=0                                     x=1
    ** y=0  +---------------------------------------+
    **      | \          X  RotateV/TranslateV    / |
    **      |   \  . . . . . . . . . . . . . .  /   |
    **      |     :                           :     |
    **      |     :                           :     |
    **      |     :                           :     |
    **      |     :                           :  X RotateU/
    **      |     :     X RotateUV/           :    TranslateU
    **      |     :       TranslateUV         :     |
    **      |  X Fov                          :     |
    **      |     :                           :     |
    **      |     :                           :     |
    **      |     :                           :     |
    **      |. . . . . . . . . . . . . . . . . \    |
    **      |  X  :      X  RotateN/TranslateN   \  |
    ** y=1  +--|------------------------------------+
    **         \__ Dolly/Depth
    */
    if (xf <= MARG && yf > 1-MARG) {
      vwr->_mode = (vwr->_button[0] ? viewerModeDolly : viewerModeDepth);
    } else if (xf <= MARG && xf <= yf) {
      vwr->_mode = viewerModeFov;
    } else if (yf <= MARG && xf > yf && 1-xf >= yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateV : viewerModeTranslateV);
    } else if (xf > 1-MARG && 1-xf < yf && 1-xf < 1-yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateU : viewerModeTranslateU);
    } else if (yf > 1-MARG && 1-xf >= 1-yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateN : viewerModeTranslateN);
    } else {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateUV : viewerModeTranslateUV);
    }
  }
  if (viewerModeNone != vwr->_mode) {
    vwr->_lastX = vwr->_lastY = AIR_NAN;
  } else {
    vwr->_lastX = xpos;
    vwr->_lastY = ypos;
  }
  if (vwr->verbose()) {
    printf("  @ (%g,%g) -> (%g,%g) -> %s\n", xpos, ypos, xf, yf,
           airEnumStr(viewerMode, vwr->_mode));
  }
  return;
}

void
Viewer::cursorPosCB(GLFWwindow *gwin, double xx, double yy) {
  static const char me[]="cursorPosCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));
  float deltaX, deltaY;

  if (viewerModeNone == vwr->_mode) {
    /* nothing to do here */
    return;
  }
  /* else we are moving the camera around */
  if (vwr->verbose()) {
    printf("%s(%g,%g): (%s) hello\n", me, xx, yy,
           airEnumStr(viewerMode, vwr->_mode));
  }
  deltaX = 0.01*(xx - vwr->_lastX);
  deltaY = 0.01*(yy - vwr->_lastY);

  glm::vec3 eye = glm::normalize(vwr->camera.from() - vwr->camera.at());
  glm::vec3 uu = vwr->camera.U();
  glm::vec3 vv = vwr->camera.V();
  switch(vwr->_mode) {
  case viewerModeRotateUV:
    eye = glm::normalize(eye);
    vwr->camera.from(eye);
    break;
  case viewerModeFov:
    break;
  case viewerModeDepth:
    break;
  case viewerModeRotateU:
    break;
  case viewerModeRotateV:
    break;
  case viewerModeRotateN:
    break;
  case viewerModeDolly:
    break;
  case viewerModeTranslateUV:
    break;
  case viewerModeTranslateU:
    break;
  case viewerModeTranslateV:
    break;
  case viewerModeTranslateN:
    break;
  }

  vwr->_lastX = xx;
  vwr->_lastY = yy;
  vwr->cameraUpdate();
  return;
}

Viewer::Viewer(int width, int height, const char *label) {
  static const char me[]="Hale::Viewer::Viewer";

  _button[0] = _button[1] = false;
  _verbose = 1;
  _upFix = false;
  _mode = viewerModeNone;
  _widthScreen = width;
  _heightScreen = height;
  _lastX = _lastY = AIR_NAN;

  // http://www.glfw.org/docs/latest/window.html
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Use OpenGL Core v3.2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  _window = glfwCreateWindow(width, height,
                             label ? label : "Viewer", NULL, NULL);
  if (!_window) {
    fprintf(stderr, "%s: glfwCreateWindow(%d,%d) failed\n",
            me, width, height);
    finishing = true;
  }
  glfwGetFramebufferSize(_window, &_widthBuffer, &_heightBuffer);

  glfwMakeContextCurrent(_window);
  glfwSetWindowUserPointer(_window, static_cast<void*>(this));
  glfwSetCursorPosCallback(_window, cursorPosCB);
  glfwSetMouseButtonCallback(_window, mouseButtonCB);
  // glfwSetWindowSizeCallback(_window, windowSizeCB);
  glfwSetFramebufferSizeCallback(_window, framebufferSizeCB);
  glfwSetKeyCallback(_window, keyCB);
  glfwSetWindowCloseCallback(_window, windowCloseCB);

  shapeUpdate();
}

Viewer::~Viewer() {
  glfwDestroyWindow(_window);
}

int Viewer::verbose() { return _verbose; }
void Viewer::verbose(int vv) { _verbose = vv; }

int Viewer::width() { return _widthScreen; }
int Viewer::height() { return _heightScreen; }

bool Viewer::upFix() { return _upFix; }
void Viewer::upFix(bool upf) { _upFix = upf; }

void Viewer::bufferSwap() { glfwSwapBuffers(_window); }

void Viewer::shapeUpdate() {
  static const char me[]="Hale::Viewer::shapeUpdate";

  _pixDensity = _widthBuffer/_widthScreen;
  camera.aspect(static_cast<double>(_widthBuffer)/_heightBuffer);
  fprintf(stderr, "!%s: density = %d; aspect = %g\n", me, _pixDensity, camera.aspect());
  glViewport(0, 0, _widthBuffer, _heightBuffer);
}

void Viewer::cameraUpdate() {
  // static const char me[]="Hale::Viewer::cameraUpdate";


  /* HEY: update GL transforms for camera */
}

} // namespace Hale
