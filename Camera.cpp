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
  _up = up;
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
glm::mat4 Camera::project() { return _project; }
const float *Camera::viewPtr() { return glm::value_ptr(_view); }
const float *Camera::projectPtr() { return glm::value_ptr(_project); }

glm::vec3 Camera::U() { return _uu; }
glm::vec3 Camera::V() { return _vv; }
glm::vec3 Camera::N() { return _nn; }

void Camera::updateView() {
  // static const char me[]="Camera::updateView";

  _view = glm::lookAt(_from, _at, _up);

  // not actually needed: _viewInv = glm::inverse(_view);

  /* N points *towards* eye, from look-at; same as if by
     _nn = glm::vec3(_viewInv * glm::vec4(0, 0, 1, 0)) */
  _nn = glm::normalize(_from - _at);
  /* U points towards right; same as if by
     _uu = glm::vec3(_viewInv * glm::vec4(1, 0, 0, 0)); */
  _uu = glm::normalize(glm::cross(_up, _nn));
  /* V is screen-space up; same as if by
     _vv = glm::vec3(_viewInv * glm::vec4(0, 1, 0, 0));
  */
  _vv = glm::cross(_nn, _uu);

  return;
}

void Camera::updateProject() {
  //static const char me[]="Camera::updateProject";

  glm::vec3 diff = _at - _from;
  double dist = glm::length(diff);
  double vspNear = dist + _clipNear;
  double vspFar = dist + _clipFar;
  double fangle = _fov*AIR_PI/360;
  if (_orthographic) {
    double vMax = dist*sin(fangle);
    double uMax = _aspect*vMax;
    _project = glm::ortho(-uMax, uMax, -vMax, vMax, vspNear, vspFar);
  } else {
    _project = glm::perspective(2*fangle, _aspect, vspNear, vspFar);
  }
  //fprintf(stderr, "!%s: &_project = %p\n", me, glm::value_ptr(_project));

  return;
}

} // namespace Hale
