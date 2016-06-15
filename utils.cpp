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

#include <GL/glew.h>
#include "Hale.h"
#include "privateHale.h"

namespace Hale {

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

std::map<GLenum, glEnumItem> glEnumDesc;

static void
glEnumDescInit() {

  /* created with a emacs keyboard macro from info copy-and-pasted from
     https://www.opengl.org/sdk/docs/man/html/glGetActiveUniform.xhtml */
  glEnumDesc[GL_FLOAT] = {GL_FLOAT, "GL_FLOAT", "float"};
  glEnumDesc[GL_FLOAT_VEC2] = {GL_FLOAT_VEC2, "GL_FLOAT_VEC2", "vec2"};
  glEnumDesc[GL_FLOAT_VEC3] = {GL_FLOAT_VEC3, "GL_FLOAT_VEC3", "vec3"};
  glEnumDesc[GL_FLOAT_VEC4] = {GL_FLOAT_VEC4, "GL_FLOAT_VEC4", "vec4"};
  glEnumDesc[GL_DOUBLE] = {GL_DOUBLE, "GL_DOUBLE", "double"};
  glEnumDesc[GL_DOUBLE_VEC2] = {GL_DOUBLE_VEC2, "GL_DOUBLE_VEC2", "dvec2"};
  glEnumDesc[GL_DOUBLE_VEC3] = {GL_DOUBLE_VEC3, "GL_DOUBLE_VEC3", "dvec3"};
  glEnumDesc[GL_DOUBLE_VEC4] = {GL_DOUBLE_VEC4, "GL_DOUBLE_VEC4", "dvec4"};
  glEnumDesc[GL_INT] = {GL_INT, "GL_INT", "int"};
  glEnumDesc[GL_INT_VEC2] = {GL_INT_VEC2, "GL_INT_VEC2", "ivec2"};
  glEnumDesc[GL_INT_VEC3] = {GL_INT_VEC3, "GL_INT_VEC3", "ivec3"};
  glEnumDesc[GL_INT_VEC4] = {GL_INT_VEC4, "GL_INT_VEC4", "ivec4"};
  glEnumDesc[GL_UNSIGNED_INT] = {GL_UNSIGNED_INT, "GL_UNSIGNED_INT", "unsigned int"};
  glEnumDesc[GL_UNSIGNED_INT_VEC2] = {GL_UNSIGNED_INT_VEC2, "GL_UNSIGNED_INT_VEC2", "uvec2"};
  glEnumDesc[GL_UNSIGNED_INT_VEC3] = {GL_UNSIGNED_INT_VEC3, "GL_UNSIGNED_INT_VEC3", "uvec3"};
  glEnumDesc[GL_UNSIGNED_INT_VEC4] = {GL_UNSIGNED_INT_VEC4, "GL_UNSIGNED_INT_VEC4", "uvec4"};
  glEnumDesc[GL_BOOL] = {GL_BOOL, "GL_BOOL", "bool"};
  glEnumDesc[GL_BOOL_VEC2] = {GL_BOOL_VEC2, "GL_BOOL_VEC2", "bvec2"};
  glEnumDesc[GL_BOOL_VEC3] = {GL_BOOL_VEC3, "GL_BOOL_VEC3", "bvec3"};
  glEnumDesc[GL_BOOL_VEC4] = {GL_BOOL_VEC4, "GL_BOOL_VEC4", "bvec4"};
  glEnumDesc[GL_FLOAT_MAT2] = {GL_FLOAT_MAT2, "GL_FLOAT_MAT2", "mat2"};
  glEnumDesc[GL_FLOAT_MAT3] = {GL_FLOAT_MAT3, "GL_FLOAT_MAT3", "mat3"};
  glEnumDesc[GL_FLOAT_MAT4] = {GL_FLOAT_MAT4, "GL_FLOAT_MAT4", "mat4"};
  glEnumDesc[GL_FLOAT_MAT2x3] = {GL_FLOAT_MAT2x3, "GL_FLOAT_MAT2x3", "mat2x3"};
  glEnumDesc[GL_FLOAT_MAT2x4] = {GL_FLOAT_MAT2x4, "GL_FLOAT_MAT2x4", "mat2x4"};
  glEnumDesc[GL_FLOAT_MAT3x2] = {GL_FLOAT_MAT3x2, "GL_FLOAT_MAT3x2", "mat3x2"};
  glEnumDesc[GL_FLOAT_MAT3x4] = {GL_FLOAT_MAT3x4, "GL_FLOAT_MAT3x4", "mat3x4"};
  glEnumDesc[GL_FLOAT_MAT4x2] = {GL_FLOAT_MAT4x2, "GL_FLOAT_MAT4x2", "mat4x2"};
  glEnumDesc[GL_FLOAT_MAT4x3] = {GL_FLOAT_MAT4x3, "GL_FLOAT_MAT4x3", "mat4x3"};
  glEnumDesc[GL_DOUBLE_MAT2] = {GL_DOUBLE_MAT2, "GL_DOUBLE_MAT2", "dmat2"};
  glEnumDesc[GL_DOUBLE_MAT3] = {GL_DOUBLE_MAT3, "GL_DOUBLE_MAT3", "dmat3"};
  glEnumDesc[GL_DOUBLE_MAT4] = {GL_DOUBLE_MAT4, "GL_DOUBLE_MAT4", "dmat4"};
  glEnumDesc[GL_DOUBLE_MAT2x3] = {GL_DOUBLE_MAT2x3, "GL_DOUBLE_MAT2x3", "dmat2x3"};
  glEnumDesc[GL_DOUBLE_MAT2x4] = {GL_DOUBLE_MAT2x4, "GL_DOUBLE_MAT2x4", "dmat2x4"};
  glEnumDesc[GL_DOUBLE_MAT3x2] = {GL_DOUBLE_MAT3x2, "GL_DOUBLE_MAT3x2", "dmat3x2"};
  glEnumDesc[GL_DOUBLE_MAT3x4] = {GL_DOUBLE_MAT3x4, "GL_DOUBLE_MAT3x4", "dmat3x4"};
  glEnumDesc[GL_DOUBLE_MAT4x2] = {GL_DOUBLE_MAT4x2, "GL_DOUBLE_MAT4x2", "dmat4x2"};
  glEnumDesc[GL_DOUBLE_MAT4x3] = {GL_DOUBLE_MAT4x3, "GL_DOUBLE_MAT4x3", "dmat4x3"};
  glEnumDesc[GL_SAMPLER_1D] = {GL_SAMPLER_1D, "GL_SAMPLER_1D", "sampler1D"};
  glEnumDesc[GL_SAMPLER_2D] = {GL_SAMPLER_2D, "GL_SAMPLER_2D", "sampler2D"};
  glEnumDesc[GL_SAMPLER_3D] = {GL_SAMPLER_3D, "GL_SAMPLER_3D", "sampler3D"};
  glEnumDesc[GL_SAMPLER_CUBE] = {GL_SAMPLER_CUBE, "GL_SAMPLER_CUBE", "samplerCube"};
  glEnumDesc[GL_SAMPLER_1D_SHADOW] = {GL_SAMPLER_1D_SHADOW, "GL_SAMPLER_1D_SHADOW", "sampler1DShadow"};
  glEnumDesc[GL_SAMPLER_2D_SHADOW] = {GL_SAMPLER_2D_SHADOW, "GL_SAMPLER_2D_SHADOW", "sampler2DShadow"};
  glEnumDesc[GL_SAMPLER_1D_ARRAY] = {GL_SAMPLER_1D_ARRAY, "GL_SAMPLER_1D_ARRAY", "sampler1DArray"};
  glEnumDesc[GL_SAMPLER_2D_ARRAY] = {GL_SAMPLER_2D_ARRAY, "GL_SAMPLER_2D_ARRAY", "sampler2DArray"};
  glEnumDesc[GL_SAMPLER_1D_ARRAY_SHADOW] = {GL_SAMPLER_1D_ARRAY_SHADOW, "GL_SAMPLER_1D_ARRAY_SHADOW", "sampler1DArrayShadow"};
  glEnumDesc[GL_SAMPLER_2D_ARRAY_SHADOW] = {GL_SAMPLER_2D_ARRAY_SHADOW, "GL_SAMPLER_2D_ARRAY_SHADOW", "sampler2DArrayShadow"};
  glEnumDesc[GL_SAMPLER_2D_MULTISAMPLE] = {GL_SAMPLER_2D_MULTISAMPLE, "GL_SAMPLER_2D_MULTISAMPLE", "sampler2DMS"};
  glEnumDesc[GL_SAMPLER_2D_MULTISAMPLE_ARRAY] = {GL_SAMPLER_2D_MULTISAMPLE_ARRAY, "GL_SAMPLER_2D_MULTISAMPLE_ARRAY", "sampler2DMSArray"};
  glEnumDesc[GL_SAMPLER_CUBE_SHADOW] = {GL_SAMPLER_CUBE_SHADOW, "GL_SAMPLER_CUBE_SHADOW", "samplerCubeShadow"};
  glEnumDesc[GL_SAMPLER_BUFFER] = {GL_SAMPLER_BUFFER, "GL_SAMPLER_BUFFER", "samplerBuffer"};
  glEnumDesc[GL_SAMPLER_2D_RECT] = {GL_SAMPLER_2D_RECT, "GL_SAMPLER_2D_RECT", "sampler2DRect"};
  glEnumDesc[GL_SAMPLER_2D_RECT_SHADOW] = {GL_SAMPLER_2D_RECT_SHADOW, "GL_SAMPLER_2D_RECT_SHADOW", "sampler2DRectShadow"};
  glEnumDesc[GL_INT_SAMPLER_1D] = {GL_INT_SAMPLER_1D, "GL_INT_SAMPLER_1D", "isampler1D"};
  glEnumDesc[GL_INT_SAMPLER_2D] = {GL_INT_SAMPLER_2D, "GL_INT_SAMPLER_2D", "isampler2D"};
  glEnumDesc[GL_INT_SAMPLER_3D] = {GL_INT_SAMPLER_3D, "GL_INT_SAMPLER_3D", "isampler3D"};
  glEnumDesc[GL_INT_SAMPLER_CUBE] = {GL_INT_SAMPLER_CUBE, "GL_INT_SAMPLER_CUBE", "isamplerCube"};
  glEnumDesc[GL_INT_SAMPLER_1D_ARRAY] = {GL_INT_SAMPLER_1D_ARRAY, "GL_INT_SAMPLER_1D_ARRAY", "isampler1DArray"};
  glEnumDesc[GL_INT_SAMPLER_2D_ARRAY] = {GL_INT_SAMPLER_2D_ARRAY, "GL_INT_SAMPLER_2D_ARRAY", "isampler2DArray"};
  glEnumDesc[GL_INT_SAMPLER_2D_MULTISAMPLE] = {GL_INT_SAMPLER_2D_MULTISAMPLE, "GL_INT_SAMPLER_2D_MULTISAMPLE", "isampler2DMS"};
  glEnumDesc[GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY] = {GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, "GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY", "isampler2DMSArray"};
  glEnumDesc[GL_INT_SAMPLER_BUFFER] = {GL_INT_SAMPLER_BUFFER, "GL_INT_SAMPLER_BUFFER", "isamplerBuffer"};
  glEnumDesc[GL_INT_SAMPLER_2D_RECT] = {GL_INT_SAMPLER_2D_RECT, "GL_INT_SAMPLER_2D_RECT", "isampler2DRect"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_1D] = {GL_UNSIGNED_INT_SAMPLER_1D, "GL_UNSIGNED_INT_SAMPLER_1D", "usampler1D"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_2D] = {GL_UNSIGNED_INT_SAMPLER_2D, "GL_UNSIGNED_INT_SAMPLER_2D", "usampler2D"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_3D] = {GL_UNSIGNED_INT_SAMPLER_3D, "GL_UNSIGNED_INT_SAMPLER_3D", "usampler3D"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_CUBE] = {GL_UNSIGNED_INT_SAMPLER_CUBE, "GL_UNSIGNED_INT_SAMPLER_CUBE", "usamplerCube"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_1D_ARRAY] = {GL_UNSIGNED_INT_SAMPLER_1D_ARRAY, "GL_UNSIGNED_INT_SAMPLER_1D_ARRAY", "usampler2DArray"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_2D_ARRAY] = {GL_UNSIGNED_INT_SAMPLER_2D_ARRAY, "GL_UNSIGNED_INT_SAMPLER_2D_ARRAY", "usampler2DArray"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE] = {GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE, "GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE", "usampler2DMS"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY] = {GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, "GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY", "usampler2DMSArray"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_BUFFER] = {GL_UNSIGNED_INT_SAMPLER_BUFFER, "GL_UNSIGNED_INT_SAMPLER_BUFFER", "usamplerBuffer"};
  glEnumDesc[GL_UNSIGNED_INT_SAMPLER_2D_RECT] = {GL_UNSIGNED_INT_SAMPLER_2D_RECT, "GL_UNSIGNED_INT_SAMPLER_2D_RECT", "usampler2DRect"};
  /*
  ** These seem to be only for older OpenGL versions
  glEnumDesc[GL_IMAGE_1D] = {GL_IMAGE_1D, "GL_IMAGE_1D", "image1D"};
  glEnumDesc[GL_IMAGE_2D] = {GL_IMAGE_2D, "GL_IMAGE_2D", "image2D"};
  glEnumDesc[GL_IMAGE_3D] = {GL_IMAGE_3D, "GL_IMAGE_3D", "image3D"};
  glEnumDesc[GL_IMAGE_2D_RECT] = {GL_IMAGE_2D_RECT, "GL_IMAGE_2D_RECT", "image2DRect"};
  glEnumDesc[GL_IMAGE_CUBE] = {GL_IMAGE_CUBE, "GL_IMAGE_CUBE", "imageCube"};
  glEnumDesc[GL_IMAGE_BUFFER] = {GL_IMAGE_BUFFER, "GL_IMAGE_BUFFER", "imageBuffer"};
  glEnumDesc[GL_IMAGE_1D_ARRAY] = {GL_IMAGE_1D_ARRAY, "GL_IMAGE_1D_ARRAY", "image1DArray"};
  glEnumDesc[GL_IMAGE_2D_ARRAY] = {GL_IMAGE_2D_ARRAY, "GL_IMAGE_2D_ARRAY", "image2DArray"};
  glEnumDesc[GL_IMAGE_2D_MULTISAMPLE] = {GL_IMAGE_2D_MULTISAMPLE, "GL_IMAGE_2D_MULTISAMPLE", "image2DMS"};
  glEnumDesc[GL_IMAGE_2D_MULTISAMPLE_ARRAY] = {GL_IMAGE_2D_MULTISAMPLE_ARRAY, "GL_IMAGE_2D_MULTISAMPLE_ARRAY", "image2DMSArray"};
  glEnumDesc[GL_INT_IMAGE_1D] = {GL_INT_IMAGE_1D, "GL_INT_IMAGE_1D", "iimage1D"};
  glEnumDesc[GL_INT_IMAGE_2D] = {GL_INT_IMAGE_2D, "GL_INT_IMAGE_2D", "iimage2D"};
  glEnumDesc[GL_INT_IMAGE_3D] = {GL_INT_IMAGE_3D, "GL_INT_IMAGE_3D", "iimage3D"};
  glEnumDesc[GL_INT_IMAGE_2D_RECT] = {GL_INT_IMAGE_2D_RECT, "GL_INT_IMAGE_2D_RECT", "iimage2DRect"};
  glEnumDesc[GL_INT_IMAGE_CUBE] = {GL_INT_IMAGE_CUBE, "GL_INT_IMAGE_CUBE", "iimageCube"};
  glEnumDesc[GL_INT_IMAGE_BUFFER] = {GL_INT_IMAGE_BUFFER, "GL_INT_IMAGE_BUFFER", "iimageBuffer"};
  glEnumDesc[GL_INT_IMAGE_1D_ARRAY] = {GL_INT_IMAGE_1D_ARRAY, "GL_INT_IMAGE_1D_ARRAY", "iimage1DArray"};
  glEnumDesc[GL_INT_IMAGE_2D_ARRAY] = {GL_INT_IMAGE_2D_ARRAY, "GL_INT_IMAGE_2D_ARRAY", "iimage2DArray"};
  glEnumDesc[GL_INT_IMAGE_2D_MULTISAMPLE] = {GL_INT_IMAGE_2D_MULTISAMPLE, "GL_INT_IMAGE_2D_MULTISAMPLE", "iimage2DMS"};
  glEnumDesc[GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY] = {GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY, "GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY", "iimage2DMSArray"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_1D] = {GL_UNSIGNED_INT_IMAGE_1D, "GL_UNSIGNED_INT_IMAGE_1D", "uimage1D"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_2D] = {GL_UNSIGNED_INT_IMAGE_2D, "GL_UNSIGNED_INT_IMAGE_2D", "uimage2D"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_3D] = {GL_UNSIGNED_INT_IMAGE_3D, "GL_UNSIGNED_INT_IMAGE_3D", "uimage3D"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_2D_RECT] = {GL_UNSIGNED_INT_IMAGE_2D_RECT, "GL_UNSIGNED_INT_IMAGE_2D_RECT", "uimage2DRect"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_CUBE] = {GL_UNSIGNED_INT_IMAGE_CUBE, "GL_UNSIGNED_INT_IMAGE_CUBE", "uimageCube"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_BUFFER] = {GL_UNSIGNED_INT_IMAGE_BUFFER, "GL_UNSIGNED_INT_IMAGE_BUFFER", "uimageBuffer"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_1D_ARRAY] = {GL_UNSIGNED_INT_IMAGE_1D_ARRAY, "GL_UNSIGNED_INT_IMAGE_1D_ARRAY", "uimage1DArray"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_2D_ARRAY] = {GL_UNSIGNED_INT_IMAGE_2D_ARRAY, "GL_UNSIGNED_INT_IMAGE_2D_ARRAY", "uimage2DArray"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE] = {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE, "GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE", "uimage2DMS"};
  glEnumDesc[GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY] = {GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, "GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY", "uimage2DMSArray"};
  glEnumDesc[GL_UNSIGNED_INT_ATOMIC_COUNTER] = {GL_UNSIGNED_INT_ATOMIC_COUNTER, "GL_UNSIGNED_INT_ATOMIC_COUNTER", "atomic_uint"};
  */
  return;
}

void
init() {
  static const std::string me="Hale::init";
  int iret;

  /* sanity check for Teem */
  nrrdSanityOrDie(me.c_str());

  /* HEY: should make sure that Hale:vertAttrIdx really mirrors
     limnPolyDataInfo */

  /* need any other sanity checks of our own? */

  /* populate glEnumDesc (HEY figure out compile-time initialization) */
  glEnumDescInit();

  /* install GLFW error hander, then try glfwInit */
  glfwSetErrorCallback(errorGLFW);
  iret = glfwInit();



  /*
  fprintf(stderr, "%s: glfwInit returned %d (%s)\n", me.c_str(), iret,
          (GL_TRUE == iret
           ? "GL_TRUE; all's well"
           : (GL_FALSE == iret
              ? "GL_FALSE"
              : "what?")));
  */
  if (GL_TRUE != iret) {
    throw std::runtime_error(me + ": glfwInit failed");
  }
  printf("%s: GLFW version \"%s\" initialized\n", me.c_str(), glfwGetVersionString());

  return;
}

void
done() {

  for (int pi=preprogramUnknown+1;
       pi<preprogramLast;
       pi++) {
    if (_program[pi]) {
      delete _program[pi];
    }
  }

  /* HEY: without this, the errorGLFW is called at exit with
     GLFW_NOT_INITIALIZED/"The GLFW library is not initialized",
     perhaps by the atexit() stack, but isn't that turned off with
     GLFW version 3.0? http://www.glfw.org/changelog.html */
  glfwSetErrorCallback(NULL);
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
      /* These seem to be only for older versions of OpenGL
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
