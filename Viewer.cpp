/*
  hale: support for minimalist scientific visualization
  Copyright (C) 2014, 2015  University of Chicago

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

#include <GL/glew.h>
#include "Hale.h"
#include "GUI.h"
#include "privateHale.h"

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/tabwidget.h>

#include <nanogui/window.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/screen.h>
#include <nanogui/layout.h>
#include <nanogui/serializer/core.h>
#include <nanogui/formhelper.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/glutil.h>
#include <iostream>
#include <map>

/* the fraction of window width along its border that will be treated
   as its margin, to support the different kinds of interactions
   triggered there */
#define MARG 0.18

namespace Hale {

/* The idea is that FOV itself is not really a quantity that makes sense
   to vary in a uniform way, because we should not be easily able to get
   to fovMin and fovMax. So we "unwarp" fov so that fovMax and fovMin
   are where they should be, at +/- infinity. */
static double fovUnwarp(double fov) {
  double fff;
  fff = AIR_AFFINE(fovMin, fov, fovMax, -AIR_PI/2, AIR_PI/2);
  fff = tan(fff);
  return fff;
}
static double fovWarp(double fff) {
  double fov;
  fff = atan(fff);
  fov = AIR_AFFINE(-AIR_PI/2, fff, AIR_PI/2, fovMin, fovMax);
  return fov;
}


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

int
Viewer::mode() const {
  return _mode;
}

void
Viewer::_buffAlloc(void) {
  static const char me[]="Hale::Viewer::_buffAlloc";
  for (unsigned int ii=0; ii<=1; ii++) {
    if (nrrdMaybeAlloc_va(_nbuffRGBA[ii], nrrdTypeUChar, 3, (size_t)4,
                          (size_t)_widthBuffer, (size_t)_heightBuffer)) {
      char *err = biffGetDone(NRRD);
      fprintf(stderr, "%s: error allocating %u x %u RGBA buffer:\n%s", me,
              _widthBuffer, _heightBuffer, err);
      free(err);
    }
    _buffRGBA[ii] = (unsigned char *)(_nbuffRGBA[ii]->data);
  }
  return;
}

GLFWwindow *Viewer::getWindow(){
  return _window;
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
  vwr->_buffAlloc();
  glfwGetWindowSize(gwin, &(vwr->_widthScreen), &(vwr->_heightScreen));
  vwr->shapeUpdate();
  vwr->title();
  return;
}


#define V2S(vv)                                 \
  (sprintf(buff[0], "%g,", vv[0]),               \
   sprintf(buff[1], "%g,", vv[1]),               \
   sprintf(buff[2], "%g", vv[2]),                                       \
   std::string(buff[0]) + " " + std::string(buff[1]) + " " + std::string(buff[2]))

std::string
Viewer::origRowCol(void) {
  char buff[3][32];
  glm::vec3 diff = camera.at() - camera.from();
  float dist = glm::length(diff);
  float vspNear = dist + camera.clipNear();
  glm::vec3 icent = camera.from() + vspNear*glm::normalize(diff);
  float hght = 2*vspNear * tan(camera.fov()*AIR_PI/360);
  float wdth = camera.aspect()*hght;
  float uspc = wdth/(this->width() - 1);
  float vspc = hght/(this->height() - 1);
  glm::vec3 rvec = -vspc*camera.V();
  glm::vec3 cvec = uspc*camera.U();
  glm::vec3 orig = icent - wdth*camera.U()/2.0f + hght*camera.V()/2.0f;
  orig += rvec/2.0f + cvec/2.0f;
  std::string ret = ("vec3 eye = [" + V2S(camera.from()) + "];\n" +
                     "vec3 orig= [" + V2S(orig) + "];\n" +
                     "vec3 rVec= [" + V2S(rvec) + "];\n" +
                     "vec3 cVec= [" + V2S(cvec) + "];\n");
  return ret;
}
void Viewer::setFocused(GLFWwindow* gwin, bool focus){
  fprintf(stderr,"\n * focus: %s\n", focus?"true":"false");
  if(!focus){
    Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));
    vwr->_mode = viewerModeNone;
  }
}
void
Viewer::keyCB(GLFWwindow *gwin, int key, int scancode, int action, int mods) {
  static const char me[]="keyCB";
  // cegui_keyCallback(gwin,key,scancode,action,mods);
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));
  vwr->verbose(2);
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
  } else if (GLFW_KEY_C == key && GLFW_PRESS == action) {
    std::string chest = vwr->camera.hest();
    chest += (" -sz " + std::to_string(vwr->width())
              + " " + std::to_string(vwr->height()));
    printf("\n%s\n", chest.c_str());
    chest = vwr->origRowCol();
    printf("\n%s\n", chest.c_str());
  } else if (GLFW_KEY_H == key && GLFW_PRESS == action) {
    vwr->helpPrint(stdout);
  } else if (GLFW_KEY_S == key && GLFW_PRESS == action) {
    vwr->snap();
  } else if (GLFW_KEY_R == key && GLFW_PRESS == action) {
    glm::vec3 wmin, wmax, wmed;
    vwr->_scene->bounds(wmin, wmax);
    wmed = (wmin + wmax)/2.0f;
    vwr->camera.at(wmed);
    vwr->camera.up(glm::vec3(0.0f,0.0f,1.0f));
    float diff = glm::length(wmax - wmin);
    float dist = (diff/2)/tanf((fovBest/2)*M_PI/180);
    vwr->camera.from(vwr->camera.at() - glm::vec3(dist, 0.0f, 0.0f));
    vwr->camera.clipNear(-diff/2);
    vwr->camera.clipFar(diff/2);
    // leave aspect and orthographic as is
  } else if (GLFW_KEY_V == key && GLFW_PRESS == action) {
    int vv = vwr->verbose();
    vv += mods ? -1 : 1;
    vv = vv < 0 ? 0 : vv;
    vwr->verbose(vv);
  } else if (GLFW_KEY_T == key && GLFW_PRESS == action) {
    if (vwr->_slidable) {
      vwr->_sliding = !(vwr->_sliding);
      printf("%s: bottom-edge slider is now %s\n", me,
             vwr->_sliding ? "on" : "off");
    }
  } else if (GLFW_KEY_SPACE  == key && GLFW_PRESS == action) {
    if (vwr->_tvalue) {
      *(vwr->_tvalue) = !(*(vwr->_tvalue));
      printf("%s: toggle is now %d\n", me, *(vwr->_tvalue));
    }
  }

  return;
}

void Viewer::helpPrint(FILE *file) const {
  fprintf(file, "\n");
  fprintf(file, "Clicking and dragging in different parts of the window does different\n");
  fprintf(file, "things with the camera. In diagram below, \"foo/bar\" means\n");
  fprintf(file, "foo happens with (left)-click, and bar happens with \"right click\"\n");
  fprintf(file, "where \"right-click\" is with any modifier (Shift,Command,Cntl,Opt)\n");
  fprintf(file, "  +---------------------------------------+\n");
  fprintf(file, "  | \\          O--RotateV/TranslateV    / |\n");
  fprintf(file, "  |   \\  . . . . . . . . . . . . . .  /   |\n");
  fprintf(file, "  |     :                           :     |\n");
  fprintf(file, "  |  O--Zoom/DepthScale             :     |\n");
  fprintf(file, "  |     :                           :     |\n");
  fprintf(file, "  |     :   V                       :  O--RotateU/\n");
  fprintf(file, "  |     :   ^      O--RotateUV/     :     TranslateU\n");
  fprintf(file, "  |     :   |         TranslateUV   :     |\n");
  fprintf(file, "  |     :   |                       :     |\n");
  fprintf(file, "  |     :   |                       :     |\n");
  fprintf(file, "  |     :   |                       :     |\n");
  fprintf(file, "  |     :  (N)------> U             :     |\n");
  fprintf(file, "  |. . . . . . . . . . . . . . . . . \\    |\n");
  fprintf(file, "  |  O  :  O--RotateN/TranslateN       \\  |\n");
  fprintf(file, "  +--|------------------------------------+\n");
  fprintf(file, "      \\__ Vertigo/FOV\n");
  fprintf(file, "\n");
  fprintf(file, " - Zoom from look-from and rescales clip distances\n");
  fprintf(file, " - DepthScale scales clip distances to,away from zero\n");
  fprintf(file, " - Vertigo changes amount of perspective distortion\n");
  fprintf(file, " - FOV just changes field-of-view\n");
  fprintf(file, "\n");
  fprintf(file, "What different keys do:\n");
  fprintf(file, "h: print this usage info\n");
  fprintf(file, "c: print command-line camera specification\n");
  fprintf(file, "s: save viewer image to snap-NNNN.png\n");
  fprintf(file, "o: toggle between orthographic, perspective\n");
  fprintf(file, "Q or shift-q or command-q or cntl-q: quit\n");
  fprintf(file, "r: reset camera to make everything visible\n");
  fprintf(file, "u: fix up vector\n");
  fprintf(file, "v,V: for debugging: increase,decrease verbosity\n");
}
  
void Viewer::slider(double *slvalue, double min, double max) {
  if (slvalue) {
    _slvalue = slvalue;
    _slmin = min;
    _slmax = max;
    _slidable = true;
  } else {
    _slvalue = NULL;
    _slidable = false;
  }
  return;
}

bool Viewer::slidable() const {
  return _slidable;
}

bool Viewer::sliding() const {
  return _sliding;
}
void Viewer::sliding(bool sld) {
  _sliding = sld;
}

void Viewer::toggle(int *tvalue) {
  _tvalue = tvalue;
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
  nanogui::leave();

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

static int
modemap(bool butt0, double xf, double yf) {
  int ret;
  if (xf <= MARG && yf > 1-MARG) {
    ret = (butt0 ? viewerModeVertigo : viewerModeFov);
  } else if (xf <= MARG && xf <= yf) {
    ret = (butt0 ? viewerModeZoom : viewerModeDepthScale);
  } else if (yf <= MARG && xf > yf && 1-xf >= yf) {
    ret = (butt0 ? viewerModeRotateV : viewerModeTranslateV);
  } else if (xf > 1-MARG && 1-xf < yf && 1-xf < 1-yf) {
    ret = (butt0 ? viewerModeRotateU : viewerModeTranslateU);
  } else if (yf > 1-MARG && 1-xf >= 1-yf) {
    ret = (butt0 ? viewerModeRotateN : viewerModeTranslateN);
  } else {
    ret = (butt0 ? viewerModeRotateUV : viewerModeTranslateUV);
  }
  return ret;
}

void
Viewer::_slrevalue(const char *me, double xx) {
  *(_slvalue) = AIR_AFFINE(0, xx, width(), _slmin, _slmax);
  if (_verbose > 1) {
    printf("%s(%g): slvalue = %g\n", me, xx, *(_slvalue));
  }
}

void
Viewer::mouseButtonCB(GLFWwindow *gwin, int button, int action, int mods) {
  static const char me[]="mouseButtonCB";
  // if(HaleGUI::gui_mouseButtonCallback(gwin,button,action,mods))return;
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));
  vwr->verbose(1);
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
  xf = (xpos-vwr->viewportX)/vwr->viewportW;
  yf = (ypos-vwr->viewportY)/vwr->viewportH;
  fprintf(stderr,"Xf, Yf: %.3f,%.3f\n", xf, yf);
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
    **      |  X Zoom/DepthScale              :     |
    **      |     :                           :     |
    **      |     :    V                      :  X RotateU/
    **      |     :   ^     X RotateUV/       :    TranslateU
    **      |     :   |       TranslateUV     :     |
    **      |     :   |                       :     |
    **      |     :   |                       :     |
    **      |     :   |                       :     |
    **      |     :   o-------> U             :     |
    **      |. . . . . . . . . . . . . . . . . \    |
    **      |  X  :  X  RotateN/TranslateN       \  |
    ** y=1  +--|------------------------------------+
    **         \__ Vertigo/Fov
    */
    if (vwr->_slidable && vwr->_sliding) {
      if (yf > 1-MARG/2) {
        vwr->_mode = viewerModeSlider;
        vwr->_slrevalue(me, xpos);
      } else {
        /* yf <= 1-Marg */
        vwr->_mode = modemap(vwr->_button[0], xf, yf/(1-MARG/2));
      }
    } else {
      vwr->_mode = modemap(vwr->_button[0], xf, yf);
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

  // bool handled = (HaleGUI::gui_cursorPosCallback(gwin, xx, yy));
  // if(handled)return;
  static const char me[]="cursorPosCB";
  Viewer *vwr = static_cast<Viewer*>(glfwGetWindowUserPointer(gwin));
  float vsize, ssize, frcX, frcY, rotX, rotY, trnX, trnY;
  glm::vec3 nfrom; // new from; used in various cases
  double fff; // tmp var

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
  toeye /= elen;
  vsize = elen*tan(vwr->camera.fov()*AIR_PI/360);
  trnX = 4*frcX*vsize;
  trnY = 4*frcY*vsize;
  glm::vec3 uu = vwr->camera.U();
  glm::vec3 vv = vwr->camera.V();
  // glm::vec3 nn = vwr->camera.N();
  switch (vwr->_mode) {
  case viewerModeRotateUV:
  case viewerModeRotateU:
  case viewerModeRotateV:
    switch (vwr->_mode) {
    case viewerModeRotateUV:
      nfrom = vwr->camera.at() + elen*glm::normalize(toeye + rotX*uu - rotY*vv);
      fprintf(stderr, "(UV)");
      break;
    case viewerModeRotateU:
      nfrom = vwr->camera.at() + elen*glm::normalize(toeye - rotY*vv);
      fprintf(stderr, "(UU)");
      break;
    case viewerModeRotateV:
      fprintf(stderr, "(VV)");
      if (!vwr->_upFix) {
        nfrom = vwr->camera.at() + elen*glm::normalize(toeye + rotX*uu);
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
  case viewerModeTranslateN:
    vwr->camera.at(vwr->camera.at() + 0.25f*dangle*vsize*toeye);
    vwr->camera.from(vwr->camera.from() + 0.25f*dangle*vsize*toeye);
    break;
  case viewerModeZoom:
    {
      float rescale = exp(-0.3*dangle);
      vwr->camera.clipNear(rescale*vwr->camera.clipNear());
      vwr->camera.clipFar(rescale*vwr->camera.clipFar());
      vwr->camera.from(vwr->camera.at() + rescale*elen*toeye);
    }
    break;
  case viewerModeFov:
    fff = fovUnwarp(vwr->camera.fov());
    // scaling here for qualitative effect, not math correctness
    vwr->camera.fov(fovWarp(fff - 0.9*dangle));
    break;
  case viewerModeDepthScale:
    {
      double lognear = log((elen + vwr->camera.clipNear())/elen);
      double logfar = log((elen + vwr->camera.clipFar())/elen);
      double logscl = pow(2, dangle);
      lognear *= logscl;
      logfar *= logscl;
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
      double oldFov = vwr->camera.fov();
      fff = fovUnwarp(oldFov);
      double newfov = fovWarp(fff + 0.9*dangle);
      vwr->camera.fov(newfov);
      float newelen = vsize/tan(newfov*AIR_PI/360);
      if (newelen > -vwr->camera.clipNear()) {
        vwr->camera.from(vwr->camera.at() + newelen*toeye);
      } else {
        /* else new from location would be inside near clipping
           plane, which will confuse projection transform */
        if (vwr->verbose()) {
          fprintf(stderr, "%s: (preventing look-from being inside near clip)\n", me);
        }
        vwr->camera.fov(oldFov);
      }
    }
    break;
  case viewerModeTranslateUV:
    vwr->camera.from(vwr->camera.from() + trnX*uu - trnY*vv);
    vwr->camera.at(vwr->camera.at() + trnX*uu - trnY*vv);
    break;
  case viewerModeTranslateU:
    vwr->camera.from(vwr->camera.from() - trnY*vv);
    vwr->camera.at(vwr->camera.at() - trnY*vv);
    break;
  case viewerModeTranslateV:
    vwr->camera.from(vwr->camera.from() + trnX*uu);
    vwr->camera.at(vwr->camera.at() + trnX*uu);
    break;
  case viewerModeSlider:
    vwr->_slrevalue(me, xx);
    break;
  }

  vwr->_lastX = xx;
  vwr->_lastY = yy;
  return;
}

/* constructor */
Viewer::Viewer(int width, int height, const char *label, Scene *scene) : nanogui::Screen(Eigen::Vector2i(width, height), "Hale with NanoGUI"){
  static const char me[]="Hale::Viewer::Viewer";

  _lightDir = glm::normalize(glm::vec3(-1.0f, 1.0f, 3.0f));
  _scene = scene;
  _button[0] = _button[1] = false;
  _verbose = 0;
  _upFix = false;
  _mode = viewerModeNone;
  _refreshCB = NULL;
  _refreshData = NULL;
  _widthScreen = width;
  _heightScreen = height;
  _lastX = _lastY = AIR_NAN;
  _slvalue = NULL;
  _tvalue = NULL;
  _slmin = _slmax = AIR_NAN;
  _slidable = false;
  _sliding = false;

  _updateFunc = 0;



  // perform layout

  // http://www.glfw.org/docs/latest/window.html
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Use OpenGL Core v3.2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  /* look into using this!
     glfwWindowHint(GLFW_SAMPLES, 0); */
  _label = label ? label : "Viewer";
  
  _window = glfwGetCurrentContext();
  if(!_window){
    _window = glfwCreateWindow(width, height, _label.c_str(), NULL, NULL);
  }
  if (!_window) {
    fprintf(stderr, "%s: glfwCreateWindow(%d,%d) failed\n",
            me, width, height);
    finishing = true;
  }

  setBackground(nanogui::Vector3f(mTheme->mWindowFillUnfocused[0],mTheme->mWindowFillUnfocused[1],mTheme->mWindowFillUnfocused[2]));

  glfwGetFramebufferSize(_window, &_widthBuffer, &_heightBuffer);
  glfwSetWindowCloseCallback(mGLFWWindow, windowCloseCB);

  _nbuffRGBA[0] = nrrdNew();
  _nbuffRGBA[1] = nrrdNew();
  _buffAlloc();
  glfwSetWindowUserPointer(_window, static_cast<void*>(this));
  shapeUpdate();
  title();
  printf("\nType 'r' to reset view, 'h' for help using keyboard and viewer\n");


}

bool Viewer::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers){
    int fpathsize = mFocusPath.size();
    if(!Screen::mouseMotionEvent(p,rel,button,modifiers) && fpathsize <= 1){
        // pass focus to hale application.
        // setFocused(getWindow(), true);
        cursorPosCB(_window, p[0], p[1]);
    }
    return true;
}

bool Viewer::resizeEvent(const nanogui::Vector2i &s) {
    fprintf(stderr,"resized");
    framebufferSizeCB(_window, s[0],s[1]);
    updateViewportSize();
    return Screen::resizeEvent(s);
}

struct WindowProperties{
  int sticky; // 0 if not sticky, 1,2,3,4 for top,left,bottom,right edge.
  nanogui::Theme *unstickyTheme;
  nanogui::Vector2i realSize;
  nanogui::Vector2i realAnchorPos;
  bool floatingAnchorPos;
};

nanogui::Theme *stickyTheme = 0;

std::map<nanogui::Widget*, WindowProperties> windowmgr;


bool Viewer::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers){
    // within the nanogui framework, only mouse clicks are able to
    // change the focus of a window.
    fprintf(stderr,"mouse button");
    int fpathsize = mFocusPath.size();
    fprintf(stderr,"path size: %d\n",fpathsize);
    if(!Screen::mouseButtonEvent(p,button,down,modifiers) && fpathsize <= 1){
        // the underlying (hale) application has focus.
        setFocused(_window, true);
        mouseButtonCB(_window, button, down?GLFW_PRESS:GLFW_RELEASE, modifiers);
    }
    else{
        // lost focus.
        setFocused(_window, false);
        if(!stickyTheme){
          stickyTheme = new nanogui::Theme(*mTheme);
          stickyTheme->mDropShadow = nanogui::Color(0,0,0,0);
          stickyTheme->mWindowFillUnfocused = nanogui::Color(43, 230);
          stickyTheme->mWindowFillFocused =   nanogui::Color(55, 230);
        }

        bool stickychanged[] = {false,false,false,false,false};
        int  panelSizes[] = {0,0,0,0,0};

        // resize viewport around sticky windows on sides of screen.

        fprintf(stderr,"iterating\n");
        for(Widget* w : mChildren){
          // if(!w->visible())continue;
          nanogui::Popup* popup = dynamic_cast<nanogui::Popup*>(w);
          std::map<nanogui::Widget*, WindowProperties>::iterator found = windowmgr.find(w);
          if(found != windowmgr.end()){
            if (!down){
              // do window-managerial things
              nanogui::Vector2i pos = w->absolutePosition();
              nanogui::Vector2i size = w->size();
              WindowProperties* props = &(*found).second;

              if(popup != 0){
                
                // this is a popup.
                fprintf(stderr, "anchor: %d, %d\n", popup->anchorPos()[0], popup->anchorPos()[1]);
                int w = popup->size()[0];
                int h = popup->size()[1];
                int absx = popup->absolutePosition()[0];
                int absy = popup->absolutePosition()[1];
                if(!props->floatingAnchorPos){
                  if(absx+w > mSize[0] || absy+h > mSize[1]){
                    props->realAnchorPos = popup->anchorPos();
                    props->floatingAnchorPos = true;
                  }
                }
                if(props->floatingAnchorPos){
                  int corrx = (absx + w - mSize[0]);
                  int corry = (absy + h - mSize[1]);
                  if(corrx > 0){
                    popup->setAnchorPos(nanogui::Vector2i(popup->anchorPos()[0] - (absx+w-mSize[0]), popup->anchorPos()[1])); 
                  }
                  else if(corrx < 0){
                    if(-corrx > props->realAnchorPos[0] - popup->anchorPos()[0]){
                      fprintf(stderr,"anchoring x \n");
                      popup->setAnchorPos(nanogui::Vector2i(props->realAnchorPos[0], popup->anchorPos()[1]));
                    }else{
                      popup->setAnchorPos(nanogui::Vector2i(popup->anchorPos()[0] - (absx+w-mSize[0]), popup->anchorPos()[1]));
                    }
                  }
                  if(corry > 0){
                    popup->setAnchorPos(nanogui::Vector2i(popup->anchorPos()[0], popup->anchorPos()[1] - (absy+h-mSize[1]))); 
                  }
                  else if(corry < 0){
                    if(-corry > props->realAnchorPos[1] - popup->anchorPos()[1]){
                      fprintf(stderr,"anchoring y\n");
                      popup->setAnchorPos(nanogui::Vector2i(popup->anchorPos()[0], props->realAnchorPos[1]));
                    }else{
                      popup->setAnchorPos(nanogui::Vector2i(popup->anchorPos()[0], popup->anchorPos()[1] - (absy+h-mSize[1])));
                    }
                  }
                  if(popup->anchorPos() == props->realAnchorPos){
                    props->floatingAnchorPos = false;
                  }
                }
                
              }
              else{
                // mouse release. set windows to be sticky if not already.
                fprintf(stderr,"stickying\n");
                if(pos[0] < 30){
                  pos[0] = 0;
                  if(props->sticky != 2){
                    // make sticky window on left of screen.
                    stickychanged[2] = true;
                    stickychanged[props->sticky] = true;
                    props->sticky = 2;
                    w->setTheme(stickyTheme);
                    w->theme()->incRef();
                  }
                  if(panelSizes[2] < size[0])panelSizes[2] = size[0];
                }
                else if(mSize[0] - pos[0] - size[0] < 30){
                  pos[0] = mSize[0] - size[0];
                  if(props->sticky != 4){
                    // make sticky window on right of screen.
                    stickychanged[4] = true;
                    stickychanged[props->sticky] = true;
                    props->sticky = 4;
                    w->setTheme(stickyTheme);
                    w->theme()->incRef();
                  }
                  if(panelSizes[4] < size[0])panelSizes[4] = size[0];
                }
                else{
                  if(props->sticky){
                    // make a sticky window unsticky.
                    fprintf(stderr,"making unsticky\n");
                    stickychanged[props->sticky] = true;
                    fprintf(stderr,"1\n");
                    props->sticky = 0;
                    fprintf(stderr," 2\n");
                    if(props->unstickyTheme)w->setTheme(props->unstickyTheme);
                    fprintf(stderr,"  3\n");
                    w->setSize(props->realSize);
                    fprintf(stderr,"   4\n");
                  }
                }
              }
              fprintf(stderr,"set sticky %d\n", props->sticky);
            }
          }else{
            // add widget to window manager if not already present.
            // if(popup)windowmgr.insert(std::pair<nanogui::Widget*, WindowProperties>(w, {0,w->theme(), nanogui::Vector2i(w->size()), nanogui::Vector2i(popup->anchorPos()), false}));
            /*else */windowmgr.insert(std::pair<nanogui::Widget*, WindowProperties>(w, {0,w->theme(), nanogui::Vector2i(w->size()), nanogui::Vector2i(0,0), false}));
          }
        }
        if(!down){
          fprintf(stderr,"{%d,%d,%d,%d,%d}\n", panelSizes[0],panelSizes[1],panelSizes[2],panelSizes[3],panelSizes[4]);
          for(auto elt : windowmgr){
            Widget* w = elt.first;
            WindowProperties *props = &elt.second;
            if(w->visible()){
              // make window sizes in panels uniform.
              if(props->sticky == 2 || props->sticky==4)w->setSize(nanogui::Vector2i(panelSizes[props->sticky], w->size()[1]));
              if(props->sticky == 1 || props->sticky==3)w->setSize(nanogui::Vector2i(w->size()[0], panelSizes[props->sticky]));
            }
          }
          updateViewportSize();
        }

    }
    fprintf(stderr,"mouse click\n");
    return true;
}

void Viewer::updateViewportSize(){

  // bounds of viewport in pixels from edges of screen.
  int vtop=0, vleft=0, vbot=0, vright=0;
  int lpft=0, rpft=0;   // current left pane vertical position (from top) for sticky windows.
  for(auto elt : windowmgr){
    Widget* w = elt.first;
    WindowProperties *props = &elt.second;
    if(w->visible()){
      // bounds of viewport from edges of screen.
      int vt1=0, vl1=0, vb1=0, vr1=0;
       // which edges the window hugs.
      bool at=false,al=false,ab=false,ar=false;
      // wehther this window fits within the space on
      // a particular edge allotted for another.
      bool shadowed = false;
      nanogui::Vector2i pos = w->absolutePosition();
      nanogui::Vector2i size = w->size();
      
      // make sure windows are on the screen.
      if (pos[0] + size[0] >mSize[0])pos[0] = mSize[0] - size[0];
      if (pos[1] + size[1] >mSize[1])pos[1] = mSize[1] - size[1];
      if (pos[0] < 0)pos[0] = 0;
      if (pos[1] < 0)pos[1] = 0;

      // now, position windows that are on sticky edges.
      if(props->sticky == 2){
        if(size[0] > vleft){
          vl1 = size[0];
          al = true;
        }else{
          shadowed = true;
        }
        pos[0] = 0;
        pos[1] = lpft;
        lpft += size[1];
      }
      if(props->sticky == 4){
        // pos[0] = mSize[0] - pos[0] - size[0];
        // sticky window on right of screen.
        if(size[0] > vright){
          vr1 = size[0];
          ar = true;
        }else{
          shadowed = true;
        }
        pos[0] = mSize[0] - size[0];
        pos[1] = rpft;
        rpft += size[1];
      }

      w->setSize(size);
      w->setPosition(pos);
      
      // skip the math if there are no sticky edges.
      if(!shadowed && (at || al || ab || ar)){
        // change the one viewport dimension which results in the smallest change in area.
        int viewportDimChange = 0;    // encode which dimension is being adjusted.
        int areachange = mSize[0] * mSize[1];
        int areat = areachange, areab = areachange, areal = areachange, arear = areachange;

        // now, figure out which dimension change leads to the greatest viewport area.
        if(at){
          areat = (vt1-vtop)*(mSize[0] - vleft - vright);
          areachange = areat;
        }
        if(ab){
          areab = (vb1-vbot)*(mSize[0] - vleft - vright);
          if(areab < areat){
            areachange = areab;
            viewportDimChange = 1;
          }
        }
        if(al){
          areal = (vl1-vleft)*(mSize[1] - vtop - vbot);
          if(areal < areachange){
            areachange = areal;
            viewportDimChange = 2;
          }
        }
        if(ar){
          arear = (vr1-vright)*(mSize[1] - vtop - vbot);
          if(arear < areachange){
            areachange = arear;
            viewportDimChange = 3;
          }
        }

        // then, make the necessary viewport adjustment.
        if(viewportDimChange == 0)vtop = vt1;
        else if(viewportDimChange == 1)vbot = vb1;
        else if(viewportDimChange == 2)vleft = vl1;
        else if(viewportDimChange == 3)vright = vr1;
      }

    }
  }

  viewportX = vleft;
  viewportY = vbot;
  viewportW = width() - vleft - vright;
  viewportH = height() - vtop - vbot;
  if(viewportW < 1)viewportW = 1;
  if(viewportH < 1)viewportH = 1;
  fprintf(stderr, "viewport: %d, %d, %d, %d\n", viewportX, viewportY, viewportW, viewportH);
}
bool Viewer::keyboardEvent(int key, int scancode, int action, int modifiers) {
    fprintf(stderr,"keyboard");
    int fpathsize = mFocusPath.size();
    if (!Screen::keyboardEvent(key, scancode, action, modifiers) && fpathsize <= 1){
        // pass events to hale application.
        setFocused(_window, true);
        keyCB(_window, key, scancode, action, modifiers);
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }
    return true;
}

void Viewer::draw(NVGcontext *ctx) {
    /* Animate the scrollbar */

    /* Draw the user interface */
    Screen::draw(ctx);

    drawContents();
}

void Viewer::setUpdateFunction(void (*func)()){
  if(func)_updateFunc = func;
}
void Viewer::drawContents() {
  glfwMakeContextCurrent(mGLFWWindow);
  if(_updateFunc){
    _updateFunc();
  }

  // the last program that was used for rendering.
  static int prog_last = 0;

  sleep(0.1);
  glEnable(GL_DEPTH_TEST);
  if(prog_last)glUseProgram(prog_last);
  draw();
  glGetIntegerv(GL_CURRENT_PROGRAM,&prog_last);
}

Viewer::~Viewer() {
  //static const char me[]="Viewer::~Viewer";
  nrrdNuke(_nbuffRGBA[0]);
  nrrdNuke(_nbuffRGBA[1]);
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

void Viewer::lightDir(glm::vec3 dir) { _lightDir = glm::normalize(dir); }
glm::vec3 Viewer::lightDir() const { return _lightDir; }

void Viewer::refreshCB(ViewerRefresher cb) { _refreshCB = cb; }
ViewerRefresher Viewer::refreshCB() { return _refreshCB; }
void Viewer::refreshData(void *data) { _refreshData = data; }
void *Viewer::refreshData() { return _refreshData; }

void Viewer::bufferSwap() {
  glfwSwapBuffers(_window);
  if (debugging)
    printf("## glfwSwapBuffers();\n");
}

void Viewer::current() {
  // glfwMakeContextCurrent(_window);

  fprintf(stderr, "Initializing glew\n");

  // ignore this error. glewInit throws an error; something to do with core profiles...
  // todo: add explanation for why this error is ignored.

  glewExperimental = GL_TRUE;
  GLenum glerr = glewInit();
  glGetError();    
  
  if (glerr != GLEW_OK){
    fprintf(stderr, "GLEW init failed: %d\n\n", glerr);
    exit(1);
  }

}

void Viewer::snap(const char *fname) {
  static const char me[]="Hale::Viewer::snap";
  glReadPixels(0, 0, _widthBuffer, _heightBuffer, GL_RGBA, GL_UNSIGNED_BYTE, _buffRGBA[0]);
  if (nrrdFlip(_nbuffRGBA[1], _nbuffRGBA[0], 2)
      || nrrdSave(fname, _nbuffRGBA[1], NULL)) {
    char *err = biffGetDone(NRRD);
    fprintf(stderr, "%s: error saving buffer:\n%s", me, err);
    free(err);
  }
  printf("%s: saved to %s\n", me, fname);
}

void Viewer::snap() {
  int si=0;
  char fname[128];
  FILE *file=NULL;
  do {
    sprintf(fname, "snap-%04d.png", si);
    file = fopen(fname, "r");
    if (file) {
      fclose(file);
      si++;
    }
  } while (file);
  snap(fname);
}

const Scene *Viewer::scene() { return _scene; }
void Viewer::scene(Scene *scn) { _scene = scn; }

void Viewer::draw(void) {

  Hale::uniform("projectMat", camera.project(), true);
  Hale::uniform("viewMat", camera.view(), true);
  /* Here is where we convert view-space light direction into world-space */
  glm::vec3 ldir = glm::vec3(camera.viewInv()*glm::vec4(_lightDir,0.0f));
  Hale::uniform("lightDir", ldir, true);
    int prog;
  glGetIntegerv(GL_CURRENT_PROGRAM,&prog);
  // fprintf(stderr, "\nusing program %d\n", prog);
  // fprintf(stderr,"Drawing!\n\n");
  glEnable(GL_SCISSOR_TEST);
  glViewport(viewportX, viewportY, viewportW, viewportH);
  glScissor(viewportX, viewportY, viewportW, viewportH);
  _scene->drawInit();
  _scene->draw();
  glViewport(0,0,mSize[0], mSize[1]);
  glScissor(0,0,mSize[0], mSize[1]);
  glDisable(GL_SCISSOR_TEST);
}

void Viewer::shapeUpdate() {
  _pixDensity = _widthBuffer/_widthScreen;
  camera.aspect(static_cast<double>(_widthBuffer)/_heightBuffer);
  updateViewportSize();
}

/* ControllerScreen */

ControllerScreen::ControllerScreen(nanogui::Vector2i s, const char* name) : nanogui::Screen(s,name), initialized(false){
  // container = new Widget(this);
  grouplayout = new nanogui::GroupLayout();
  setLayout(grouplayout);
  initialized = true;

  glfwSetWindowCloseCallback(mGLFWWindow, ControllerScreen::window_close_callback);
}
void ControllerScreen::addChild(int index, Widget * widget) {
  nanogui::Screen::addChild(index,widget);
}
void ControllerScreen::performLayout(NVGcontext *ctx) {
  int w=0,h=0;
  for (auto c : mChildren) {
    nanogui::Vector2i sz = c->preferredSize(ctx);
    nanogui::Vector2i ps = c->position();
    if(sz[0]+ps[0] > w)w = sz[0]+ps[0];
    if(sz[1]+ps[1] > h)h = sz[1]+ps[1];
  }
  nanogui::Widget::performLayout(ctx);
}
void ControllerScreen::setSize(){
  int w=0,h=0;
  for (auto c : mChildren) {
    nanogui::Vector2i sz = c->preferredSize(mNVGContext);
    nanogui::Vector2i ps = c->position();
    if(sz[0]+ps[0] > w)w = sz[0]+ps[0];
    if(sz[1]+ps[1] > h)h = sz[1]+ps[1];
    fprintf(stderr,"size %d, %d\n", sz[0]+ps[0], sz[1]+ps[1]);
  }
  nanogui::Screen::setSize(nanogui::Vector2i(w+grouplayout->margin(),h+grouplayout->margin()));
}
void ControllerScreen::performLayout(){
  nanogui::Screen::performLayout();
}
bool ControllerScreen::resizeEvent(const nanogui::Vector2i &) {
  performLayout();
  return false;
}
void ControllerScreen::addPopInButton(Widget* forWindow){
  correspondence = forWindow;
  nanogui::Button *b = new nanogui::Button(this, "Pop in");
  b->setCallback([&] {
    setVisible(false);
    correspondence->setVisible(true);
  });
  popupCorrespondence.insert(std::pair<GLFWwindow*, nanogui::Widget*>(mGLFWWindow, correspondence));
  nanogui::Button *popout = new nanogui::Button(correspondence, "Pop out");
  forWindow->layout();
  popout->setCallback([&]{
    fprintf(stderr,"callback\n");
    correspondence->setVisible(false);
    setVisible(true);
  });
}

std::map<GLFWwindow*, nanogui::Widget*> ControllerScreen::popupCorrespondence;
void ControllerScreen::window_close_callback(GLFWwindow* window){
  // ControllerScreen::popupCorrespondence[window]->setVisible(true);
  glfwSetWindowShouldClose(window, GLFW_FALSE);
}

} // namespace Hale