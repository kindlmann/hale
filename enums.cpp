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

namespace Hale {

/* ------------------------------------------------------------- */

#define VIEWER_MODE_NUM 14
const char *
_viewerModeStr[VIEWER_MODE_NUM+1] = {
  "unknown mode",    /* (0) */
  "none",            /*  1 */
  "fov",             /*  2 */
  "depth-scale",     /*  3 */
  "translate-N",     /*  4 */
  "rotate-UV",       /*  5 */
  "rotate-U",        /*  6 */
  "rotate-V",        /*  7 */
  "rotate-N",        /*  8 */
  "vertigo",         /*  9 */
  "translate-UV",    /* 10 */
  "translate-U",     /* 11 */
  "translate-V",     /* 12 */
  "zoom",            /* 13 */
  "slider"           /* 14 */
};

airEnum
_viewerMode = {
  "viewer mode",
  VIEWER_MODE_NUM,
  _viewerModeStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
viewerMode = &_viewerMode;

/* ------------------------------------------------------------- */

#define FINISHING_STATUS_NUM 3

const char *
_finishingStatusStr[FINISHING_STATUS_NUM+1] = {
  "unknown finishing status", /* (0) */
  "not",                      /* 1 */
  "okay",                     /* 2 */
  "error"                     /* 3 */
};

airEnum
_finishingStatus = {
  "finishing status",
  FINISHING_STATUS_NUM,
  _finishingStatusStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
finishingStatus = &_finishingStatus;

/* ------------------------------------------------------------- */

#define VERT_ATTR_INDX_NUM 5
const char *
_vertAttrIndxStr[VERT_ATTR_INDX_NUM+1] = {
  "unknown vert attr index", /* (-1) */
  "xyzw",                    /*  0 */
  "rgba"                     /*  1 */
  "norm",                    /*  2 */
  "tex2",                    /*  3 */
  "tang"                     /*  4 */
};

airEnum
_vertAttrIndex = {
  "vertex attribute index",
  VERT_ATTR_INDX_NUM,
  _vertAttrIndxStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
vertAttrIndex = &_vertAttrIndex;

} // namespace Hale
