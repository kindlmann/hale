/*
  Hale: support for minimalist scientific visualization
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

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp> // for glm::lookAt(), glm::perspective()
#include <glm/gtx/string_cast.hpp> // for glm::to_string()
#include <glm/gtc/type_ptr.hpp> // for glm::value_ptr()

namespace Hale {

#define GLBOOLSTR(status) (GL_FALSE == (status) ? "GL_FALSE" : (GL_TRUE == (status) ? "GL_TRUE" : "GL_?!?!?"))

/* a field-of-view of 180 is useless and weird, so we want to make
   sure we don't get any where near that. Likewise prevent fov from
   getting uselessly small, so we start by clamping fov to range
   [fovMin/fovPerc, fovMax*fovPerc] */
const double fovMax = 125;
const double fovMin = 0.05;
const double fovPerc = 0.99;
/* when resetting view to contain scene, what FOV to use */
const double fovBest = 20;

/* globals.cpp */
extern const Program *_programCurrent;

/* compiled as needed */
extern const Program *_program[preprogramLast];

}
