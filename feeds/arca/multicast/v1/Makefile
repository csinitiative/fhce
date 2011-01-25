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

TOP = ../../../..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

SRCS = $(wildcard *.c)
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

FH_BIN = $(BINDIR)/fharca_v1

INSTDIR := $(INSTDIR)/arca

INSTBINDIR     = $(INSTDIR)/bin/
INSTETCDIR     = $(INSTDIR)/etc/
INSTPLGDIR     = $(INSTDIR)/plugins/

FH_CFG         = ../etc/arca.conf
DIRS           = $(OBJDIR) $(BINDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Distribution "stuff"
# ------------------------------------------------------------------------------

PKGFILES  = build common mgmt msg scripts feeds/arca/codec feeds/arca/common
PKGFILES += feeds/arca/multicast plugins/sample/arca
PKGNAME   = arcav1

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

ARCADIR   = ../common
ARCALIB   = $(ARCADIR)/$(LIBDIR)/libfharca.a

SHAREDDIR = $(TOP)/common
SHAREDLIB = $(SHAREDDIR)/$(LIBDIR)/libfh.a

MGMTDIR   = $(TOP)/mgmt/lib
MGMTLIB   = $(MGMTDIR)/$(LIBDIR)/libfhmgmt.a

MSGDIR    = $(TOP)/msg
MSGLIB    = $(MSGDIR)/$(LIBDIR)/libfhmsg.a

FH_LIBS = $(ARCALIB) $(MSGLIB) $(MGMTLIB) $(SHAREDLIB)

# ------------------------------------------------------------------------------
# Unit testing
# ------------------------------------------------------------------------------

TESTDIR   = ../test
TESTLIB   = $(TESTDIR)/$(LIBDIR)/libtestarca.a

ifdef __UNIT_TEST__
	FH_LIBS := $(TESTLIB) $(FH_LIBS)
endif

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES  = -I$(SHAREDDIR) -I$(SHAREDDIR)/missing
INCLUDES += -I$(ARCADIR) -I$(MSGDIR) -I$(MGMTDIR)
INCLUDES += -I$(TOP)/feeds/arca/common
INCLUDES += -I$(TOP)/feeds/arca/codec

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FH_BIN)

$(FH_BIN): $(OBJS) $(FH_LIBS)
	$(CC) -o $@ $(OBJS) $(FH_LIBS) $(LDFLAGS)

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

$(ARCALIB): FORCE
	@$(MAKE) -C $(ARCADIR) all

$(MGMTLIB): FORCE
	@$(MAKE) -C $(MGMTDIR) all

$(MSGLIB): FORCE
	@$(MAKE) -C $(MSGDIR) all

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

dist: all
	install $(INSTFLAGS) -d $(INSTBINDIR)
	install $(INSTFLAGS) -d $(INSTETCDIR)
	install $(INSTFLAGS) -d $(INSTPLGDIR)
	install $(INSTFLAGS) $(FH_BIN) $(INSTBINDIR)
	@if [ ! -f $(INSTETCDIR)$(FH_CFG) ]; then                   \
		install $(INSTFLAGS) $(FH_CFG) $(INSTETCDIR);           \
		echo "install $(INSTFLAGS) $(FH_CFG) $(INSTETCDIR)";	\
	fi

# include packaging targets
include $(TOP)/build/dist.mk

clean:
	rm -rf $(DIRS)

test:

-include $(DEPS)
