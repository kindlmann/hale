# NOTE: Change these next three lines as needed
TEEM = /Users/gk/teem-install
HALE = /Users/gk/hale-install
GLM = /usr/local/include/glm/

OS := $(shell uname)
ifeq ($(OS), Darwin)
OS_LIBS = -framework CoreVideo  -framework Cocoa -framework OpenGL -framework IOKit
else
OS_LIBS = -lGL -lX11 -lXxf86vm -lXrandr -lpthread -lXi
endif

LIBS = -lglfw3 -lhale -lteem -lpng -lz

RPATH = -Wl,-rpath,$(TEEM)/lib
IPATH = -I$(HALE)/include -I$(TEEM)/include -I$(GLM)
LPATH = -L$(HALE)/lib -L$(TEEM)/lib -L$(GLM)

CC = g++ -std=c++11

all: iso foo

iso: iso.cpp
	$(CC) $(IPATH) iso.cpp -o iso $(RPATH) $(LPATH) $(LIBS) $(OS_LIBS)

foo: foo.cpp
	$(CC) $(IPATH) foo.cpp -o foo $(RPATH) $(LPATH) $(LIBS) $(OS_LIBS)

clean:
	rm -rf iso foo
