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

TOP = ../..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

SRCS = $(wildcard *.c)
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

FHCTL_BIN = $(BINDIR)/fhctl

DIRS = $(OBJDIR) $(BINDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

SHAREDDIR = $(TOP)/common
SHAREDLIB = $(SHAREDDIR)/$(LIBDIR)/libfh.a

CLIDIR = $(TOP)/mgmt/cli
CLILIB = $(CLIDIR)/$(LIBDIR)/libfhcli.a

MGMTDIR = $(TOP)/mgmt/lib
MGMTLIB = $(MGMTDIR)/$(LIBDIR)/libfhmgmt.a

MGRDIR = $(TOP)/mgmt/fhmgr

LIBS = $(MGMTLIB) $(CLILIB) $(SHAREDLIB)

INSTDIR := $(INSTDIR)/mgmt/bin/

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES = -I$(SHAREDDIR) -I$(SHAREDDIR)/missing -I$(CLIDIR) -I$(MGMTDIR) -I$(MGRDIR)

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FHCTL_BIN)

$(FHCTL_BIN): $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

$(CLILIB): FORCE
	@$(MAKE) -C $(CLIDIR) all

$(MGMTLIB): FORCE
	@$(MAKE) -C $(MGMTDIR) all

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(DEPDIR) $(BINDIR)

dist: all
	install $(INSTFLAGS) -d $(INSTDIR)
	install $(INSTFLAGS) $(FHCTL_BIN) $(INSTDIR)

-include $(DEPS)

FORCE:
