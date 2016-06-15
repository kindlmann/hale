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

bool finishing = false;
int debugging = 0;

const Program *_programCurrent = NULL;

const Program *_program[preprogramLast] = {
  NULL, /* preprogramUnknown,            0 */
  NULL, /* preprogramAmbDiff,            1 */
  NULL, /* preprogramAmbDiffSolid,       2 */
  NULL, /* preprogramAmbDiff2Side,       3 */
  NULL, /* preprogramAmbDiff2SideSolid,  4 */
};

} // namespace Hale
