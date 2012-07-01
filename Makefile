## @file
# Makefile for TWScript, modified from the Makefile included with Tellliamed's
# Publis Scripts package.
#
# @author Chris Page <chris@starforge.co.uk>
# @author Tom N Harris <telliamed@whoopdedo.org>
#

# Original header comment follows:
#
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

# Update these with the name of your script file, and the output .osm
MYSCRIPT = TWScript
MYOSM    = twscript.osm

srcdir = .
bindir = ./obj

GAME = 2
PUBDIR = ./pubscript
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

ARFLAGS  = rc
LDFLAGS  = -mwindows -mdll -Wl,--enable-auto-image-base
LIBDIRS  = -L. -L$(LGDIR) -L$(SCRLIBDIR) -L$(DH2DIR)
LIBS     = $(DH2LIB) $(LGLIB) -luuid
INCLUDES = -I. -I$(srcdir) -I$(LGDIR) -I$(SCRLIBDIR) -I$(DH2DIR) -I$(PUBDIR)
CXXFLAGS = -W -Wall -masm=intel -std=gnu++0x
DLLFLAGS = --add-underscore

# Public scripts objects
OSM_OBJS  = $(PUBDIR)/ScriptModule.o $(PUBDIR)/Script.o $(PUBDIR)/Allocator.o $(PUBDIR)/exports.o
BASE_OBJS = $(PUBDIR)/MsgHandlerArray.o $(PUBDIR)/BaseTrap.o $(PUBDIR)/BaseScript.o
MISC_OBJS = $(bindir)/ScriptDef.o $(PUBDIR)/utils.o

# Custom script objects
SCR_OBJS  = $(bindir)/$(MYSCRIPT).o
RES_OBJS  = $(bindir)/$(MYSCRIPT)_res.o

$(bindir)/%.o: $(srcdir)/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) $(GAME2) $(INCLUDES) -o $@ -c $<

$(bindir)/%_res.o: $(srcdir)/%.rc
	$(RC) $(DEFINES) $(GAME2) -o $@ -i $<

$(PUBDIR)/%.o: $(PUBDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) $(GAME2) $(INCLUDES) -o $@ -c $<

$(PUBDIR)/%_res.o: $(PUBDIR)/%.rc
	$(RC) $(DEFINES) $(GAME2) -o $@ -i $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CXXDEBUG) $(DEFINES) $(GAME2) $(INCLUDES) -o $@ -c $<

%_res.o: %.rc
	$(RC) $(DEFINES) $(GAME2) -o $@ -i $<

%.osm: %.o $(OSM_OBJS)
	$(LD) $(LDFLAGS) $(LDDEBUG) $(LIBDIRS) -o $@ script.def $< $(OSM_OBJS) $(SCR2LIB) $(LIBS)

all: $(bindir) $(MYOSM)

clean:
	$(RM) $(bindir)/* $(PUBDIR)/*.o $(MYOSM)

$(bindir):
	mkdir -p $@

$(PUBDIR)/exports.o: $(PUBDIR)/ScriptModule.o
	$(DLLTOOL) $(DLLFLAGS) --dllname script.osm --output-exp $@ $^

$(PUBDIR)/ScriptModule.o: $(PUBDIR)/ScriptModule.cpp $(PUBDIR)/ScriptModule.h $(PUBDIR)/Allocator.h
$(PUBDIR)/Script.o: $(PUBDIR)/Script.cpp $(PUBDIR)/Script.h
$(PUBDIR)/Allocator.o: $(PUBDIR)/Allocator.cpp $(PUBDIR)/Allocator.h

$(PUBDIR)/BaseScript.o: $(PUBDIR)/BaseScript.cpp $(PUBDIR)/BaseScript.h $(PUBDIR)/Script.h $(PUBDIR)/ScriptModule.h $(PUBDIR)/MsgHandlerArray.h
$(PUBDIR)/BaseTrap.o: $(PUBDIR)/BaseTrap.cpp $(PUBDIR)/BaseTrap.h $(PUBDIR)/BaseScript.h $(PUBDIR)/Script.h
$(bindir)/ScriptDef.o: ScriptDef.cpp $(MYSCRIPT).h $(PUBDIR)/BaseTrap.h $(PUBDIR)/BaseScript.h $(PUBDIR)/ScriptModule.h $(PUBDIR)/genscripts.h
$(bindir)/$(MYSCRIPT)s.o: $(MYSCRIPT).cpp $(MYSCRIPT).h $(PUBDIR)/BaseTrap.h $(PUBDIR)/BaseScript.h $(PUBDIR)/Script.h
$(bindir)/$(MYSCRIPT)_res.o: $(MYSCRIPT).rc $(PUBDIR)/version.rc

$(MYOSM): $(SCR_OBJS) $(BASE_OBJS) $(OSM_OBJS) $(MISC_OBJS) $(RES_OBJS)
	$(LD) $(LDFLAGS) -Wl,--image-base=0x11200000 $(LDDEBUG) $(LIBDIRS) -o $@ $(PUBDIR)/script.def $^ $(SCR2LIB) $(LIBS)
