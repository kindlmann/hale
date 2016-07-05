#
# Hale: support for minimalist scientific visualization
# Copyright (C) 2014, 2015  University of Chicago
#
# This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software. Permission is granted to anyone to
# use this software for any purpose, including commercial applications, and
# to alter it and redistribute it freely, subject to the following
# restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software in a
# product, an acknowledgment in the product documentation would be
# appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source distribution.
#

LIB = libhale.a

TEEM = $(HOME)/teem-install
GLM = /usr/local/include/glm/
NANOGUI = /home/ashwin/repo/nanogui/

IPATH = -I$(TEEM)/include -I$(GLM) -I$(NANOGUI)/include -I$(NANOGUI)/ext/eigen -I$(NANOGUI)/ext/nanovg/src

CXX = g++
AR = ar crs
CXXFLAGS = -Wall -std=c++11 -g

HDR = Hale.h
PRIV_HDR = privateHale.h
SRCS = GUI.cpp enums.cpp globals.cpp utils.cpp Camera.cpp Viewer.cpp Program.cpp \
	Polydata.cpp Scene.cpp
OBJS = $(SRCS:.cpp=.o)

DEST = $(HOME)/hale-install
DEST_LIB = $(DEST)/lib/$(LIB)
DEST_HDR = $(DEST)/include/$(HDR)

all: $(LIB)

$(OBJS): $(HDR) $(PRIV_HDR)

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(IPATH) -o $@ $<

$(LIB): $(OBJS)
	$(AR) $(LIB) $(OBJS)

$(DEST_LIB): $(LIB)
	cp $(LIB) $(DEST_LIB)

$(DEST_HDR): $(LIB) $(HDR) $(DEST_LIB)
	cp $(HDR) $(DEST_HDR)

install: $(DEST_LIB) $(DEST_HDR)

clean:
	rm -f $(OBJS) $(LIB)

clobber: clean
	rm -f $(DEST_LIB) $(DEST_HDR)
