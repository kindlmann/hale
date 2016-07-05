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

void
Polydata::_buffer(bool newaddr) {
  static const char me[]="Hale::Polydata::_buffer";
  const limnPolyData *lpd = this->lpld();
  unsigned int ibits = limnPolyDataInfoBitFlag(lpd);

  if (debugging)
    printf("!%s(%s): ____________________________________________ \n", me, _name.c_str());
  glBindVertexArray(_vao);
  if (debugging)
    printf("# glBindVertexArray(%u);\n", _vao);
  glBindBuffer(GL_ARRAY_BUFFER, _buff[_buffIdx[vertAttrIdxXYZW]]);
  if (debugging)
    printf("# glBindBuffer(GL_ARRAY_BUFFER, %u);\n", _buff[_buffIdx[vertAttrIdxXYZW]]);
  if (newaddr) {
    glBufferData(GL_ARRAY_BUFFER, lpd->xyzwNum*sizeof(float)*4, lpd->xyzw, GL_DYNAMIC_DRAW);
    if (debugging)
      printf("# glBufferData(GL_ARRAY_BUFFER, %u, lpd->xyzw, GL_DYNAMIC_DRAW);\n", (unsigned int)(lpd->xyzwNum*sizeof(float)*4));
  } else {
    glBufferSubData(GL_ARRAY_BUFFER, 0, lpd->xyzwNum*sizeof(float)*4, lpd->xyzw);
  }
  glVertexAttribPointer(Hale::vertAttrIdxXYZW, 4, GL_FLOAT, GL_FALSE, 0, 0);
  if (debugging)
    printf("# glVertexAttribPointer(%u, 4, GL_FLOAT, GL_FALSE, 0, 0);\n", Hale::vertAttrIdxXYZW);

  if (ibits & (1 << limnPolyDataInfoNorm)) {
    glBindBuffer(GL_ARRAY_BUFFER, _buff[_buffIdx[vertAttrIdxNorm]]);
    if (debugging)
      printf("# glBindBuffer(GL_ARRAY_BUFFER, %u);\n", _buff[_buffIdx[vertAttrIdxNorm]]);
    if (newaddr) {
      glBufferData(GL_ARRAY_BUFFER, lpd->normNum*sizeof(float)*3, lpd->norm, GL_DYNAMIC_DRAW);
      if (debugging)
        printf("# glBufferData(GL_ARRAY_BUFFER, %u, lpd->norm, GL_DYNAMIC_DRAW);\n", (unsigned int)(lpd->normNum*sizeof(float)*3));
    } else {
      glBufferSubData(GL_ARRAY_BUFFER, 0, lpd->normNum*sizeof(float)*3,lpd->norm);
    }
    glVertexAttribPointer(Hale::vertAttrIdxNorm, 3, GL_FLOAT,GL_FALSE, 0, 0);
    if (debugging)
      printf("# glVertexAttribPointer(%u, 3, GL_FLOAT, GL_FALSE, 0, 0);\n", Hale::vertAttrIdxNorm);
  }

  if (ibits & (1 << limnPolyDataInfoRGBA)) {
    glBindBuffer(GL_ARRAY_BUFFER, _buff[_buffIdx[vertAttrIdxRGBA]]);
    if (debugging)
      printf("# glBindBuffer(GL_ARRAY_BUFFER, %u);\n", _buff[_buffIdx[vertAttrIdxRGBA]]);
    if (newaddr) {
      glBufferData(GL_ARRAY_BUFFER, lpd->rgbaNum*sizeof(char)*4, lpd->rgba, GL_DYNAMIC_DRAW);
      if (debugging)
        printf("# glBufferData(GL_ARRAY_BUFFER, %u, lpd->rgba, GL_DYNAMIC_DRAW);\n", (unsigned int)(lpd->rgbaNum*sizeof(char)*4));
    } else {
      glBufferSubData(GL_ARRAY_BUFFER, 0, lpd->rgbaNum*sizeof(char)*4,lpd->rgba);
    }
    glVertexAttribPointer(Hale::vertAttrIdxRGBA, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);
    if (debugging)
      printf("# glVertexAttribPointer(%u, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, 0);\n", Hale::vertAttrIdxRGBA);
  }

  /* HEY: tang and tex2 */


  if (!_elms || newaddr) {
    if (_elms) {
      glDeleteBuffers(1, &_elms);
      if (debugging)
        printf("# glDeleteBuffers(1, %u);\n", _elms);
    }
    glGenBuffers(1, &_elms);
    if (debugging)
      printf("# glGenBuffers(1, &); -> %u\n", _elms);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _elms);
    if (debugging)
      printf("# glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, %u);\n", _elms);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 lpd->indxNum * sizeof(unsigned int),
                 lpd->indx, GL_DYNAMIC_DRAW);
    if (debugging)
      printf("# glBufferData(GL_ELEMENT_ARRAY_BUFFER, %u, lpd->indx, GL_DYNAMIC_DRAW);\n", (unsigned int)(lpd->indxNum * sizeof(unsigned int)));
  }
  return;
}

void Polydata::model(glm::mat4 mat) { _model = mat; }
glm::mat4 Polydata::model() const { return _model; }

void
Polydata::_init(std::string name) {
  static const char me[]="Hale::Polydata::_init";

  if (name.empty()) {
    std::ostringstream address;
    address << (void const *)this;
    _name = address.str();
  } else {
    _name = name;
  }
  if (debugging)
    printf("!%s(%s): ____________________________________________ \n", me, _name.c_str());
  _colorSolid = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  _model = glm::mat4(1.0f);

  const limnPolyData *lpld = this->lpld();
  unsigned int aa, ibits = limnPolyDataInfoBitFlag(lpld);
  _buffNum = 1 + airBitsSet(ibits);   /* lpld->xyzw is always set */
  if (debugging)
    printf("!%s: %p|%p %u buffers to set\n", me, _lpld, _lpldOwn, _buffNum);
  _buff = AIR_CALLOC(_buffNum, GLuint);
  if (debugging)
    printf("!%s: _buff = %p\n", me, _buff);
  glGenBuffers(_buffNum, _buff);
  if (debugging) {
    printf("# glGenBuffers(%u, &); -> %u", _buffNum, _buff[0]);
    for (int bi=1; bi<_buffNum; bi++) {
      printf(" %u", _buff[bi]);
    }
    printf("\n");
  }
  glGenVertexArrays(1, &_vao);
  if (debugging)
    printf("# glGenVertexArrays(1, &); -> %u\n", _vao);
  glBindVertexArray(_vao);
  if (debugging)
    printf("# glBindVertexArray(%u);\n", _vao);

  aa = 0;
  glEnableVertexAttribArray(Hale::vertAttrIdxXYZW);
  if (debugging)
    printf("# glEnableVertexAttribArray(%u);\n", Hale::vertAttrIdxXYZW);
  _buffIdx[Hale::vertAttrIdxXYZW] = aa++;
  for (int ii=limnPolyDataInfoUnknown+1; ii<limnPolyDataInfoLast; ii++) {
    /* HEY assumption of limnPolyDataInfo, Hale::vertAttrIdx mirroring */
    int hva = ii - limnPolyDataInfoRGBA + Hale::vertAttrIdxRGBA;
    if (ibits & (1 << ii)) {
      glEnableVertexAttribArray(hva);
      if (debugging)
        printf("# glEnableVertexAttribArray(%u);\n", hva);
      _buffIdx[hva] = aa++;
    } else {
      _buffIdx[hva] = -1;
    }
  }
  _elms = 0;
  memcpy(&_lpldCopy, lpld, sizeof(limnPolyData));
  _buffer(true);
  return;
}

Polydata::Polydata(const limnPolyData *poly, const Program *prog,
                   std::string name) {

  _lpld = poly;
  _lpldOwn = NULL;
  _program = prog;
  _init(name);
}

Polydata::Polydata(limnPolyData *poly, bool own, const Program *prog,
                   std::string name) {

  if (own) {
    _lpld = NULL;
    _lpldOwn = poly;
  } else {
    _lpld = poly;
    _lpldOwn = NULL;
  }
  _program = prog;
  _init(name);
}

void Polydata::rebuffer() {
  static const char me[]="Polydata::rebuffer";
  const limnPolyData *lpld = this->lpld();
  unsigned int cbits = limnPolyDataInfoBitFlag(&_lpldCopy),
    ibits = limnPolyDataInfoBitFlag(lpld);
  bool newaddr;

  if (cbits != ibits) {
    newaddr = true;
  } else {
    newaddr = (_lpldCopy.xyzw != lpld->xyzw
               || _lpldCopy.xyzwNum != lpld->xyzwNum
               || _lpldCopy.rgba != lpld->rgba
               || _lpldCopy.rgbaNum != lpld->rgbaNum
               || _lpldCopy.norm != lpld->norm
               || _lpldCopy.normNum != lpld->normNum
               || _lpldCopy.tex2 != lpld->tex2
               || _lpldCopy.tex2Num != lpld->tex2Num
               || _lpldCopy.tang != lpld->tang
               || _lpldCopy.tangNum != lpld->tangNum);
  }
  if (debugging)
    printf("!%s: calling _buffer(newaddr=%s)\n", me, newaddr ? "true" : "false");
  _buffer(newaddr);
  memcpy(&_lpldCopy, lpld, sizeof(limnPolyData));
  return;
}

Polydata::~Polydata() {
  // static const char me[]="Hale::Polydata::~Polydata";

  if (_lpldOwn) {
    limnPolyDataNix(_lpldOwn);
  }
  glDeleteVertexArrays(1, &_vao);
  glDeleteBuffers(1, &_elms);
  glDeleteBuffers(_buffNum, _buff);
  free(_buff);
}

void Polydata::colorSolid(float rr, float gg, float bb) {
  _colorSolid = glm::vec4(rr, gg, bb, 1.0f);
}
void Polydata::colorSolid(glm::vec3 rgb) {
  _colorSolid = glm::vec4(rgb, 1.0f);
}
void Polydata::colorSolid(glm::vec4 rgba) {
  _colorSolid = rgba;
}
glm::vec4 Polydata::colorSolid() const {
  return _colorSolid;
}

void Polydata::program(const Program *prog) {
  static const std::string me="Hale::Polydata::program";
  if (!prog) {
    throw std::runtime_error(me + ": got NULL program");
  }
  _program = prog;
}
const Program *Polydata::program() const {
  return _program;
}

void Polydata::bounds(glm::vec3& finalmin, glm::vec3& finalmax) const {
  const limnPolyData *lpd = lpld();
  glm::vec4 wmin, wmax, wpos;
  const float *_wpos = lpd->xyzw;
  wmin = wmax = glm::vec4(_wpos[0], _wpos[1], _wpos[2], _wpos[3]);
  for (unsigned int ii=1; ii<lpd->xyzwNum; ii++) {
    _wpos += 4;
    wpos = _model*glm::vec4(_wpos[0], _wpos[1], _wpos[2], _wpos[3]);
    wpos /= wpos[3];
    wmin = glm::min(wmin, wpos);
    wmax = glm::max(wmax, wpos);
  }
  finalmin = glm::vec3(wmin);
  finalmax = glm::vec3(wmax);
}

void Polydata::name(std::string nm) {
  _name = nm;
}
std::string Polydata::name() const {
  return _name;
}

void
Polydata::draw() const {
  static const char me[]="Hale::Polydata::draw";
  if (debugging)
    printf("!%s(%s): ____________________________________________ \n", me, _name.c_str());
  _program->use();
  const limnPolyData *lpld = this->lpld();
  int ibits = limnPolyDataInfoBitFlag(lpld);
  if (!(ibits & (1 << limnPolyDataInfoRGBA))) {
    _program->uniform("colorSolid", _colorSolid);
  }
  _program->uniform("modelMat", _model);
  /* would be nice to call this only if the values have changed;
     but the Program pointer is to a const Program, so we can't easily
     make this into a stateful/conditional call to uniform() */
  _program->uniform("phongKa", 0.2);
  _program->uniform("phongKd", 0.8);
  if (debugging)
    printf("!%s: (done setting uniforms)\n", me);

  glBindVertexArray(_vao);
  if (debugging)
    printf("# glBindVertexArray(%u);\n", _vao);

  int offset = 0;
  for (unsigned int ii=0; ii<lpld->primNum; ii++) {
    glDrawElements(Hale::limnToGLPrim(lpld->type[ii]),
                   lpld->icnt[ii],
                   GL_UNSIGNED_INT, ((void*) 0));
    if (debugging)
      printf("# glDrawElements(%u, %u, GL_UNSIGNED_INT, 0);\n", Hale::limnToGLPrim(lpld->type[ii]), lpld->icnt[ii]);
    Hale::glErrorCheck(me, "glDrawElements(prim " + std::to_string(ii) + ")");
    offset += lpld->icnt[ii];
  }
  return;
}


} // namespace Hale
