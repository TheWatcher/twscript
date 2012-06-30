###############################################################################
##  Makefile-gcc
##
##  This file is part of Public Scripts
##  Copyright (C) 2005-2011 Tom N Harris <telliamed@whoopdedo.org>
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program. If not, see <http://www.gnu.org/licenses/>.
##
###############################################################################


.SUFFIXES:
.SUFFIXES: .o .cpp .rc
.PRECIOUS: %.o

GAME = 2

srcdir = .
bindir = ./obj

LGDIR = ../lg
SCRLIBDIR = ../ScriptLib
DH2DIR = ../DH2
DH2LIB = -ldh2

CC = gcc
CXX = g++
AR = ar
LD = g++
DLLTOOL = dlltool
RC = windres

DEFINES = -DWINVER=0x0400 -D_WIN32_WINNT=0x0400 -DWIN32_LEAN_AND_MEAN
GAME2 = -D_DARKGAME=2

ifdef DEBUG
DEFINES := $(DEFINES) -DDEBUG
CXXDEBUG = -g -O0
LDDEBUG = -g
LGLIB = -llg-d
SCR2LIB = -lScript2-d
else
DEFINES := $(DEFINES) -DNDEBUG
CXXDEBUG = -O2
LDDEBUG =
LGLIB = -llg
SCR2LIB = -lScript2
endif

ARFLAGS = rc
LDFLAGS = -mwindows -mdll -Wl,--enable-auto-image-base
LIBDIRS = -L. -L$(LGDIR) -L$(SCRLIBDIR) -L$(DH2DIR)
LIBS = $(DH2LIB) $(LGLIB) -luuid
INCLUDES = -I. -I$(srcdir) -I$(LGDIR) -I$(SCRLIBDIR) -I$(DH2DIR)
# If you care for this... # -Wno-unused-variable
# A lot of the callbacks have unused parameters, so I turn that off.
CXXFLAGS = -W -Wall -masm=intel -std=gnu++0x
DLLFLAGS = --add-underscore

OSM_OBJS = $(bindir)/ScriptModule.o $(bindir)/Script.o $(bindir)/Allocator.o $(bindir)/exports.o
BASE_OBJS = $(bindir)/MsgHandlerArray.o $(bindir)/BaseTrap.o $(bindir)/BaseScript.o
SCR_OBJS = $(bindir)/TWScripts.o
MISC_OBJS = $(bindir)/ScriptDef.o $(bindir)/utils.o
RES_OBJS = $(bindir)/twscript_res.o

$(bindir)/%.o: $(srcdir)/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) $(GAME2) $(INCLUDES) -o $@ -c $<

$(bindir)/%_res.o: $(srcdir)/%.rc
	$(RC) $(DEFINES) $(GAME2) -o $@ -i $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) $(GAME2) $(INCLUDES) -o $@ -c $<

%_res.o: %.rc
	$(RC) $(DEFINES) $(GAME2) -o $@ -i $<

%.osm: %.o $(OSM_OBJS)
	$(LD) $(LDFLAGS) $(LDDEBUG) $(LIBDIRS) -o $@ script.def $< $(OSM_OBJS) $(SCR2LIB) $(LIBS)


all: $(bindir) twscript.osm

clean:
	$(RM) $(bindir)/* twscript.osm

$(bindir):
	mkdir -p $@

#.INTERMEDIATE: exports.o

$(bindir)/exports.o: $(bindir)/ScriptModule.o
	$(DLLTOOL) $(DLLFLAGS) --dllname script.osm --output-exp $@ $^

$(bindir)/ScriptModule.o: ScriptModule.cpp ScriptModule.h Allocator.h
$(bindir)/Script.o: Script.cpp Script.h
$(bindir)/Allocator.o: Allocator.cpp Allocator.h

$(bindir)/BaseScript.o: BaseScript.cpp BaseScript.h Script.h ScriptModule.h MsgHandlerArray.h
$(bindir)/BaseTrap.o: BaseTrap.cpp BaseTrap.h BaseScript.h Script.h
$(bindir)/TWScripts.o: TWScripts.cpp TWScripts.h BaseTrap.h BaseScript.h Script.h
$(bindir)/ScriptDef.o: ScriptDef.cpp TWScripts.h BaseTrap.h BaseScript.h ScriptModule.h genscripts.h
$(bindir)/twscript_res.o: twscript.rc version.rc

twscript.osm: $(SCR_OBJS) $(BASE_OBJS) $(OSM_OBJS) $(MISC_OBJS) $(RES_OBJS)
	$(LD) $(LDFLAGS) -Wl,--image-base=0x11200000 $(LDDEBUG) $(LIBDIRS) -o $@ script.def $^ $(SCR2LIB) $(LIBS)
