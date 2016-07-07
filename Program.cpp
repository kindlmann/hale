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

#include "Hale.h"
#include "privateHale.h"

namespace Hale {

/* gadgets to map uniform names to variables */
std::map<std::string, float> stickyUniformFloat;
std::map<std::string, glm::vec3> stickyUniformVec3;
std::map<std::string, glm::vec4> stickyUniformVec4;
std::map<std::string, glm::mat4> stickyUniformMat4;

#define VERSION "#version 150 core\n "
static const char *AmbDiff_vert =
  (VERSION
   "uniform mat4 projectMat;\n "
   "uniform mat4 viewMat;\n "
   "uniform mat4 modelMat;\n "
   "in vec4 positionVA;\n "
   "in vec3 normalVA;\n "
   "in vec4 colorVA;\n "
   "out vec3 norm_frag;\n "
   "out vec4 color_frag;\n "
   "mat4 modIT = transpose(inverse(modelMat));\n "
   "void main(void) {\n "
   "  gl_Position = projectMat * viewMat * modelMat * positionVA;\n "
   "  norm_frag = mat3(modIT) * normalVA;\n "
   "  color_frag = colorVA;\n "
   "}\n ");

static const char *AmbDiffSolid_vert =
  (VERSION
   "uniform mat4 projectMat;\n "
   "uniform mat4 viewMat;\n "
   "uniform mat4 modelMat;\n "
   "uniform vec4 colorSolid;\n "
   "in vec4 positionVA;\n "
   "in vec3 normalVA;\n "
   "out vec3 norm_frag;\n "
   "out vec4 color_frag;\n "
   "mat4 modIT = transpose(inverse(modelMat));\n "
   "void main(void) {\n "
   "  gl_Position = projectMat * viewMat * modelMat * positionVA;\n "
   "  norm_frag = mat3(modIT) * normalVA;\n "
   "  color_frag = colorSolid;\n "
   "}\n ");

static const char *AmbDiff_frag =
  (VERSION
   "uniform vec3 lightDir;\n "
   "uniform float phongKa;\n "
   "uniform float phongKd;\n "
   "in vec4 color_frag;\n "
   "in vec3 norm_frag;\n "
   "out vec4 fcol;\n "
   "void main(void) {\n "
   "  float ldot = max(0, dot(lightDir, normalize(norm_frag)));\n "
   "  fcol = color_frag*(phongKa + phongKd*ldot);\n "
   "  fcol.a = color_frag.a;\n "
   "}\n");

static const char *AmbDiff2Side_frag =
  (VERSION
   "uniform vec3 lightDir;\n "
   "uniform float phongKa;\n "
   "uniform float phongKd;\n "
   "in vec4 color_frag;\n "
   "in vec3 norm_frag;\n "
   "out vec4 fcol;\n "
   "void main(void) {\n "
   // here's the two-sided lighting
   "  float ldot = max(dot(lightDir, -normalize(norm_frag)), dot(lightDir, normalize(norm_frag)));\n "
   "  fcol = color_frag*(phongKa + phongKd*ldot);\n "
   "  fcol.a = color_frag.a;\n "
   "}\n");

static GLchar * // HEY what's the right way to do this
strdupe(const char *str) {
  GLchar *ret;
  ret = new GLchar[strlen(str)+1];
  strcpy(ret, str);
  return ret;
}

static GLchar *
fileContents(const char *fname) {
  static const std::string me = "Hale::fileContents";
  GLchar *ret;
  FILE *file;
  long len;

  if (!fname) {
    throw std::runtime_error(me + ": got NULL fname");
  }
  if (!(file = fopen(fname, "r"))) {
    throw std::runtime_error(me + ": unable to open \"" + fname + "\": " + std::strerror(errno));
  }
  /* learn length of file contents */
  fseek(file, 0L, SEEK_END);
  len = ftell(file);
  fseek(file, 0L, SEEK_SET);
  ret = new GLchar[len+1];
  /* copy file into string */
  fread(ret, 1, len, file);
  ret[len] = '\0';
  fclose(file);
  return ret;
}

static GLint
shaderNew(GLint shtype, const GLchar *shaderSrc) {
  static const std::string me = "Hale::shaderNew";
  GLuint shaderId;
  GLint status;

  shaderId = glCreateShader(shtype);

  // if (debugging)
    fprintf(stderr,"# glCreateShader(%u) -> %u\n", shtype, shaderId);

  glErrorCheck(me, "glCreateShader");
  glShaderSource(shaderId, 1, &shaderSrc, NULL);
  if (debugging)
    printf("# glShaderSource(%u, 1):\n%s\n", shaderId, shaderSrc);
  glErrorCheck(me, "glShaderSource");
  glCompileShader(shaderId);
  if (debugging)
    printf("# glCompileShader(%u)\n", shaderId);
  glErrorCheck(me, "glCompileShader");
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
  if (debugging)
    printf("# glGetShaderiv(%u, GL_COMPILE_STATUS, &) -> %d (%s)\n", shaderId, status, GLBOOLSTR(status));
  glErrorCheck(me, "glGetShaderiv");
  /* HEY why does this sometimes set status to 32767 (not 0 or 1)? */

  if (GL_FALSE == status) {
    GLint logSize;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logSize);
    //printf("!%s: GL_INFO_LOG_LENGTH(%d) = %d\n", me.c_str(), shaderId, logSize);
    if (logSize) {
      char logMsg[logSize];
      glGetShaderInfoLog(shaderId, logSize, NULL, logMsg);
      glDeleteShader(shaderId);
      throw std::runtime_error(me + ": compiler error:\n" + logMsg);
    }
    return 0;
  }
  return shaderId;
}

Program::Program(preprogram prog) {
  static const std::string me = "Hale::Program::Program(prog)";

  if (preprogramAmbDiff == prog
      || preprogramAmbDiff2Side == prog) {
    _vertCode = strdupe(AmbDiff_vert);
  } else if (preprogramAmbDiffSolid == prog
             || preprogramAmbDiff2SideSolid == prog) {
    _vertCode = strdupe(AmbDiffSolid_vert);
  } else {
    throw std::runtime_error(me + ": prog " + std::to_string(prog)
                             + " not recognized");
  }
  if (preprogramAmbDiff2Side == prog
      || preprogramAmbDiff2SideSolid == prog) {
    _fragCode = strdupe(AmbDiff2Side_frag);
  } else {
    _fragCode = strdupe(AmbDiff_frag);
  }
  _vertId = 0;
  _fragId = 0;
  _progId = 0;
}

Program::Program(const char *vertFname, const char *fragFname) {

  _vertCode = fileContents(vertFname);
  _vertId = 0;
  _fragCode = fileContents(fragFname);
  _fragId = 0;
  _progId = 0;
}

Program::~Program() {

  delete _vertCode;
  delete _fragCode;
  // "A value of 0 for shader will be silently ignored."
  glDeleteShader(_vertId);
  glDeleteShader(_fragId);
  // "A value of 0 for program will be silently ignored."
  glDeleteProgram(_progId);
}

void
Program::compile() {
  static const std::string me="Hale::Program::compile";

  /* would need these if you can re-use a Hale::Program
     with different shader sources
  glDeleteShader(_vertId);
  glDeleteShader(_fragId);
  glDeleteProgram(_progId);
  */


  _vertId = shaderNew(GL_VERTEX_SHADER, _vertCode);
  _fragId = shaderNew(GL_FRAGMENT_SHADER, _fragCode);
  _progId = glCreateProgram();
  // if (debugging)d
    printf("# glCreateProgram() -> %d\n", _progId);
  glErrorCheck(me, "glCreateProgram");
  glAttachShader(_progId, _vertId);
  glErrorCheck(me, "glAttachShader(vertId " + std::to_string(_vertId) + ")");
  glAttachShader(_progId, _fragId);
  glErrorCheck(me, "glAttachShader(fragId " + std::to_string(_fragId) + ")");

  return;
}

void
Program::bindAttribute(GLuint idx, const GLchar *name) {
  static const std::string me="Hale::Program::bindAttribute";
  glBindAttribLocation(_progId, idx, name);
  if (debugging)
    printf("# glBindAttribLocation(%u, %u, %s)\n", _progId, idx, name);
  /* had hoped to use something like glGetVertexAttribiv to learn
     about the type of variable "name" (to enable the kind of type
     checking we support for uniforms), but those functions don't even
     take a program index ... */
  glErrorCheck(me, std::string("glBindAttribLocation(") + name + ")");
}

void
Program::link() {
  static const std::string me="Hale::Program::use";
  GLint status;

  glLinkProgram(_progId);
  if (debugging)
    printf("# glLinkProgram(%u);\n", _progId);
  glGetProgramiv(_progId, GL_LINK_STATUS, &status);
  if (debugging)
    printf("# glGetProgramiv(%u, GL_LINK_STATUS, &) -> %d (%s);\n", _progId, status, GLBOOLSTR(status));
  if (GL_FALSE == status) {
    GLint logSize;
    glGetProgramiv(_progId, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize) {
      char logMsg[logSize];
      glGetProgramInfoLog(_progId, logSize, NULL, logMsg);
      throw std::runtime_error(me + ": linking error: " + logMsg);
    }
    throw std::runtime_error("compilation failed");
  }

  if (debugging) {
    GLint idx;
#define GETALOC(str)                                                    \
    idx = glGetAttribLocation(_progId, str);                            \
    printf("# glGetAttribLocation(%d, \"" str "\") -> %d\n", _progId, idx);
    /* these two were added for initial debugging; ideally which ones
       are queried should depend on which attributes are used */
    GETALOC("positionVA");
    GETALOC("normalVA");
#undef GETALOC
  }

  /* learn uniform types (uniformType) and locations (uniformLocation) once,
     so that we can re-use them during use. Based on
     https://www.opengl.org/wiki/Program_Introspection */
  GLint uniN, uniI, uniSize;
  GLenum uniType;
  char uniName[512];
  uniformType.clear();
  uniformLocation.clear();
  glGetProgramiv(_progId, GL_ACTIVE_UNIFORMS, &uniN);
  if (debugging)
    printf("# glGetProgramiv(%d, GL_ACTIVE_UNIFORMS, &); -> %d; ... instrospection to get uniform types ...\n", _progId, uniN);
  glErrorCheck(me, "glGetProgramiv(GL_ACTIVE_UNIFORMS)");
  for (uniI=0; uniI<uniN; uniI++) {
    glGetActiveUniform(_progId, uniI, sizeof(uniName), NULL, &uniSize, &uniType, uniName);
    glErrorCheck(me, std::string("glGetActiveUniform(") + std::to_string(uniI) + ")");
    uniformType[uniName] = glEnumDesc[uniType];
    GLint uniLoc = glGetUniformLocation(_progId, uniName);
    glErrorCheck(me, std::string("glGetUniformLocation(") + uniName + ")");
    if (-1 == uniLoc) {
      throw std::runtime_error(me + ": \"" + uniName + "\" is not a known uniform name");
    }
    uniformLocation[uniName] = uniLoc;
    if (debugging)
      printf("!%s: uniform[%d]: \"%s\": type %s size %d location %d\n", me.c_str(),
             uniI, uniName, uniformType[uniName].enumStr.c_str(), uniSize,
             uniformLocation[uniName]);
  }

  return;
}

void
Program::use() const {
  static const std::string me="Hale::Program::use";

  if (Hale::_programCurrent == this) {
    /* we're already using this program; nothing to do here */
    /* But still set the program, in case it was changed elsewhere. */
    // return;
  }
  fprintf(stderr,"\nusing program %d\n", _progId);
  glUseProgram(_progId);
  if (debugging)
    printf("# glUseProgram(%u)\n", _progId);
  glErrorCheck(me, "glUseProgram(" + std::to_string(_progId) + ")");
  /* set global Program pointer to us */
  Hale::_programCurrent = this;
  /* re-set the sticky uniforms */
  stickyUniform();
  return;
}

GLuint
Program::progId() const {
  return _progId;
}

/* ------------------------------------------------------------ */

// HEY: what's right way to avoid copy+paste?
void uniform(std::string name, float vv, bool sticky) {
  if (_programCurrent) {
    _programCurrent->uniform(name, vv, sticky);
  }
}
void
Program::uniform(std::string name, float vv, bool sticky) const {
  static const std::string me="Program::uniform";
  auto iter = uniformType.find(name);
  if (uniformType.end() == iter) {
    throw std::runtime_error(me + ": \"" + name + "\" is not an active uniform");
  }
  glEnumItem ii = iter->second;
  if (GL_FLOAT != ii.enumVal) {
    throw std::runtime_error(me + ": \"" + name + "\" is a " + ii.glslStr + " but got a float");
  }
  glUniform1f(uniformLocation.at(name), vv);
  glErrorCheck(me, std::string("glUniform1f(") + name + ")");
  if (debugging) {
    printf("# glUniform1f(%u, %f);\n", uniformLocation.at(name), vv);
  }
  if (sticky) {
    stickyUniformFloat[name] = vv;
  }
}

// HEY: what's right way to avoid copy+paste?
void uniform(std::string name, glm::vec3 vv, bool sticky) {
  if (_programCurrent) {
    _programCurrent->uniform(name, vv, sticky);
  }
}
void
Program::uniform(std::string name, glm::vec3 vv, bool sticky) const {
  static const std::string me="Program::uniform";
  auto iter = uniformType.find(name);
  if (uniformType.end() == iter) {
    throw std::runtime_error(me + ": \"" + name + "\" is not an active uniform");
  }
  glEnumItem ii = iter->second;
  if (GL_FLOAT_VEC3 != ii.enumVal) {
    throw std::runtime_error(me + ": \"" + name + "\" is a " + ii.glslStr + " but got a vec3");
  }
  glUniform3fv(uniformLocation.at(name), 1, glm::value_ptr(vv));
  glErrorCheck(me, std::string("glUniform3fv(") + name + ")");
  if (debugging) {
    printf("# glUniform3fv(%u, 1, %s);\n", uniformLocation.at(name), glm::to_string(vv).c_str());
  }
  if (sticky) {
    stickyUniformVec3[name] = vv;
  }
}

// HEY: what's right way to avoid copy+paste?
void uniform(std::string name, glm::vec4 vv, bool sticky) {
  if (_programCurrent) {
    _programCurrent->uniform(name, vv, sticky);
  }
}
void
Program::uniform(std::string name, glm::vec4 vv, bool sticky) const {
  static const std::string me="Program::uniform";
  auto iter = uniformType.find(name);
  if (uniformType.end() == iter) {
    throw std::runtime_error(me + ": \"" + name + "\" is not an active uniform");
  }
  glEnumItem ii = iter->second;
  if (GL_FLOAT_VEC4 != ii.enumVal) {
    throw std::runtime_error(me + ": \"" + name + "\" is a " + ii.glslStr + " but got a vec4");
  }
  glUniform4fv(uniformLocation.at(name), 1, glm::value_ptr(vv));
  glErrorCheck(me, std::string("glUniform4fv(") + name + ")");
  if (debugging) {
    printf("# glUniform4fv(%u, 1, %s);\n", uniformLocation.at(name), glm::to_string(vv).c_str());
  }
  if (sticky) {
    stickyUniformVec4[name] = vv;
  }
}

// HEY: what's right way to avoid copy+paste?
void uniform(std::string name, glm::mat4 vv, bool sticky) {
  if (_programCurrent) {
    _programCurrent->uniform(name, vv, sticky);
  }
}
void
Program::uniform(std::string name, glm::mat4 vv, bool sticky) const {
  static const std::string me="Program::uniform";
  auto iter = uniformType.find(name);
  if (uniformType.end() == iter) {
    throw std::runtime_error(me + ": \"" + name + "\" is not an active uniform");
  }
  glEnumItem ii = iter->second;
  if (GL_FLOAT_MAT4 != ii.enumVal) {
    throw std::runtime_error(me + ": \"" + name + "\" is a " + ii.glslStr + " but got a mat4");
  }
  glUniformMatrix4fv(uniformLocation.at(name), 1, 0, glm::value_ptr(vv));
  glErrorCheck(me, std::string("glUniformMatrix4fv(") + name + ")");
  if (debugging) {
    printf("# glUniformMatrix4fv(%u, 1, 0, %s);\n", uniformLocation.at(name), glm::to_string(vv).c_str());
  }
  if (sticky) {
    stickyUniformMat4[name] = vv;
  }
}

/* ------------------------------------------------------------ */

/* HEY: there's currently no way to unstick a sticky uniform; perhaps there
   needs to be a new "unstick" argument to the uniform() calls, or the
   "sticky" argument can have 1 of 3 values: "true", "false", or "unstick" */
void stickyUniform(void) {
  for (auto si = stickyUniformFloat.begin(); si != stickyUniformFloat.end(); si++) {
    Hale::uniform(si->first, si->second);
  }
  for (auto si = stickyUniformVec3.begin(); si != stickyUniformVec3.end(); si++) {
    Hale::uniform(si->first, si->second);
  }
  for (auto si = stickyUniformVec4.begin(); si != stickyUniformVec4.end(); si++) {
    Hale::uniform(si->first, si->second);
  }
  for (auto si = stickyUniformMat4.begin(); si != stickyUniformMat4.end(); si++) {
    Hale::uniform(si->first, si->second);
  }
}

/* ------------------------------------------------------------ */

const Program *
ProgramLib(preprogram pp) {
  static const std::string me = "Hale::ProgramLib";

  if (!( preprogramUnknown < pp && pp < preprogramLast )) {
    throw std::runtime_error(me + ": prog " + std::to_string(pp)
                             + " not valid");
  }
  if (_program[pp]) {
    /* this preprogram has already been compiled; re-use */
    if (debugging)
      printf("!%s: re-using pre-compiled %d (GL %u)\n",
             me.c_str(), pp, _program[pp]->progId());
    return _program[pp];
  }
  Program *prog = new Program(pp);
  prog->compile();
  switch (pp) {
  case preprogramAmbDiff:
  case preprogramAmbDiff2Side:
    prog->bindAttribute(Hale::vertAttrIdxXYZW, "positionVA");
    prog->bindAttribute(Hale::vertAttrIdxRGBA, "colorVA");
    prog->bindAttribute(Hale::vertAttrIdxNorm, "normalVA"); // HEY Tex2, Tang
    break;
  case preprogramAmbDiffSolid:
  case preprogramAmbDiff2SideSolid:
    prog->bindAttribute(Hale::vertAttrIdxXYZW, "positionVA");
    prog->bindAttribute(Hale::vertAttrIdxNorm, "normalVA"); // HEY Tex2, Tang
    break;
  default:
    throw std::runtime_error(me + ": sorry, prog " + std::to_string(pp)
                             + " not implemented");
    break;
  }
  prog->link();
  _program[pp] = prog;
  return prog;
}

}
