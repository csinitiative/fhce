#  Copyright (C) 2008, 2009, 2010 The Collaborative Software Foundation.
#
#  This file is part of FeedHandlers (FH).
#
#  FH is free software: you can redistribute it and/or modify it under the terms of the
#  GNU Lesser General Public License as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#
#  FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with FH.  If not, see <http://www.gnu.org/licenses/>.

# ------------------------------------------------------------------------------
# Common variables
# ------------------------------------------------------------------------------

#NOOPTIMIZE = yes

ARCH = $(shell uname -m)

DIST = $(shell if [ -f /etc/redhat-release ]; then				\
                   echo "RedHat";                               \
               elif [ -f /etc/SuSE-release ]; then              \
                   echo "SuSE";									\
			   elif [ -f /etc/lsb-release ]; then				\
				   echo "LSB";									\
               else												\
                   echo "Linux";								\
               fi)

KVER = $(shell uname -r | awk -F. '{print $$1 "." $$2 }')

ifeq ($(DIST),RedHat)
    OS = rhel$(shell sed 's/^.*release \([0-9]\+\).*$$/\1/' /etc/redhat-release)
endif

ifeq ($(DIST),SuSE)
    OS = sles$(word 3,$(shell grep VERSION /etc/SuSE-release))
endif

ifeq ($(DIST),LSB)
	OS_NAME = $(word 2,$(subst =, ,$(shell grep DISTRIB_ID /etc/lsb-release)))
    OS_VERSION = $(word 2,$(subst =, ,$(shell grep DISTRIB_RELEASE /etc/lsb-release)))
	OS = $(OS_NAME)$(OS_VERSION)
endif

BUILD = $(OS)_$(KVER)_$(ARCH)

TOPABS = $(shell cd $(TOP) && pwd)

# ------------------------------------------------------------------------------
# Target directories
# ------------------------------------------------------------------------------

OBJDIR  = obj_$(BUILD)
BINDIR  = bin_$(BUILD)
DEPDIR  = dep_$(BUILD)
LIBDIR  = lib_$(BUILD)
SRCDIR  = src_$(BUILD)
DISTDIR = dist_$(BUILD)
TARDIR  = tar_$(BUILD)
DIRS	  = $(OBJDIR) $(BINDIR) $(DEPDIR) $(LIBDIR) $(SRCDIR) $(DISTDIR)

RPMDIR  = $(TOPABS)/rpmdist_$(BUILD)
RPMDIRS = $(addprefix $(RPMDIR)/,BUILD RPMS SOURCES SPECS SRPMS)

CURRDIR = $(shell pwd)

ifdef INSTROOT
	INSTDIR = $(INSTROOT)/opt/csi/fh
else
	INSTDIR = $(TOP)/$(DISTDIR)/fh
endif

# ------------------------------------------------------------------------------
# Dependencies & Versioning
# ------------------------------------------------------------------------------

MAKEDEPEND = gcc -M $(CFLAGS) -o $*.d $<;                                \
             sed 's|^\(.*\.o:\)|$(OBJDIR)/\1|g' $*.d > $(DEPDIR)/$*.P;   \
             rm -f $*.d

MKREVISION = $(TOP)/scripts/mkrevision

# ------------------------------------------------------------------------------
# Object archives
# ------------------------------------------------------------------------------

AR      = ar
RANLIB  = ranlib
LIBTOOL = libtool

# ------------------------------------------------------------------------------
# Commands to use for compilation, linking, source generation, etc
# ------------------------------------------------------------------------------

CC   = gcc
CXX  = g++

ifeq ($(USE_COMPILER),intel)
	CC   = icc
endif

LEX  = flex
YACC = bison
TAR  = tar

# ------------------------------------------------------------------------------
# Flags for generating packages (tgz & RPM)
# ------------------------------------------------------------------------------

PKGNAME    = (unknown)
TARFLAGS   = -cz
FINDFLAGS  = \( ! -name "*_*_?.?_*" -o -prune \) ! -name "*_*_?.?_*"
FINDFLAGS += ! -wholename ".git*" ! -name "*~"

# ------------------------------------------------------------------------------
# Default compiler and linker flags
# ------------------------------------------------------------------------------
ifeq ($(USE_COMPILER),intel)
	CFLAGS    = -Wall -g -fPIC $(INCLUDES) -I.
	CXXFLAGS  = -Wall -g -fPIC $(INCLUDES)
else
	CFLAGS    = -Wall -Wextra -g -fPIC $(INCLUDES) -I.
	CXXFLAGS  = -Wall -Wextra -g -fPIC $(INCLUDES)
ifeq ($(ARCH),x86_64)
	CFLAGS += -mtune=opteron
endif

endif

LDFLAGS   = -rdynamic -lpthread -lrt -ldl -lfl -lm
TGZROOT   = $(notdir $(shell cd $(TOP); pwd))
RPMFLAGS  = --define 'version $(VERSION)' --define '_topdir $(RPMDIR)'		\
			--define 'tgzroot $(TGZROOT)'									\
			--buildroot $(RPMDIR)/BUILDROOT --clean --rmsource --rmspec -ba

ifndef NOOPTIMIZE
	CFLAGS += -O3 -D__OPTIMIZE__
endif
ifdef __UNIT_TEST__
	CFLAGS  += -D__UNIT_TEST__
endif

INSTFLAGS = -m 755

# ------------------------------------------------------------------------------
# Generic make targets that are used in many places
# ------------------------------------------------------------------------------

DEFAULT: all

$(RPMDIRS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(DIRS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

requireversion: FORCE
	$(if $(VERSION),,$(error You must set VERSION (make <target> VERSION=x) for this make target))

buildstring:
	echo $(BUILD)

FORCE:
