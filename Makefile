# Makefile                                                       -*-makefile-*-

PREFIX     ?= $(firstword $(wildcard /opt/bb /usr))
LLVMDIR    ?= $(PREFIX)

SYSTEM     := $(shell uname -s)
PROCESSOR   = $(shell uname -p)
DESTDIR    ?= $(PREFIX)

GCCVERSION  = 4.9.2
GCCDIR      = /opt/swt/install/gcc-$(GCCVERSION)
CXX         = $(GCCDIR)/bin/g++


TARGET      = bde_verify_bin

CSABASE     = csabase
LCB         = bde-verify
LIBCSABASE  = lib$(LCB).a
CSABASEDIR  = groups/csa/csabase

CSAUTIL     = csautil
LCU         = bde-verify-util
LIBCSAUTIL  = lib$(LCU).a
CSAUTILDIR  = groups/csa/csautil

AET         = aet
LAET        = aet
LIBAET      = lib$(LAET).a
AETDIR      = /home/shargadon/aet/aetutils

CXXFLAGS   += -m64 -std=c++11
CXXFLAGS   += -Wall -Wno-unused-local-typedefs

CXXFLAGS   += -DSPELL_CHECK=1
INCFLAGS   += -I$(PREFIX)/include -I/opt/swt/include

# Set up locations and flags for the compiler that will build bde_verify.
ifeq ($(notdir $(CXX)),clang++)
    CXXFLAGS   += --gcc-toolchain=$(GCCDIR) -Wno-mismatched-tags
endif

ifeq ($(SYSTEM),Linux)
    AR          = /usr/bin/ar
	ARCHCODE    = linux
    LIBDIRS     = $(GCCDIR)/lib64                                             \
                  $(PREFIX)/lib64                                             \
                  /opt/swt/lib64                                              \
                  /usr/lib64
    LDFLAGS    += -Wl,--enable-new-dtags
    LDFLAGS    += $(foreach L,$(LIBDIRS),-Wl,-L$(L),-rpath,$(L))
else ifeq ($(SYSTEM),SunOS)
    AR          = /usr/ccs/bin/ar
    CXXFLAGS   += -DBYTE_ORDER=BIG_ENDIAN
    LIBDIRS     = $(GCCDIR)/lib/sparcv9                                       \
                  $(PREFIX)/lib64                                             \
                  /opt/swt/lib64                                              \
                  /usr/lib/sparcv9
    LDFLAGS    += $(foreach L,$(LIBDIRS),-Wl,-L$(L),-R,$(L))
    EXTRALIBS  += -lrt
    ifneq (,$(wildcard $(foreach L,$(LIBDIRS),$(L)/libtinfo.so)))
        EXTRALIBS += -ltinfo
    endif
    ifneq (,$(wildcard $(foreach L,$(LIBDIRS),$(L)/libmalloc.so)))
        EXTRALIBS += -lmalloc
    endif
endif

OBJ        := $(SYSTEM)-$(notdir $(CXX))

# Set up location of clang headers and libraries needed by bde_verify.
INCFLAGS   += -I$(LLVMDIR)/include
INCFLAGS   += -I/bb/build/$(SYSTEM)-$(PROCESSOR)-64/release/robolibs/trunk/lib/dpkgroot/opt/bb/include/  
LDFLAGS    += -L$(LLVMDIR)/lib -L$(CSABASEDIR)/$(OBJ) -L$(CSAUTILDIR)/$(OBJ)
LDFLAGS    += -L$(AETDIR)/$(OBJ) -L./bregsvc
LDFLAGS    += -L/bb/build/$(SYSTEM)-$(PROCESSOR)-64/release/robolibs/trunk/lib/
LDFLAGS    += -L/bb/build/$(SYSTEM)-$(PROCESSOR)-64/release/robolibs/trunk/lib/dpkgroot/opt/bb/lib64/robo/
LDFLAGS    += -L/bb/build/$(SYSTEM)-$(PROCESSOR)-64/release/robolibs/trunk/lib/dpkgroot/opt/bb/lib64/


VERBOSE ?= @

#  ----------------------------------------------------------------------------

CXXFILES =                                                                    \
		groups/csa/breg_matchers.cpp								          \
		groups/csa/breg_remover.cpp										      \
		groups/csa/breg_replacer.cpp                                          \
		groups/csa/expr_tree.cpp                                              \

# -----------------------------------------------------------------------------

DEFFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
INCFLAGS += -I. -I$(CSABASEDIR) -I$(CSAUTILDIR) -Igroups/csa/csadep
 

CXXFLAGS += -g -fno-common -fno-strict-aliasing -fno-exceptions -fno-rtti

OFILES = $(CXXFILES:%.cpp=$(OBJ)/%.o)

EXTRALIBS +=  -lbte              											  \
			  -lbce              											  \
			  -lbae              											  \
			  -lbas  														  \
			  -lbde  														  \
			  -lbsc  														  \
			  -lbsl  													 	  \
			  -lbae.opt_exc_mt_64  										  	  \
			  -lbte.opt_exc_mt_64  										  	  \
			  -lbce.opt_exc_mt_64  										  	  \
			  -lbde.opt_exc_mt_64  										  	  \
			  -lbdl.opt_exc_mt_64  										  	  \
			  -lpthread  													  \
			  -la_baslt  													  \
			  -lparmsbase  												  	  \
			  -lbsc  														  \
			  -lbsi  														  \
			  -le_ipc  												      	  \
			  -lbbmsgbufs  												  	  \
			  -lbbipc  													   	  \
			  -lsysutil  													  \
			  -lbae.opt_exc_mt_64  										  	  \
			  -lbte.opt_exc_mt_64  										  	  \
			  -lbce.opt_exc_mt_64  										  	  \
			  -lbde.opt_exc_mt_64  										  	  \
			  -lbdl.opt_exc_mt_64  										  	  \
			  -lbsl.opt_exc_mt_64  										  	  \
			  -lm  														  	  \
			  -lopenbsd-compat  											  \
			  -lrt  														  \

LIBS     =    -l$(LCB)                                                        \
			  -l$(LCU)                                                        \
			  -l$(LAET)                                                       \
			  -lbreguisvcmsg.$(ARCHCODE).64                                   \
              -lLLVMX86Info                                                   \
              -lLLVMSparcInfo                                                 \
              -lclangFrontendTool                                             \
              -lclangCodeGen                                                  \
              -lLLVMIRReader                                                  \
              -lLLVMLinker                                                    \
              -lLLVMipo                                                       \
              -lLLVMX86AsmParser                                              \
              -lLLVMSparcAsmParser                                            \
              -lLLVMX86CodeGen                                                \
              -lLLVMSparcCodeGen                                              \
              -lLLVMSelectionDAG                                              \
              -lLLVMCodeGen                                                   \
              -lLLVMScalarOpts                                                \
              -lLLVMInstCombine                                               \
              -lLLVMVectorize                                                 \
              -lLLVMInstrumentation                                           \
              -lLLVMObjCARCOpts                                               \
              -lLLVMTransformUtils                                            \
              -lLLVMipa                                                       \
              -lLLVMAnalysis                                                  \
              -lclangStaticAnalyzerFrontend                                   \
              -lclangRewriteFrontend                                          \
              -lclangARCMigrate                                               \
              -lclangFrontend                                                 \
              -lclangSerialization                                            \
              -lLLVMProfileData                                               \
              -lLLVMX86Desc                                                   \
              -lLLVMSparcDesc                                                 \
              -lLLVMObject                                                    \
              -lLLVMBitReader                                                 \
              -lLLVMTarget                                                    \
              -lLLVMAsmParser                                                 \
              -lLLVMBitWriter                                                 \
              -lLLVMX86AsmPrinter                                             \
              -lLLVMSparcAsmPrinter                                           \
              -lLLVMX86Utils                                                  \
              -lLLVMCore                                                      \
              -lclangParse                                                    \
              -lLLVMMCParser                                                  \
              -lLLVMMCDisassembler                                            \
              -lclangSema                                                     \
              -lclangStaticAnalyzerCheckers                                   \
              -lclangStaticAnalyzerCore                                       \
              -lclangASTMatchers                                              \
              -lclangEdit                                                     \
              -lclangAnalysis                                                 \
              -lclangAST                                                      \
              -lclangToolingCore                                              \
              -lclangRewrite                                                  \
              -lclangLex                                                      \
              -lclangDriver                                                   \
              -lclangBasic                                                    \
              -lLLVMMC                                                        \
              -lLLVMOption                                                    \
              -lLLVMSupport                                                   \
              -lncurses                                                       \
              -lpthread                                                       \
              -ldl                                                            \
              -lz                                                             \
              -laspell                                                        \
              $(EXTRALIBS)

default: $(OBJ)/$(TARGET)

.PHONY: csabase
.PHONY: csautil

$(CSABASEDIR)/$(OBJ)/$(LIBCSABASE): csabase
	$(VERBOSE) $(MAKE) -C $(CSABASEDIR)

$(CSAUTILDIR)/$(OBJ)/$(LIBCSAUTIL): csautil
	$(VERBOSE) $(MAKE) -C $(CSAUTILDIR)


$(OBJ)/$(TARGET): $(CSABASEDIR)/$(OBJ)/$(LIBCSABASE) $(CSAUTILDIR)/$(OBJ)/$(LIBCSAUTIL) $(OFILES)
	@echo linking executable
	$(VERBOSE) $(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@.$$ $(OFILES) $(LIBS)
	mv $@.$$ $@

$(OBJ)/%.o: %.cpp
	@if [ ! -d $(@D) ]; then mkdir -p $(@D); fi
	@echo compiling $(@:$(OBJ)/%.o=%.cpp)
	$(VERBOSE) $(CXX) $(INCFLAGS) $(DEFFLAGS) $(CXXFLAGS) $(WARNFLAGS) \
                          -o $@ -c $(@:$(OBJ)/%.o=%.cpp)
.PHONY: clean

clean:
	@echo cleaning files
	$(VERBOSE) $(MAKE) -C $(CSABASEDIR) clean
	$(VERBOSE) $(RM) -rf $(OBJ)

gh-pages:
	$(VERBOSE) $(MAKE) -C doc \
    SPHINXOPTS='-t bb_cppverify' BUILDDIR='../bb_cppverify_build' clean html
	$(VERBOSE) $(MAKE) -C doc \
    SPHINXOPTS='-t bde_verify'   BUILDDIR='../bde_verify_build'   clean html
	cp doc/index.html .
	git checkout gh-pages
	rm -rf Linux-*g++ $(CSABASEDIR)/Linux-*g++ \
           SunOS-*g++ $(CSABASEDIR)/SunOS-*g++
	git add -A
	git commit -m "Generate gh-pages"

# -----------------------------------------------------------------------------

BDE_VERIFY_DIR := $(shell /bin/pwd)

# All the Makefiles below the checks directory.
define ALLM :=
    $(shell find checks -name Makefile | sort)
endef

# All the Makefiles below both the checks and CURRENT directory.
define CURM :=
    $(shell find checks -regex 'checks\(/.*\)?/$(CURRENT)\(/.*\)?/Makefile' | \
            sort)
endef

CNAMES   := $(foreach N,$(ALLM),$(patsubst %,%.check,$(N)))
CCURNAME := $(foreach N,$(CURM),$(patsubst %,%.check,$(N)))
RNAMES   := $(foreach N,$(ALLM),$(patsubst %,%.run,$(N)))
RCURNAME := $(foreach N,$(CURM),$(patsubst %,%.run,$(N)))

.PHONY: check-current check $(CNAMES) run-current run $(RNAMES)

check: $(OBJ)/$(TARGET) $(CNAMES)
check-current: $(OBJ)/$(TARGET) $(CCURNAME)

$(CNAMES):
	$(VERBOSE) $(MAKE) -C $(@D) -k --no-print-directory check

run: $(OBJ)/$(TARGET) $(RNAMES)
run-current: $(OBJ)/$(TARGET) $(RCURNAME)

$(RNAMES):
	$(VERBOSE) $(MAKE) -C $(@D) -k --no-print-directory run

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
