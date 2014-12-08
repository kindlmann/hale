/*
  Hale: support for minimalist scientific visualization
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

namespace Hale {

bool
finishing = false;

static void
errorGLFW(int errnum, const char *errstr) {
  static int count = 0;
  static const char me[]="Hale::errorGLFW";

  /*
  ** HEY: following needs to be kept in synch
  ** with GLFW's error codes, listed at
  ** http://www.glfw.org/docs/latest/group__errors.html
  */
  fprintf(stderr, "%s: Err# %d (", me, errnum);
  switch (errnum) {
  case GLFW_NOT_INITIALIZED:
    fprintf(stderr, "GLFW_NOT_INITIALIZED");
    break;
  case GLFW_NO_CURRENT_CONTEXT:
    fprintf(stderr, "GLFW_NO_CURRENT_CONTEXT");
    break;
  case GLFW_INVALID_ENUM:
    fprintf(stderr, "GLFW_INVALID_ENUM");
    break;
  case GLFW_INVALID_VALUE:
    fprintf(stderr, "GLFW_INVALID_VALUE");
    break;
  case GLFW_OUT_OF_MEMORY:
    fprintf(stderr, "GLFW_OUT_OF_MEMORY");
    break;
  case GLFW_API_UNAVAILABLE:
    fprintf(stderr, "GLFW_API_UNAVAILABLE");
    break;
  case GLFW_VERSION_UNAVAILABLE:
    fprintf(stderr, "GLFW_VERSION_UNAVAILABLE");
    break;
  case GLFW_PLATFORM_ERROR:
    fprintf(stderr, "GLFW_PLATFORM_ERROR");
    break;
  case GLFW_FORMAT_UNAVAILABLE:
    fprintf(stderr, "GLFW_FORMAT_UNAVAILABLE");
    break;
  default:
    fprintf(stderr, "?? unknown GLFW error code ??");
    break;
  }
  fprintf(stderr, "): \"%s\"\n", errstr);

  count++;
  if (10 == count) {
    fprintf(stderr, "%s: too many calls; bye\n", me);
    exit(1);
  }
  return;
}

int
init() {
  static const char me[]="Hale::init";
  int iret;

  /* sanity check for Teem */
  nrrdSanityOrDie(me);

  /* need any sanity checks of our own? */

  /* install GLFW error hander, then try glfwInit */
  glfwSetErrorCallback(errorGLFW);
  iret = glfwInit();
  /*
  fprintf(stderr, "%s: glfwInit returned %d (%s)\n", me, iret,
          (GL_TRUE == iret
           ? "GL_TRUE; all's well"
           : (GL_FALSE == iret
              ? "GL_FALSE"
              : "what?")));
  */
  if (GL_TRUE != iret) {
    fprintf(stderr, "%s: trouble with glfwInit\n", me);
    return 1;
  }

  return 0;
}

void
done() {
  // HEY should have a way of taking mops to clean up
  glfwTerminate();
  return;
}

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

/*
** Hey: check out https://www.opengl.org/wiki/Debug_Output
** (core in version 4.3)
*/
void
glErrorCheck(std::string whence, std::string context) {
  std::string desc;
  GLenum err = glGetError();
  if (GL_NO_ERROR != err) {
    switch (err) {
    case GL_INVALID_ENUM:
      desc = "GL_INVALID_ENUM";
      break;
    case GL_INVALID_VALUE:
      desc = "GL_INVALID_VALUE";
      break;
    case GL_INVALID_OPERATION:
      desc = "GL_INVALID_OPERATION";
      break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      desc = "GL_INVALID_FRAMEBUFFER_OPERATION";
      break;
    case GL_OUT_OF_MEMORY:
      desc = "GL_OUT_OF_MEMORY";
      break;
      /* These seem to be for older versions of OpenGL
    case GL_STACK_OVERFLOW:
      desc = "GL_STACK_OVERFLOW";
      break;
    case GL_STACK_UNDERFLOW:
      desc = "GL_STACK_UNDERFLOW";
      break;
    case GL_TABLE_TOO_LARGE:
      desc = "GL_TABLE_TOO_LARGE";
      break;
      */
    default:
      desc = "unknown error value " + std::to_string(static_cast<int>(err));
      break;
    }
    throw std::runtime_error(whence + ": " + context + ": glGetError(): " + desc);
  }
  return;
}

} // namespace Hale
