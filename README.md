hale
====

Support for minimalist scientific visualization.

The main purpose of this library is to be a bridge between Teem and OpenGL3,
and to simplify writing programs that use Teem and OpenGL3.

We also want to make it easy to create a window that views a 3D scene,
so we rely on GLFW3 for window creation and event handling.

Describing where things are in 3D is facilitated by a vector math
library, so we go with GLM as a path of least resistance, even though
the templates-gone-wild nature of GLM may cause this to be rethought.
With time and experience we may also include dependencies on
AntTweakBar.

Hale will gradually subsume code from GLK's "Deft" project (which is
dead because FLTK2 is dead).
