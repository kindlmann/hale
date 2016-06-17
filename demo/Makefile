# Demo makefile

# NOTE: Change these next three lines as needed
TEEM = ~/dev/teem/teem-install
HALE = ~/dev/hale
GLM = /usr/include/glm/
CEGUI = /usr/local/include/
CEGUILIB = /usr/local/lib/cegui-0.8/

OS := $(shell uname)
ifeq ($(OS), Darwin)
OS_LIBS = -framework CoreVideo  -framework Cocoa -framework OpenGL -framework IOKit
else
OS_LIBS = -lGL -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor -lrt -lm -lpthread
endif

# TODO: clean this up a little bit with LPATH

LIBS = $(CEGUILIB)libCEGUIFreeImageImageCodec.so -lGL -lglfw3 $(HALE)/libhale.a -lGLEW -lteem -lpng -lz $(CEGUILIB)libCEGUIBase-0.so.2 $(CEGUILIB)libCEGUICommonDialogs-0.so.2 $(CEGUILIB)libCEGUICoreWindowRendererSet.so $(CEGUILIB)libCEGUIExpatParser.so  $(CEGUILIB)libCEGUIOpenGLRenderer-0.so.2.4.2


RPATH = -Wl,-rpath,$(TEEM)/lib
IPATH = -I$(HALE) -I$(TEEM)/include -I$(GLM) -I$(CEGUI)
LPATH = -L$(HALE)/lib -L$(TEEM)/lib -L$(GLM) -L$(CEGUILIB)

CC = g++ -g -O0 -std=c++11

all: iso foo atg

# We depend on the installed hale library because in practice that's
# often what has actually changed when the demo programs need to be rebuilt.
# Happily the library doesn't get included in the $< automatic variable.
%: %.cpp $(HALE)/libhale.*
	$(CC) $(IPATH) $< -o $@ $(RPATH) $(LPATH) $(LIBS) $(OS_LIBS) -ldl

clean:
	rm -rf iso foo


# -lGLEW -lglfw3 -lGL -lX11 -lXi -lXrandr -lXxf86vm -lXinerama -lXcursor -lrt -lm -pthread