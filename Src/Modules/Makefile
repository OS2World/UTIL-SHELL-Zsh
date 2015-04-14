# Generated automatically from Makefile.in by configure.
#
# $Id: Makefile.in,v 3.1.0.1 1996/12/05 03:59:45 hzoli Exp $
#
# Makefile for Src/Modules subdirectory
#
# Copyright (c) 1996 Richard Coleman
# All rights reserved.
#
# Permission is hereby granted, without written agreement and without
# license or royalty fees, to use, copy, modify, and distribute this
# software and to distribute modified versions of this software for any
# purpose, provided that the above copyright notice and the following
# two paragraphs appear in all copies of this software.
#
# In no event shall Richard Coleman or the Zsh Development Group be liable
# to any party for direct, indirect, special, incidental, or consequential
# damages arising out of the use of this software and its documentation,
# even if Richard Coleman and the Zsh Development Group have been advised of
# the possibility of such damage.
#
# Richard Coleman and the Zsh Development Group specifically disclaim any
# warranties, including, but not limited to, the implied warranties of
# merchantability and fitness for a particular purpose.  The software
# provided hereunder is on an "as is" basis, and Richard Coleman and the
# Zsh Development Group have no obligation to provide maintenance,
# support, updates, enhancements, or modifications.
#

# zsh version
VERSION = 3.1.0

SHELL = /bin/sh

top_srcdir = ../..
srcdir     = .

prefix      = /usr/local
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin
libdir      = ${exec_prefix}/lib
MODDIR      = $(libdir)/zsh/$(VERSION)

CC        = gcc
CPPFLAGS  = 
DEFS      = -DHAVE_CONFIG_H
CFLAGS    =  -Wall -Wno-implicit -Wmissing-prototypes -O2
LDFLAGS   = 
LIBS      =  -ltermcap
DL_EXT    = 
DLLD      = 
DLCFLAGS  = 
DLLDFLAGS = 

INCLUDES = -I../.. -I. -I.. -I$(srcdir) -I$(srcdir)/..

COMPILE = $(CC) -c $(INCLUDES) $(CPPFLAGS) $(DEFS) $(CFLAGS) $(DLCFLAGS)
LINK    = $(DLLD) $(LDFLAGS) $(DLLDFLAGS) -o $@

INSTALL         = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}

SED = sed

KNR_OBJ=.o
KNROBJ=._foo_

ANSIOBJ=.o
ANSI_OBJ=._foo_

.SUFFIXES:
.SUFFIXES: .c .$(DL_EXT) $(ANSIOBJ) $(KNROBJ) .pro

.c$(ANSIOBJ):
	$(COMPILE) -o $@ $<

.c$(KNROBJ):
	../ansi2knr $< > $@.c
	$(COMPILE) -o $@ $@.c
	rm -f $@.c

.o.$(DL_EXT):
	$(LINK) $<

.c.pro:
	$(SED) -n -f $(srcdir)/../makepro.sed $< > $@

# generated prototypes
PROTO = example.pro

# target modules
MODULES = example.so

# ========= DEPENDENCIES FOR BUILDING ==========

# default target
all: modules

modules: $(PROTO) $(MODULES)

# ========== DEPENDENCIES FOR INSTALLING ==========

install: install.bin install.modules
uninstall: uninstall.bin uninstall.modules

install.bin:
uninstall.bin:

install.modules:
	$(top_srcdir)/mkinstalldirs $(MODDIR)
	for mod in .. $(MODULES); do \
	    if test .$$mod = ...; then :; else \
		$(INSTALL_PROGRAM) $$mod $(MODDIR)/$$mod; \
	    fi; \
	done

uninstall.modules:
	for mod in .. $(MODULES); do \
	    if test .$$mod = ...; then :; else \
		test -f $(MODDIR)/$$mod && rm -f $(MODDIR)/$$mod; \
	    fi; \
	done

# ========== DEPENDENCIES FOR CLEANUP ==========

DLCLEAN = $(MODULE_DIR)/*.$(DL_EXT) $(MODULE_DIR)/*.*.[co] $(MODULE_DIR)/*.pro
NLCLEAN =
mostlyclean:
	rm -f core stamp-modobjs* *.o *.$(DL_EXT) *~

clean: mostlyclean
	rm -f *.*.c *.pro

distclean: clean
	rm -f Makefile

realclean: distclean

superclean: realclean

# ========== DEPENDENCIES FOR MAINTENANCE ==========

subdir = Src/Modules

Makefile: Makefile.in ../../config.status
	cd ../.. && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= ./config.status
