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

#include "hale.hpp"

namespace hale {

/* Converts a teem enum to an openGL enum */
GLuint
limnToGLPrim(int type) {
  GLuint ret;
  switch(type){
  case limnPrimitiveTriangles:
    ret = GL_TRIANGLES;
    break;
  case limnPrimitiveTriangleStrip:
    ret = GL_TRIANGLE_STRIP;
    break;
  case limnPrimitiveTriangleFan:
    ret = GL_TRIANGLE_FAN;
    break;
  case limnPrimitiveLineStrip:
    ret = GL_LINE_STRIP;
    break;
  case limnPrimitiveLines:
    ret = GL_LINES;
    break;
  case limnPrimitiveUnknown:
  case limnPrimitiveNoop:
  case limnPrimitiveLast:
  case limnPrimitiveQuads: /* no longer part of OpenGL3 */
  default:
    ret = 0; /* HEY: make sure this is how OpenGL says: "invalid" */
    break;
  }
  return ret;
}

}
