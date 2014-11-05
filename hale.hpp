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

#ifndef HALE_HAS_BEEN_INCLUDED
#define HALE_HAS_BEEN_INCLUDED

#ifdef __APPLE__
#  include <OpenGL/gl3.h>
#else
#  include <GL/gl3.h>
#endif

#include <teem/meet.h>

/* any glm includes? */

namespace hale {

/*
** viewerMode* enum
**
** The GUI modes that the viewer can be in. In Fov and Depth (distance from
** near to far adjusted), the eye and look-at point are both fixed. The eye
** moves around a fixed look-at point in the Rotate and Dolly modes. The eye
** and look-at points move together in the Translate modes.
*/
enum {
  viewerModeUnknown,     /*  0 */
  viewerModeFov,         /*  1: standard "zoom" */
  viewerModeDepth,       /*  2: adjust distance between near and far planes */
  viewerModeRotateUV,    /*  3: usual rotate (around look-at point) */
  viewerModeRotateU,     /*  4: rotate around horizontal axis */
  viewerModeRotateV,     /*  5: rotate around vertical axis */
  viewerModeRotateN,     /*  6: in-plane rotate (around at point) */
  viewerModeDolly,       /*  7: fix at, move from, adjust fov: the effect is
                                direct control on the amount of perspective */
  viewerModeTranslateUV, /*  8: usual translate */
  viewerModeTranslateU,  /*  9: translate only horizontal */
  viewerModeTranslateV,  /* 10: translate only vertical */
  viewerModeTranslateN,  /* 11: translate from *and* at along view direction */
  viewerModeLast
};
#define HALE_VIEWER_MODE_MAX  11


/* enums.cpp */
extern airEnum *viewerMode;

/* utils.cpp */
extern GLuint limnToGLPrim(int type);


} /* namespace hale */

#endif /* HALE_HAS_BEEN_INCLUDED */
