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

/* ------------------------------------------------------------- */

const char *
_viewerModeStr[HALE_VIEWER_MODE_MAX+1] = {
  "unknown mode",
  "none",
  "fov",
  "depth",
  "rotate-UV",
  "rotate-U",
  "rotate-V",
  "rotate-N",
  "dolly",
  "translate-UV",
  "translate-U",
  "translate-V",
  "translate-N"
};

airEnum
_viewerMode = {
  "viewer mode",
  HALE_VIEWER_MODE_MAX,
  _viewerModeStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
viewerMode = &_viewerMode;

/* ------------------------------------------------------------- */

const char *
_finishingStatusStr[HALE_VIEWER_MODE_MAX+1] = {
  "unknown finishing status",
  "not",
  "okay",
  "error"
};

airEnum
_finishingStatus = {
  "finishing status",
  HALE_FINISHING_STATUS_MAX,
  _finishingStatusStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};

airEnum *
finishingStatus = &_finishingStatus;

} // namespace Hale
