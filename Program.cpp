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
#include "privateHale.h"

namespace Hale {

GLchar *
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

GLint
shaderNew(GLint shtype, const GLchar *shaderSrc) {
  static const std::string me = "Hale::shaderNew";
  GLuint shaderId;
  GLint status;

  shaderId = glCreateShader(shtype);
  glErrorCheck(me, "glCreateShader");
  glShaderSource(shaderId, 1, &shaderSrc, NULL);
  glCompileShader(shaderId);
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
  if (GL_FALSE == status) {
    GLint logSize;
    glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize) {
      char *logMsg = new char[logSize];
      glGetShaderInfoLog(shaderId, logSize, NULL, logMsg);
      glDeleteShader(shaderId);
      delete logMsg;
      throw std::runtime_error(me + ": compiler error:\n" + logMsg);
    }
    return 0;
  }
  return shaderId;
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

  _vertId = shaderNew(GL_VERTEX_SHADER, _vertCode);
  _fragId = shaderNew(GL_FRAGMENT_SHADER, _fragCode);
  _progId = glCreateProgram();
  glAttachShader(_progId, _vertId);
  glErrorCheck(me, "glAttachShader(vertId " + std::to_string(_vertId) + ")");
  glAttachShader(_progId, _fragId);
  glErrorCheck(me, "glAttachShader(fragId " + std::to_string(_fragId) + ")");

  return;
}

void
Program::bind(GLuint idx, const GLchar *name) {
  static const std::string me="Hale::Program::bind";
  glBindAttribLocation(_progId, idx, name);
  glErrorCheck(me, "glBindAttribLocation");
}

GLint
Program::uniformLocation(const char *name) {
  static const std::string me="Hale::Program::uniformLocation";
  GLint id = glGetUniformLocation(_progId, name);
  glErrorCheck(me, std::string("glGetUniformLocation(") + name + ")");
  return id;
}

void
Program::link() {
  static const char me[]="Program::link";
  GLint status;

  glLinkProgram(_progId);
  glGetProgramiv(_progId, GL_LINK_STATUS, &status);
  if (GL_FALSE == status) {
    GLint logSize;
    char *logMsg;
    glGetProgramiv(_progId, GL_INFO_LOG_LENGTH, &logSize);
    if (logSize) {
      logMsg = (char*)malloc(logSize);
      glGetProgramInfoLog(_progId, logSize, NULL, logMsg);
      fprintf(stderr, "%s: linking error:\n%s", me, logMsg);
      free(logMsg);
    }
    throw std::runtime_error("compilation failed");
  }

  return;
}

void
Program::use() {
  static const std::string me="Hale::Program::use";
  glUseProgram(_progId);
  glErrorCheck(me, "glUseProgram(" + std::to_string(_progId) + ")");
}

}
