# $Id$
#
# example module makefile
#
# 
# WARNING: do not run this directly, it should be run by the master Makefile

include ../../Makefile.defs
auto_gen=
NAME=kex.so
LIBS=

DEFS+=-DOPENSER_MOD_INTERFACE

SERLIBPATH=../../lib
SER_LIBS+=$(SERLIBPATH)/kmi/kmi
SER_LIBS+=$(SERLIBPATH)/kcore/kcore
SER_LIBS+=$(SERLIBPATH)/srutils/srutils
include ../../Makefile.modules
