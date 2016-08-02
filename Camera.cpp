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

#include "Hale.h"
#include "privateHale.h"

namespace Hale {

Camera::Camera(glm::vec3 from, glm::vec3 at, glm::vec3 up,
               double fov, double aspect,
               double clipNear, double clipFar,
               bool orthographic) {
  // static const char me[]="Hale::Camera::Camera";

  _verbose = 0;

  _from = from;
  _at = at;
  _up = up;
  _fov = AIR_CLAMP(fovMin/fovPerc, fov, fovMax*fovPerc);
  _aspect = aspect;
  _clipNear = clipNear;
  _clipFar = clipFar;
  _orthographic = false;

  VariableExposure<double>::expose(this, "fov", [&](){return this->fov();}, [&](double in){this->fov(in);});
  VariableExposure<double>::expose(this, "nearclip", [&](){return this->clipNear();}, [&](double in){this->clipNear(in);});
  VariableExposure<double>::expose(this, "farclip", [&](){return this->clipFar();}, [&](double in){this->clipFar(in);});
  VariableExposure<bool>::expose(this, "ortho", [&](){return this->orthographic();}, [&](bool in){this->orthographic(in);});
  VariableExposure<glm::vec3>::expose(this, "upvec",
    [&](){return this->_up;},
    [&](glm::vec3 in){
      _up = in;
      updateView();
      updateProject();
    });
  VariableExposure<glm::vec3>::expose(this, "fromvec",
    [&](){return this->_from;},
    [&](glm::vec3 in){
      fprintf(stderr,"set fromvec: %f,%f,%f\n", in[0],in[1],in[2]);
      _from = in;
      updateView();
      updateProject();
    });
  VariableExposure<glm::vec3>::expose(this, "atvec",
    [&](){return this->_at;},
    [&](glm::vec3 in){
      _at = in;
      updateView();
      updateProject();
    });
  fprintf(stderr,"exposed varables\n");
  updateView();
  updateProject();
}

int Camera::verbose() { return _verbose; }
void Camera::verbose(int vv) { _verbose = vv; }

void Camera::init(glm::vec3 from, glm::vec3 at, glm::vec3 up,
                  double fov, double aspect,
                  double clipNear, double clipFar,
                  bool orthographic) {

  _from = from;
  _at = at;
  // _up = up;
  _fov = AIR_CLAMP(fovMin/fovPerc, fov, fovMax*fovPerc);
  _aspect = aspect;
  _clipNear = clipNear;
  _clipFar = clipFar;
  _orthographic = orthographic;
  updateView();
  updateProject();
}

glm::vec3 Camera::from() { return _from; }
glm::vec3 Camera::at() { return _at; }
glm::vec3 Camera::up() { return _up; }
void Camera::from(glm::vec3 ff) {
  _from = ff;
  updateView();
  updateProject();
}
void Camera::at(glm::vec3 aa) {
  _at = aa;
  updateView();
  updateProject();
}
void Camera::up(glm::vec3 uu) {
  _up = uu;
  updateView();
}

void Camera::reup() {
  glm::vec3 edir = glm::normalize(_from - _at);
  _up = glm::normalize(_up - glm::dot(_up, edir)*edir);
  updateView();
}

double Camera::fov() { return _fov; }
double Camera::aspect() { return _aspect; }
double Camera::clipNear() { return _clipNear; }
double Camera::clipFar() { return _clipFar; }
bool Camera::orthographic() { return _orthographic; }

void Camera::fov(double vv) {
  _fov = AIR_CLAMP(fovMin, vv, fovMax);
  updateProject();
}
void Camera::aspect(double aa) {
  _aspect = aa;
  updateView();
  updateProject();
}
void Camera::clipNear(double nn) {
  _clipNear = nn;
  updateProject();
}
void Camera::clipFar(double ff) {
  _clipFar = ff;
  updateProject();
}
void Camera::orthographic(bool ortho) {
  _orthographic = ortho;
  updateProject();
}

glm::mat4 Camera::view() { return _view; }
glm::mat4 Camera::viewInv() { return _viewInv; }
glm::mat4 Camera::project() { return _project; }
const float *Camera::viewPtr() { return glm::value_ptr(_view); }
const float *Camera::projectPtr() { return glm::value_ptr(_project); }

glm::vec3 Camera::U() { return _uu; }
glm::vec3 Camera::V() { return _vv; }
glm::vec3 Camera::N() { return _nn; }

void Camera::updateView() {
  // static const char me[]="Camera::updateView";

  _view = glm::lookAt(_from, _at, _up);
  _viewInv = glm::inverse(_view);

  /* not actually needed, but these lines may help for disambiguation
     and documentation; writing out the 16 elements of the value
     pointer shows that "glm is column major" (by default):
     vp[ 0]  vp[ 4]  vp[ 8]  vp[12]
     vp[ 1]  vp[ 5]  vp[ 9]  vp[13]
     vp[ 2]  vp[ 6]  vp[10]  vp[14]
     vp[ 3]  vp[ 7]  vp[11]  vp[15]
  const float *vp = glm::value_ptr(_view); */
  /*
  printf("!%s: M_view = \n", me);
  ell_4m_print_f(stdout, glm::value_ptr(glm::transpose(_view)));
  */
  /* N points *towards* eye, from look-at; same as if by
     _nn = glm::vec3(_viewInv * glm::vec4(0, 0, 1, 0)); (3rd column) or
     _nn = glm::vec3(glm::vec4(0, 0, 1, 0) * _view); (3rd row of _view) or
     _nn = glm::vec3(vp[2], vp[6], vp[10]); (3rd row) */
  _nn = glm::normalize(_from - _at);
  /* U points towards right; same as if by
     _uu = glm::vec3(_viewInv * glm::vec4(1, 0, 0, 0)); (1st column) or
     _uu = glm::vec3(glm::vec4(1, 0, 0, 0) * _view); (1st row of _view) or
     _uu = glm::vec3(vp[0], vp[4], vp[8]); (1st row of _view) */
  _uu = glm::normalize(glm::cross(_up, _nn));
  /* V is screen-space up; same as if by
     _vv = glm::vec3(_viewInv * glm::vec4(0, 1, 0, 0)); (2nd column) or
     _vv = glm::vec3(glm::vec4(0, 1, 0, 0) * _view); (2nd row or _view) or
     _vv = glm::vec3(vp[1], vp[5], vp[9]); (2nd row of _view)
  */
  _vv = glm::cross(_nn, _uu);

  return;
}

void Camera::updateProject() {
  // static const char me[]="Camera::updateProject";

  glm::vec3 diff = _at - _from;
  double dist = glm::length(diff);
  double vspNear = dist + _clipNear;
  double vspFar = dist + _clipFar;
  double fangle = _fov*AIR_PI/360;
  if (_orthographic) {
    double vMax = dist*tan(fangle);
    double uMax = _aspect*vMax;
    _project = glm::ortho(-uMax, uMax, -vMax, vMax, vspNear, vspFar);
  } else {
    _project = glm::perspective(2*fangle, _aspect, vspNear, vspFar);
    /*
    double hght = 2*vspNear * tan(fangle);
    double wdth = _aspect*hght;
    printf("!%s: 2n/w = %g,  2n/h = %g\n", me,
           2*vspNear/wdth, 2*vspNear/hght);
    printf("!%s: (f+n)/(f-n) = %g\n", me,
           (vspFar+vspNear)/(vspFar-vspNear));
    printf("!%s: -2fn/(f-n) = %g\n", me,
           -2*vspFar*vspNear/(vspFar-vspNear));
    */
  }
  // printf("!%s: %s projection = \n", me, _orthographic ? "ortho" : "persp");
  // ell_4m_print_f(stdout, glm::value_ptr(glm::transpose(_project)));

  return;
}

#define V2S(vv)                                 \
  (sprintf(buff[0], "%g", vv[0]),               \
   sprintf(buff[1], "%g", vv[1]),               \
   sprintf(buff[2], "%g", vv[2]),                                       \
   std::string(buff[0]) + " " + std::string(buff[1]) + " " + std::string(buff[2]))

#define F2S(vv)                                 \
  (sprintf(buff[0], "%g", vv),                  \
   std::string(buff[0]))

std::string Camera::hest(void) {
  char buff[3][32];
  std::string ret;
  ret = ("-fr " + V2S(_from)
         + " -at " + V2S(_at)
         + " -up " + V2S(_up)
         + " -nc " + F2S(_clipNear)
         + " -fc " + F2S(_clipFar)
         + " -fov " + F2S(_fov));
  if (_orthographic) {
    ret += " -ortho";
  }
  return ret;
}

} // namespace Hale
