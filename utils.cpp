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

int
init() {
  static const char me[]="Hale::init";
  int iret;

  nrrdSanityOrDie(me);
  /* install GLFW error hander, then try glfwInit */
  glfwSetErrorCallback(errorGLFW);
  iret = glfwInit();
  fprintf(stderr, "%s: glfwInit returned %d (%s)\n", me, iret,
          (GL_TRUE == iret
           ? "GL_TRUE; all's well"
           : (GL_FALSE == iret
              ? "GL_FALSE"
              : "what?")));
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

void
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

char *
fileContents(const char *fname) {
  static const char me[]="Hale::fileContents";
  char *ret;
  FILE *file;
  long len;

  if (!fname) {
    fprintf(stderr, "%s: got NULL fname", me);
    return NULL;
  }
  if (!(file = fopen(fname, "r"))) {
    fprintf(stderr, "%s: couldn't open \"%s\" for reading", me, fname);
    return NULL;
  }
  /* learn length of file contents */
  fseek(file, 0L, SEEK_END);
  len = ftell(file);
  fseek(file, 0L, SEEK_SET);
  ret = (char*)malloc(len+1);
  if (!ret) {
    fprintf(stderr, "%s: allocation failure", me);
    fclose(file);
    return NULL;
  }
  /* copy file into string */
  fread(ret, 1, len, file);
  ret[len] = '\0';
  fclose(file);
  return ret;
}

GLint
shaderNew(GLint shtype, const char *filename) {
  static const char me[]="Hale::shaderNew";
  GLuint shaderId;
  GLint status;
  char *shaderTxt;

  shaderId = glCreateShader(shtype);
  if (!shaderId) {
    /* HEY should be using glGetError? */
    fprintf(stderr, "%s: trouble creating shader of type %d", me, shtype);
    return 0;
  }
  if (!(shaderTxt = fileContents(filename))) {
    fprintf(stderr, "%s: trouble reading from \"%s\"", me, filename);
    return 0;
  }
  glShaderSource(shaderId, 1, (const GLchar **)(&shaderTxt), NULL);
  glCompileShader(shaderId);
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
  if (GL_FALSE == status) {
    GLint logSize;
    char *logMsg;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize) {
      logMsg = (char*)malloc(logSize);
      glGetShaderInfoLog(shaderId, logSize, NULL, logMsg);
      fprintf(stderr, "%s: shader compiler error:\n%s", me, logMsg);
      glDeleteShader(shaderId);
      free(logMsg);
    }
    return 0;
  }
  return shaderId;
}


} // namespace Hale
