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

Scene::Scene() {

  ELL_3V_SET(_bgColor, 0.1, 0.15, 0.2);
  _lightDir = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
}

Scene::~Scene() {
  /* anything to clean up? */
}

void Scene::add(const Polydata *pd) { _polydata.push_back(pd); }

void Scene::bgColor(float rr, float gg, float bb) { ELL_3V_SET(_bgColor, rr, gg, bb); }

glm::vec3 Scene::bgColor() const {
  return glm::vec3(_bgColor[0], _bgColor[1], _bgColor[2]);
}

void Scene::lightDir(glm::vec3 dir) { _lightDir = glm::normalize(dir); }
glm::vec3 Scene::lightDir() const { return _lightDir; }

void Scene::drawInit() {
  glClearColor(_bgColor[0], _bgColor[1], _bgColor[2], 0.0f);
  if (debugging)
    printf("# glClearColor(%g, %g, %g, 1.0f);\n", _bgColor[0], _bgColor[1], _bgColor[2]);
  glEnable(GL_DEPTH_TEST);
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA,GL_ONE);
}

void Scene::bounds(glm::vec3& finalmin, glm::vec3& finalmax) const {
  glm::vec3 min, max;
  auto pi = _polydata.begin();
  if (*pi) {
    (*pi)->bounds(min, max);
    pi++;
    for (; pi != _polydata.end(); pi++) {
      glm::vec3 tmin, tmax;
      (*pi)->bounds(tmin, tmax);
      min = glm::min(min, tmin);
      max = glm::max(max, tmax);
    }
    finalmin = min;
    finalmax = max;
  } else {
    finalmin = finalmax = glm::vec3(0.0f, 0.0f, 0.0f);
  }
}

void Scene::draw() {

  glClear(GL_DEPTH_BUFFER_BIT);
  if (debugging)
    printf("# glClear(GL_DEPTH_BUFFER_BIT);\n");
  glClear(GL_COLOR_BUFFER_BIT);
  if (debugging)
    printf("# glClear(GL_COLOR_BUFFER_BIT);\n");

  for (auto pi = _polydata.begin(); pi != _polydata.end(); pi++) {
    (*pi)->draw();
  }

}


} // namespace Hale
