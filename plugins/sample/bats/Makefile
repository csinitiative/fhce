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

TOP = ../../..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

SRCS = $(wildcard *.c)
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

DIRS = $(OBJDIR) $(LIBDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

SHRDIR   	 = $(TOP)/feeds/shared

INCLDIR		+= -I$(SHRDIR)/config
INCLDIR		+= -I$(SHRDIR)/mgmt_thread
INCLDIR		+= -I$(SHRDIR)/lookup_tables
INCLDIR		+= -I$(SHRDIR)/line_handler

INCLDIR		+= -I$(TOP)/common -I$(TOP)/common/missing
INCLDIR		+= -I$(TOP)/mgmt/lib -I$(TOP)/mgmt/lib/admin

INCLDIR		+= -I$(TOP)/feeds/bats/multicast/common

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES  	 = $(INCLDIR)
LDFLAGS   	 = -shared

# ------------------------------------------------------------------------------
# Target FH Common library
# ------------------------------------------------------------------------------

INSTDIR 	:= $(INSTDIR)/bats/plugins
PLUGIN   	 = $(LIBDIR)/sample_bats.so

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(PLUGIN)

dist: all
	install $(INSTFLAGS) -d $(INSTDIR)
	install $(INSTFLAGS) $(PLUGIN) $(INSTDIR)

$(PLUGIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o: %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(DEPDIR) $(LIBDIR)

-include $(DEPS)
