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
  appreciated but is not requilsred.

  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

#include <GL/glew.h>
#include "Hale.h"
#include "GUI.h"
#include "privateHale.h"

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

/* CEGUI functions. todo: make cleaner */



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

void cegui_charCallback(GLFWwindow* window, unsigned int char_pressed){
    HaleGUI::gui_charCallback(window,char_pressed);    
}

void cegui_cursorPosCallback(GLFWwindow* window, double x, double y){
    HaleGUI::gui_cursorPosCallback(window,x,y);
}

void cegui_keyCallback(GLFWwindow* window, int key, int scan, int action, int mod){
    HaleGUI::gui_keyCallback(window,key,scan,action,mod);
}

void cegui_mouseButtonCallback(GLFWwindow* window, int button, int state, int mod){
    HaleGUI::gui_mouseButtonCallback(window,button,state,mod);
}

void cegui_mouseWheelCallback(GLFWwindow* window, double x, double y){
    HaleGUI::gui_mouseWheelCallback(window,x,y);
}


void cegui_errorCallback(int error, const char* message){
    CEGUI::Logger::getSingleton().logEvent(message, CEGUI::Errors);
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

void
Viewer::keyCB(GLFWwindow *gwin, int key, int scancode, int action, int mods) {
  static const char me[]="keyCB";
  cegui_keyCallback(gwin,key,scancode,action,mods);
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
  if(HaleGUI::gui_mouseButtonCallback(gwin,button,action,mods))return;
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

  bool handled = (HaleGUI::gui_cursorPosCallback(gwin, xx, yy));
  if(handled)return;
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
      break;
    case viewerModeRotateU:
      nfrom = vwr->camera.at() + elen*glm::normalize(toeye - rotY*vv);
      break;
    case viewerModeRotateV:
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
Viewer::Viewer(int width, int height, const char *label, Scene *scene) {
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

  // http://www.glfw.org/docs/latest/window.html
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Use OpenGL Core v3.2
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  /* look into using this!
     glfwWindowHint(GLFW_SAMPLES, 0); */
  _label = label ? label : "Viewer";
  _window = glfwCreateWindow(width, height, _label.c_str(), NULL, NULL);
  if (!_window) {
    fprintf(stderr, "%s: glfwCreateWindow(%d,%d) failed\n",
            me, width, height);
    finishing = true;
  }

  glfwGetFramebufferSize(_window, &_widthBuffer, &_heightBuffer);
  _nbuffRGBA[0] = nrrdNew();
  _nbuffRGBA[1] = nrrdNew();
  _buffAlloc();
  glfwSetWindowUserPointer(_window, static_cast<void*>(this));
  glfwSetCursorPosCallback(_window, cursorPosCB);
  glfwSetMouseButtonCallback(_window, mouseButtonCB);
  // glfwSetWindowSizeCallback(_window, windowSizeCB);
  glfwSetFramebufferSizeCallback(_window, framebufferSizeCB);
  glfwSetKeyCallback(_window, keyCB);
  glfwSetWindowCloseCallback(_window, windowCloseCB);
  glfwSetWindowRefreshCallback(_window, windowRefreshCB);


  /*     CEGUI  input callbacks    */

  glfwSetCharCallback(_window, cegui_charCallback);
  // glfwSetCursorPosCallback(window, cegui_cursorPosCallback);
  // glfwSetKeyCallback(window, cegui_keyCallback);
  // glfwSetMouseButtonCallback(window, cegui_mouseButtonCallback);
  glfwSetScrollCallback(_window, cegui_mouseWheelCallback);

  // // window callback
  glfwSetWindowSizeCallback(_window, HaleGUI::gui_windowResizedCallback);

    // // error callback
  // glfwSetErrorCallback(errorCallback);}

  shapeUpdate();
  title();



  printf("\nType 'r' to reset view, 'h' for help using keyboard and viewer\n");
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
  glfwMakeContextCurrent(_window);

  fprintf(stderr, "Initializing glew\n");

  // ignore this error. glewInit throws an error; something to do with core profiles...
  // todo: add explanation for why this error is ignored.

  glewExperimental = GL_TRUE;
  GLenum glerr = glewInit();
  GLenum err = glGetError();    
  
  if (glerr != GLEW_OK){
    fprintf(stderr, "GLEW init failed: %d\n\n", glerr);
    exit(1); // or handle the error in a nicer way
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


  if(HaleGUI::getInstance()->leftPane){
    HaleGUI::getInstance()->leftPane->setPosition(CEGUI::UVector2(CEGUI::UDim(0,0),CEGUI::UDim(0,0)));
    float rect_width = HaleGUI::getInstance()->leftPane->getClipRect().getWidth();
    int newwidth = _widthBuffer-rect_width;
    if(newwidth < 1){
      newwidth = 1;
    }
    glViewport(rect_width, 0, newwidth, _heightBuffer);
  }

  // glViewport(0, 0, _widthBuffer/2, _heightBuffer);
  // glScissor(0, 0, _widthBuffer/2, _heightBuffer);
  Hale::uniform("projectMat", camera.project(), true);
  Hale::uniform("viewMat", camera.view(), true);
  /* Here is where we convert view-space light direction into world-space */
  glm::vec3 ldir = glm::vec3(camera.viewInv()*glm::vec4(_lightDir,0.0f));
  Hale::uniform("lightDir", ldir, true);
  _scene->draw();

  glViewport(0, 0, _widthBuffer, _heightBuffer);
  // glViewport(_widthBuffer/2, 0, _widthBuffer/2, _heightBuffer);
  // glClearColor(0,0,0,0);
  // glScissor(_widthBuffer/2, 0, _widthBuffer/2, _heightBuffer);
  // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  HaleGUI::getInstance()->renderAll();
}

/*
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