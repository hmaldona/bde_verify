# Makefile                                                       -*-makefile-*-

PREFIX     ?= $(firstword $(wildcard /opt/bb /usr))
LLVMDIR    ?= $(PREFIX)

SYSTEM     := $(shell uname -s)
PROCESSOR   = $(shell uname -p)

GCCVERSION  = 4.9.2
GCCDIR      = /opt/swt/install/gcc-$(GCCVERSION)
CXX         = $(GCCDIR)/bin/g++

# ifeq ($(notdir $(CXX)),g++)
# GCCDIR     ?= $(patsubst %/bin/g++,%,$(shell which $(CXX)))
# else
# GCCDIR     ?= $(patsubst %/bin/g++,%,$(shell which g++))
# endif

CSAUTIL  = csautil
LCU      = bde-verify-util
LIBCSAUTIL = lib$(LCU).a

AET        = aet
LAET       = aet
LIBAET     = lib$(LAET).a
AETDIR     = /home/shargadon/aet/aetutils

CXXFLAGS   += -m64 -std=c++11
CXXFLAGS   += -Wall -Wno-unused-local-typedefs
INCFLAGS   += -I$(PREFIX)/include -I/opt/swt/include

# Set up locations and flags for the compiler that will build bde_verify.
ifeq ($(notdir $(CXX)),clang++)
    CXXFLAGS   += --gcc-toolchain=$(GCCDIR) -Wno-mismatched-tags
endif

ifeq ($(SYSTEM),Linux)
    AR          = /usr/bin/ar
else ifeq ($(SYSTEM),SunOS)
    AR          = /usr/ccs/bin/ar
    CXXFLAGS   += -DBYTE_ORDER=BIG_ENDIAN
endif

OBJ        := $(SYSTEM)-$(notdir $(CXX))

VERBOSE ?= @

# -----------------------------------------------------------------------------

CXXFILES =                                                 \
	csautil_bregistry.cpp                              \
	csautil_bregsvcclient.cpp                          \

# -----------------------------------------------------------------------------

DEFFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
INCFLAGS += -I. -I$(AETDIR) -I../../../bregsvc/src 
INCFLAGS += -I/bb/build/$(SYSTEM)-$(PROCESSOR)-64/release/robolibs/trunk/lib/dpkgroot/opt/bb/include/
CXXFLAGS += -fno-common -fno-strict-aliasing

OFILES = $(CXXFILES:%.cpp=$(OBJ)/%.o)

default: $(CSAUTIL)

.PHONY: aet

$(CSAUTIL): $(OBJ)/$(LIBCSAUTIL)

#$(OBJ)/$(LIBAET): aet
#	$(VERBOSE) $(MAKE) -s -C $(AETDIR)

$(OBJ)/$(LIBCSAUTIL): $(AETDIR)/$(OBJ)/$(LIBAET) $(OFILES)
	@echo creating library
	$(VERBOSE) $(AR) cr $@ $(OFILES)

$(OBJ)/%.o: %.cpp
	@if [ ! -d $(@D) ]; then mkdir -p $(@D); fi
	@echo compiling $(@:$(OBJ)/%.o=%.cpp)
	$(VERBOSE) $(CXX) $(INCFLAGS) $(DEFFLAGS) $(CXXFLAGS) $(WARNFLAGS) \
                          -o $@ -c $(@:$(OBJ)/%.o=%.cpp)

.PHONY: install install-bin install-dev

install: install-bin install-dev

install-bin:

install-dev: $(OBJ)/$(LIBCSABASE)
	mkdir -p $(DESTDIR)/include/bde-verify/utils
	cp csabase_*.h $(DESTDIR)/include/bde-verify
	cp utils/*.hpp $(DESTDIR)/include/bde-verify/utils
	mkdir -p $(DESTDIR)/lib
	cp $(OBJ)/$(LIBCSABASE) $(DESTDIR)/lib

.PHONY: clean

clean:
	@echo cleaning files
	$(VERBOSE) $(RM) -rf $(OBJ)

# -----------------------------------------------------------------------------

.PHONY: depend

depend $(OBJ)/make.depend:
	@if [ ! -d $(OBJ) ]; then mkdir $(OBJ); fi
	@echo analysing dependencies
	$(VERBOSE) $(CXX) $(INCFLAGS) $(DEFFLAGS) -M $(CXXFILES)              \
            $(filter-out -Wno-unused-local-typedefs, $(CXXFLAGS))             \
        | perl -pe 's[^(?! )][$(OBJ)/]' > $(OBJ)/make.depend

ifneq "$(MAKECMDGOALS)" "clean"
    -include $(OBJ)/make.depend
endif

## ----------------------------------------------------------------------------
## Copyright (C) 2014 Bloomberg Finance L.P.
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to
## deal in the Software without restriction, including without limitation the
## rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
## sell copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in
## all copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
## FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
## IN THE SOFTWARE.
## ----------------------------- END-OF-FILE ----------------------------------
