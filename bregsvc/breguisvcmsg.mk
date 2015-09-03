# -*- makefile -*-
# $Id: breguisvcmsg.mk $

ANAME=libbreguisvcmsg.a

SRCDIRS=src

COMMON_FLAGS=$(addprefix -I, $(SRCDIRS))

USER_CFLAGS   += ${COMMON_FLAGS} -I.
USER_CPPFLAGS += ${COMMON_FLAGS} -I.
USER_FFLAGS   += ${COMMON_FLAGS} -I.

IS_GCC_WARNINGS_CLEAN=yes
IS_OPTIMIZED=debug

#--- do not modify below ---

IS_BDE=no
IS_PTHREAD=yes
IS_EXCEPTION=yes
IS_DEPENDS_NATIVE=1

MKINCL?=/bbsrc/mkincludes/
include $(MKINCL)sourcelist.mk
AOBJS?=$(OBJS_AR)
include $(MKINCL)machdep.newlink

-include local.mk

include $(MKINCL)linklib.newlink

# vim:ft=make
