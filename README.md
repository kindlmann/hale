Hale
====

Support for minimalist scientific visualization.

The main purpose of this library is to be a bridge between Teem and OpenGL3,
and to simplify writing research software that uses Teem and OpenGL3.

This is easier with other additional libraries. GLFW makes it easy to
create a window with a modern OpenGL context and handle events.
Describing where things are in 3D is facilitated by a vector math
library; GLM is a path of least resistance, even with its extensive
reliance on template.  With time and experience, AntTweakBar or some
other GL-based widget toolkit, may be relied upon. The hope is that
Hale will gradually subsume code from GLK's "Deft" project (which
is dead because [FLTK2 is
dead](http://www.fltk.org/articles.php?L825)).

Thus to build Hale you'll need to first install:

1. Teem (checkout from source):

   svn co https://svn.code.sf.net/p/teem/code/teem/trunk teem
   and follow info at http://teem.sourceforge.net/build.html
2. GLFW3: http://www.glfw.org
3. GLM: http://glm.g-truc.net

Help is welcome for simplifying/automating how the build process finds
these dependencies.

Hale has been written by [Gordon
Kindlmann](http://people.cs.uchicago.edu/~glk/) based in part on code
examples written by [Roman Amici](https://github.com/roman-amici).
