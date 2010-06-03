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

ETCDIR = etc

FHMGR_BIN = $(BINDIR)/fhmgr
FHMGR_CFG = $(ETCDIR)/fhmgr.conf

DIRS = $(OBJDIR) $(BINDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

SHAREDDIR = $(TOP)/common
SHAREDLIB = $(SHAREDDIR)/$(LIBDIR)/libfh.a

MGMTDIR = $(TOP)/mgmt/lib
MGMTLIB = $(MGMTDIR)/$(LIBDIR)/libfhmgmt.a

LIBS = $(SHAREDLIB) $(MGMTLIB)

INSTBINDIR := $(INSTDIR)/mgmt/bin/
INSTETCDIR := $(INSTDIR)/mgmt/etc/
INSTPLGDIR := $(INSTDIR)/mgmt/plugins/

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES = -I$(SHAREDDIR) -I$(SHAREDDIR)/missing -I$(MGMTDIR)

# ------------------------------------------------------------------------------
# Versioning
# ------------------------------------------------------------------------------

REV_FILE   = fh_mgmt_revision.h

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FHMGR_BIN)

$(FHMGR_BIN): $(REV_FILE) $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

$(MGMTLIB): FORCE
	@$(MAKE) -C $(MGMTDIR) all

$(REV_FILE):
	@$(MKREVISION) $@

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(DEPDIR) $(BINDIR) $(REV_FILE)

dist: all
	install $(INSTFLAGS) -d $(INSTBINDIR)
	install $(INSTFLAGS) -d $(INSTETCDIR)
	install $(INSTFLAGS) -d $(INSTPLGDIR)
	install $(INSTFLAGS) $(FHMGR_BIN) $(INSTBINDIR)
	install $(INSTFLAGS) $(FHMGR_CFG) $(INSTETCDIR)

-include $(DEPS)

FORCE:
