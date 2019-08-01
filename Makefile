# Author: Brian Francis
# Makefile V1.2 10/26/2017
##########################
# Changelog:
# 10/5/2017  V1.1: add support for LXPLUS and FNALLPC without CAEN libs
# 10/26/2017 V1.2: add support for OSX

#--------------------------------------------------
# Main targets
#--------------------------------------------------

TITLE = MilliDAQ

SHAREDLIBS = libMilliDAQ.so
TARGETS = MilliDAQ.cc InteractiveDAQ.cc Synchronize.cc

PREFIX = $(DESTDIR)/usr/local
LOGDIR = $(DESTDIR)/var/log
LOGCONFIG = $(DESTDIR)/etc/logrotate.d

#--------------------------------------------------
# Flags/sources
#--------------------------------------------------

CXX = g++
INCLUDE = -I./interface -I$(shell root-config --incdir)
CXXFLAGS = -O3 -Wall -fPIC -std=c++11 $(INCLUDE)
LDFLAGS = -L./. $(shell root-config --libs)
SOFLAGS = -shared

SRC = $(wildcard src/*.cc)
INC = $(filter-out interface/LinkDef.h, $(wildcard interface/*.h))
OBJ = $(SRC:.cc=.o)
OOBJ = $(TARGETS:.cc=.o)
EXE = $(TARGETS:.cc=)

# LXPLUS
ifneq (,$(findstring lxplus,$(shell uname -n)))
	INCLUDE += -I$(BOOST_INCLUDE)
	CXXFLAGS += -Wno-unused-local-typedefs -Wno-maybe-uninitialized
endif

# FNALLPC
ifneq (,$(findstring cmslpc,$(shell uname -n)))
	INCLUDE += -I$(BOOST_INCLUDE)
	CXXFLAGS += -Wno-unused-local-typedefs -Wno-maybe-uninitialized
endif

# OSX
ifeq ($(shell uname -s),Darwin)
	CXX = clang++
	INCLUDE += -I/usr/local/include
	ROOCINTFLAGS = -DNO_BOOST_INSTALL
	SOFLAGS += -stdlib=libc++
endif

#--------------------------------------------------
# Rules
#--------------------------------------------------

all: interface/GitVersion.h $(SHAREDLIBS) $(OBJ) $(OOBJ) $(EXE)
	@@echo "Done!"

shared: $(SHAREDLIBS)
	@@echo "Done!"

debug: CXXFLAGS:=$(filter-out -O3,$(CXXFLAGS) + -DDEBUG -g3)
debug: interface/GitVersion.h $(SHAREDLIBS) $(OBJ) $(OOBJ) $(EXE)
	@@echo "Done!"

#--------------------------------------------------
# Shared libraries for ROOT classes
#--------------------------------------------------

Dict.cxx:
	@@echo "Generating dictonary..."
	@@rootcint -f $@ $(ROOCINTFLAGS) -c $(INCLUDE) interface/DemonstratorConfiguration.h interface/DemonstratorConstants.h interface/GlobalEvent.h interface/V1743Event.h interface/V1743Configuration.h interface/LinkDef.h

libMilliDAQ.so: Dict.cxx
	@@echo "Making $@ ..."
	@@$(CXX) $(CXXFLAGS) $(ROOCINTFLAGS) $(SOFLAGS) -o $@ $(LDFLAGS)  src/DemonstratorConfiguration.cc src/GlobalEvent.cc src/V1743Configuration.cc src/V1743Event.cc src/Logger.cc $<

#--------------------------------------------------
# Rules for main targets
#--------------------------------------------------

interface/GitVersion.h:
	@@echo "#define GIT_HASH \"$(shell git rev-parse HEAD)\"" > $@

%.o: %.cc
	$(CXX) -c $(CXXFLAGS) $< -o $@

%: %.o
	$(CXX) $(CXXFLAGS) $< $(OBJ) $(LDFLAGS) -lCAENDigitizer -lCAENVME -lcaenhvwrapper -lMilliDAQ -lpython2.7 -o $@

.PHONY: install
install:
	@@mkdir -p $(PREFIX)/include/$(TITLE)
	@@install --mode=644 $(INC) $(PREFIX)/include/$(TITLE)/
	@@mkdir -p $(PREFIX)/bin
	@@install --mode=755 $(EXE) $(PREFIX)/bin
	@@mkfifo --mode=666 /tmp/MilliDAQ_FIFO
	@@install --mode=755 scripts/DAQCommand.py $(PREFIX)/bin/DAQCommand
	@@install --mode=755 scripts/FGCommand.sh $(PREFIX)/bin/FGCommand
	@@install --mode=755 $(SHAREDLIBS) /usr/lib/
	@@install --mode=755 Dict_rdict.pcm /usr/lib/
	@@touch $(LOGDIR)/MilliDAQ.log
	@@chmod 666 $(LOGDIR)/MilliDAQ.log
	@@touch $(LOGDIR)/MilliDAQ_RunList.log
	@@chmod 666 $(LOGDIR)/MilliDAQ_RunList.log
	@@install --mode=644 etc/MilliDAQ_log.conf $(LOGCONFIG)/MilliDAQ
	@@install --mode=664 etc/MilliDAQ.service /etc/systemd/system/
	@@systemctl daemon-reload
	@@echo
	@@echo "Installation complete! To begin MilliDAQ service:"
	@@echo "	sudo systemctl start MilliDAQ.service"
	@@echo

.PHONY: uninstall
uninstall:
	@@echo "Stopping service..."
	@@systemctl stop MilliDAQ.service
	@@rm -f /etc/systemd/system/MilliDAQ.service
	@@systemctl daemon-reload
	@@rm -f /usr/lib/libMilliDAQ.so
	@@rm -f /usr/lib/Dict_rdict.pcm
	@@rm -rf $(PREFIX)/include/$(TITLE)/
	@@rm -f $(PREFIX)/bin/$(TITLE)
	@@rm -f $(PREFIX)/bin/DAQCommand
	@@rm -f $(PREFIX)/bin/FGCommand
	@@rm -f /tmp/MilliDAQ_FIFO
	@@rm -f $(LOGCONFIG)/MilliDAQ
	@@echo "Uninstalled MilliDAQ!"

.PHONY: clean
clean:
	@@/bin/rm -f interface/GitVersion.h $(SHAREDLIBS) $(EXE) $(OBJ) $(OOBJ) *_cc.d *_cc.so *_rdict.pcm Dict.cxx *~
