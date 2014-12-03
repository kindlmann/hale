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

consider removing use of glm::lookat and glm::perspective, in any case:
figure out relationship between basis implied by glm::lookat, and {U,V,N}

with up fix, make it harder to get from-at aligned with up

make something like 90 degrees be upper limit on FOV, to be
enforced with both FOV and Vertigo controls

viewer capture/dump to image

generating hest options

interpreting hest options

*/

#include "Hale.h"
#include "privateHale.h"

/* the fraction of window width along its border that will be treated
   as its margin, to support the different kinds of interactions
   triggered there */
#define MARG 0.18

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
  return;
}

*/

void
Viewer::title() {
  char xd[128], buff[128];

  if (1 != _pixDensity) {
    sprintf(xd, "x%d", _pixDensity);
  } else {
    strcpy(xd, "");
  }
  sprintf(buff, " (%d,%d)%s", _widthScreen, _heightScreen, xd);
  std::string title = _label + buff;
  glfwSetWindowTitle(_window, title.c_str());
}

void
Viewer::framebufferSizeCB(GLFWwindow *gwin, int newWidth, int newHeight) {
  static const char me[]="framebufferSizeCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose() > 1) {
    printf("%s(%d,%d)\n", me, newWidth, newHeight);
  }
  vwr->_widthBuffer = newWidth;
  vwr->_heightBuffer = newHeight;
  glfwGetWindowSize(gwin, &(vwr->_widthScreen), &(vwr->_heightScreen));
  vwr->shapeUpdate();
  vwr->title();
  return;
}

void
Viewer::keyCB(GLFWwindow *gwin, int key, int scancode, int action, int mods) {
  static const char me[]="keyCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose() > 1) {
    printf("%s(%d,%d,%d,%d)\n", me, key, scancode, action, mods);
  }
  if (GLFW_KEY_Q == key && GLFW_PRESS == action && mods) {
    /* this is where we handle modifer+'q' as 'quit' on linux;
       actually *any* modifier will do. */
    finishing = true;
  } else if (GLFW_KEY_U == key && GLFW_PRESS == action) {
    vwr->upFix(!vwr->upFix());
    if (vwr->verbose()) {
      printf("%s: fixed-up is now %s\n", me, vwr->upFix() ? "on" : "off");
    }
  } else if (GLFW_KEY_O == key && GLFW_PRESS == action) {
    vwr->camera.orthographic(!vwr->camera.orthographic());
    if (vwr->verbose()) {
      printf("%s: projection is %s\n", me,
             vwr->camera.orthographic() ? "orthographic" : "perspective");
    }
  } else if (GLFW_KEY_V == key && GLFW_PRESS == action) {
    int vv = vwr->verbose();
    vv += mods ? -1 : 1;
    vv = vv < 0 ? 0 : vv;
    vwr->verbose(vv);
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
Viewer::windowRefreshCB(GLFWwindow *gwin) {
  static const char me[]="windowRefreshCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));

  if (vwr->verbose() > 1) {
    printf("%s()\n", me);
  }
  if (vwr->_refreshCB) {
    vwr->_refreshCB(vwr->_refreshData);
  }

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
    **      |     :                           :     |
    **      |  X Fov/DepthScale               :     |
    **      |     :                           :     |
    **      |     :                           :     |
    **      |. . . . . . . . . . . . . . . . . \    |
    **      |  X  :      X  RotateN/Dolly        \  |
    ** y=1  +--|------------------------------------+
    **         \__ Vertigo/DepthTranslate
    */
    if (xf <= MARG && yf > 1-MARG) {
      vwr->_mode = (vwr->_button[0] ? viewerModeVertigo : viewerModeDepthTranslate);
    } else if (xf <= MARG && xf <= yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeFov : viewerModeDepthScale);
    } else if (yf <= MARG && xf > yf && 1-xf >= yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateV : viewerModeTranslateV);
    } else if (xf > 1-MARG && 1-xf < yf && 1-xf < 1-yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateU : viewerModeTranslateU);
    } else if (yf > 1-MARG && 1-xf >= 1-yf) {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateN : viewerModeDolly);
    } else {
      vwr->_mode = (vwr->_button[0] ? viewerModeRotateUV : viewerModeTranslateUV);
    }
  }
  if (viewerModeNone != vwr->_mode) {
    /* reset the information about last position */
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
  float vsize, ssize, frcX, frcY, rotX, rotY, trnX, trnY;
  glm::vec3 nfrom; // new from; used in various cases

  if (viewerModeNone == vwr->_mode) {
    /* nothing to do here */
    return;
  }
  if (!AIR_EXISTS(vwr->_lastX) || !AIR_EXISTS(vwr->_lastY)) {
    /* this is our first time through there; we don't have a previous
       position recorded, so all we can do is record the position and do
       the real work the next time we're here */
    vwr->_lastX = xx;
    vwr->_lastY = yy;
    return;
  }
  /* else we are moving the camera around */
  if (vwr->verbose() > 2) {
    printf("%s(%g,%g): (%s) hello\n", me, xx, yy,
           airEnumStr(viewerMode, vwr->_mode));
  }
  /* "dangle" is for the modes accessed on the left and bottom sides of the
     window; things for which its nice to be able to make a large change
     (larger than would fit with only vertical or horizontal cursor
     motion) */
  float angle0 = atan2(vwr->_lastY - vwr->height()/2,
                       vwr->_lastX - vwr->width()/2);
  float angle1 = atan2(yy - vwr->height()/2,
                       xx - vwr->width()/2);
  float dangle = angle1 - angle0;
  /* undo the wrapping to [0,2*pi] */
  if (dangle < -AIR_PI) {
    dangle += 2*AIR_PI;
  } else if (dangle > AIR_PI) {
    dangle -= 2*AIR_PI;
  }
  // dangle *= 1.5;
  ssize = sqrt(vwr->width()*vwr->width() + vwr->height()*vwr->height());
  frcX = (vwr->_lastX - xx)/ssize;
  frcY = (vwr->_lastY - yy)/ssize;
  rotX = 4*frcX;
  rotY = 4*frcY;
  glm::vec3 toeye = vwr->camera.from() - vwr->camera.at();
  float elen = glm::length(toeye);
  vsize = elen*tan(vwr->camera.fov()*AIR_PI/360);
  trnX = 4*frcX*vsize;
  trnY = 4*frcY*vsize;
  toeye /= elen;
  glm::vec3 uu = vwr->camera.U();
  glm::vec3 vv = vwr->camera.V();
  glm::vec3 nn = vwr->camera.N();
  switch (vwr->_mode) {
  case viewerModeRotateUV:
  case viewerModeRotateU:
  case viewerModeRotateV:
    switch (vwr->_mode) {
    case viewerModeRotateUV:
      toeye = glm::normalize(toeye + rotX*uu + rotY*vv);
      nfrom = vwr->camera.at() + elen*toeye;
      break;
    case viewerModeRotateU:
      toeye = glm::normalize(toeye + rotY*vv);
      nfrom = vwr->camera.at() + elen*toeye;
      break;
    case viewerModeRotateV:
      if (!vwr->_upFix) {
        toeye = glm::normalize(toeye + rotX*uu);
        nfrom = vwr->camera.at() + elen*toeye;
      } else {
        /* in this one case, we move along cylinders wrapped around up,
           instead of along spheres.  We have to decompose the vector
           to the eye into parts "cpar" parallel with up, and
           "cort" orthogonal to up, and only update cpar */
        glm::vec3 up = vwr->camera.up();
        glm::vec3 diff = vwr->camera.from() - vwr->camera.at();
        glm::vec3 cpar = up*glm::dot(diff, up);
        glm::vec3 cort = diff - cpar;
        /*
        fprintf(stderr, "%s: |diff| = %g; |cpar| = %g; |cort| = %g (|up|=%g)\n", me,
                glm::length(diff), glm::length(cpar), glm::length(cort),
                glm::length(up));
        */
        float rad = glm::length(cort);
        cort /= rad;
        cort = glm::normalize(cort + rotX*uu);
        nfrom = vwr->camera.at() + rad*cort + cpar;
      }
      break;
    }
    vwr->camera.from(nfrom);
    if (!vwr->_upFix) {
      vwr->camera.reup();
    }
    break;
  case viewerModeRotateN:
    if (!vwr->_upFix) {
      vwr->camera.up(glm::normalize(vwr->camera.up() - dangle*uu));
    }
    break;
  case viewerModeDolly:
    vwr->camera.from(vwr->camera.from() - dangle*nn);
    /* to be consistent with Translate{U,V,UV}, we move both
       "from" and "at" here, though sometimes it is nice to be
       able to move only "from", leaving "at" as is */
    vwr->camera.at(vwr->camera.at() - dangle*nn);
    break;
  case viewerModeFov:
    {
      /* the idea is that we want to make it hard to reach FOVs of 0 and
         180.  So we treat those limits as the -pi/2,pi/2 limits of atan(),
         and manipulate the FOV in the "tangent" space (ha ha, bad pun) */
      double ff = vwr->camera.fov();
      ff = AIR_AFFINE(0, ff, 180, -AIR_PI/2, AIR_PI/2);
      ff = tan(ff);
      // scaling here for qualitative effect, not math correctness
      ff -= 0.9f*dangle;
      ff = atan(ff);
      ff = AIR_AFFINE(-AIR_PI/2, ff, AIR_PI/2, 0, 180);
      vwr->camera.fov(ff);
    }
    break;
  case viewerModeDepthScale:
  case viewerModeDepthTranslate:
    {
      double lognear = log((elen + vwr->camera.clipNear())/elen);
      double logfar = log((elen + vwr->camera.clipFar())/elen);
      if (viewerModeDepthScale == vwr->_mode) {
        double logscl = pow(2, dangle);
        lognear *= logscl;
        logfar *= logscl;
      } else {
        lognear += 0.09*dangle;
        logfar += 0.09*dangle;
      }
      vwr->camera.clipNear(exp(lognear)*elen - elen);
      vwr->camera.clipFar(exp(logfar)*elen - elen);
    }
    break;
  case viewerModeVertigo:
    if (vwr->camera.orthographic()) {
      if (vwr->verbose()) {
        fprintf(stderr, "%s: (no effect from %s w/ orthographic projection)\n",
                me, airEnumStr(viewerMode, viewerModeVertigo));
      }
    } else {
      glm::vec3 teye = vwr->camera.from() - vwr->camera.at();
      float odist = glm::length(teye);
      teye /= odist;
      float ndist = exp(log(odist) - 0.9f*dangle);
      vwr->camera.from(ndist*teye + vwr->camera.at());
      vwr->camera.fov(atan(vsize/ndist)*360/AIR_PI);
    }
    break;
  case viewerModeTranslateUV:
    vwr->camera.from(vwr->camera.from() + trnX*uu + trnY*vv);
    vwr->camera.at(vwr->camera.at() + trnX*uu + trnY*vv);
    break;
  case viewerModeTranslateU:
    vwr->camera.from(vwr->camera.from() + trnY*vv);
    vwr->camera.at(vwr->camera.at() + trnY*vv);
    break;
  case viewerModeTranslateV:
    vwr->camera.from(vwr->camera.from() + trnX*uu);
    vwr->camera.at(vwr->camera.at() + trnX*uu);
    break;
  }

  vwr->_lastX = xx;
  vwr->_lastY = yy;
  return;
}

/* constructor */
Viewer::Viewer(int width, int height, const char *label) {
  static const char me[]="Hale::Viewer::Viewer";

  _button[0] = _button[1] = false;
  _verbose = 0;
  _upFix = false;
  _mode = viewerModeNone;
  _refreshCB = NULL;
  _refreshData = NULL;
  _widthScreen = width;
  _heightScreen = height;
  _lastX = _lastY = AIR_NAN;

  // http://www.glfw.org/docs/latest/window.html
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Use OpenGL Core v3.2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  /* HEY HEY should look into using this!
     glfwWindowHint(GLFW_SAMPLES, 0); */
  _label = label ? label : "Viewer";
  _window = glfwCreateWindow(width, height, _label.c_str(), NULL, NULL);
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
  glfwSetWindowRefreshCallback(_window, windowRefreshCB);

  shapeUpdate();
  title();
}

Viewer::~Viewer() {
  glfwDestroyWindow(_window);
}

int Viewer::verbose() { return _verbose; }
void Viewer::verbose(int vv) {
  static const char me[]="Viewer::verbose";
  if (_verbose && !vv) {
    fprintf(stderr, "%s: now 0 (silent)\n", me);
  }
  _verbose = vv;
  if (vv) {
    fprintf(stderr, "%s: now %d\n", me, vv);
  }
}

int Viewer::width() { return _widthScreen; }
int Viewer::height() { return _heightScreen; }

bool Viewer::upFix() { return _upFix; }
void Viewer::upFix(bool upf) {
  _upFix = upf;
  camera.reup();
}

void Viewer::refreshCB(ViewerRefresher cb) { _refreshCB = cb; }
ViewerRefresher Viewer::refreshCB() { return _refreshCB; }
void Viewer::refreshData(void *data) { _refreshData = data; }
void *Viewer::refreshData() { return _refreshData; }

void Viewer::bufferSwap() { glfwSwapBuffers(_window); }

/* HEY HEY finish this
int Viewer::bufferSave(Nrrd *nrgba, Nrrd *ndepth) {
  static const char me[]="Hale::Viewer::bufferSave";

  if (!nrgba && !ndepth) { // huh; no output requested?
    return 0;
  }

  return 0;
}
*/

void Viewer::shapeUpdate() {
  // static const char me[]="Hale::Viewer::shapeUpdate";

  _pixDensity = _widthBuffer/_widthScreen;
  camera.aspect(static_cast<double>(_widthBuffer)/_heightBuffer);
  glViewport(0, 0, _widthBuffer, _heightBuffer);
}

} // namespace Hale
