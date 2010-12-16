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

FH_BIN = $(BINDIR)/fhopra_v2

INSTDIR := $(INSTDIR)/opra

INSTBINDIR     = $(INSTDIR)/bin/
INSTETCDIR     = $(INSTDIR)/etc/
INSTSCRIPTSDIR = $(INSTDIR)/scripts/
INSTPLGDIR     = $(INSTDIR)/plugins/perf

FH_CFG    = ../etc/opra.conf
FH_LO     = ../etc/listedoptions.txt

DIRS    = $(OBJDIR) $(BINDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Distribution "stuff"
# ------------------------------------------------------------------------------

PKGFILES = build common mgmt msg scripts feeds/opra feeds/shared plugins/sample/opra
PKGNAME  = oprav2

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

OPRADIR   = ../common
OPRALIB   = $(OPRADIR)/$(LIBDIR)/libfhopra.a

SHAREDDIR = $(TOP)/common
SHAREDLIB = $(SHAREDDIR)/$(LIBDIR)/libfh.a

MGMTDIR   = $(TOP)/mgmt/lib
MGMTLIB   = $(MGMTDIR)/$(LIBDIR)/libfhmgmt.a

FASTDIR   = ../codec
FASTLIB   = $(FASTDIR)/$(LIBDIR)/libfhopra_fast.a

MSGDIR    = $(TOP)/msg
MSGLIB    = $(MSGDIR)/$(LIBDIR)/libfhmsg.a

FH_LIBS = $(OPRALIB) $(FASTLIB) $(MSGLIB) $(MGMTLIB) $(SHAREDLIB)

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES  = -I$(SHAREDDIR) -I$(SHAREDDIR)/missing -I$(OPRADIR) -I$(FASTDIR) -I$(MSGDIR) -I$(MGMTDIR)

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FH_BIN)

$(FH_BIN): $(OBJS) $(FH_LIBS)
	$(CC) -o $@ $(OBJS) $(FH_LIBS) $(LDFLAGS)

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

$(OPRALIB): FORCE
	@$(MAKE) -C $(OPRADIR) all

$(FASTLIB): FORCE
	@$(MAKE) -C $(FASTDIR) all

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
	install $(INSTFLAGS) -d $(INSTSCRIPTSDIR)
	install $(INSTFLAGS) -d $(INSTPLGDIR)
	install $(INSTFLAGS) $(FH_BIN) $(INSTBINDIR)
	install $(INSTFLAGS) $(FH_LO) $(INSTETCDIR)
	install $(INSTFLAGS) $(FH_BIN) $(INSTBINDIR)
	@if [ ! -f $(INSTETCDIR)$(FH_CFG) ]; then                   \
		install $(INSTFLAGS) $(FH_CFG) $(INSTETCDIR);           \
		echo "install $(INSTFLAGS) $(FH_CFG) $(INSTETCDIR)";	\
	fi

# include packaging targets
include $(TOP)/build/dist.mk

clean:
	rm -rf $(OBJDIR) $(DEPDIR) $(BINDIR)

-include $(DEPS)
