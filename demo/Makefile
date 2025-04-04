# NOTE: Change these next lines as needed, or override via, e.g.
# "GLM=/other/path make"
TEEM ?= ../../teem-install
HALE ?= ../../hale-install
GLM ?= /opt/homebrew/Cellar/glm/1.0.1
GLFW ?= /opt/homebrew/Cellar/glfw/3.4
PNG ?= /opt/homebrew/Cellar/libpng/1.6.47

OS := $(shell uname)
ifeq ($(OS), Darwin)
# OS_LIBS = -framework CoreVideo  -framework Cocoa -framework OpenGL -framework IOKit
OS_LIBS = -framework Cocoa -framework OpenGL -framework IOKit
else
OS_LIBS = -lGL -lX11 -lXxf86vm -lXrandr -lpthread -lXi
endif

LIBS = -lglfw -lhale -lteem -lpng -lz

RPATH = -Wl,-rpath,$(TEEM)/lib
IPATH = -I$(HALE)/include -I$(TEEM)/include -I$(GLFW)/include -I$(GLM)/include
LPATH = -L$(HALE)/lib -L$(TEEM)/lib -L$(GLFW)/lib -L$(GLM)/lib -L$(PNG)/lib

CC = g++ -std=c++11 -g

all: iso simple

# We depend on the installed hale library because in practice that's
# often what has actually changed when the demo programs need to be rebuilt.
# Happily the library doesn't get included in the $< automatic variable.
%: %.cpp $(HALE)/lib/libhale.*
	$(CC) $(IPATH) $< -o $@ $(RPATH) $(LPATH) $(LIBS) $(OS_LIBS)

clean:
	rm -rf iso iso.dSYM simple simple.dSYM
