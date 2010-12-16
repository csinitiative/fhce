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

OPRADIR = $(TOP)/feeds/opra/fast
OPRAINC = $(addprefix -I$(OPRADIR)/, common codec v2)

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES   = -I$(TOP)/common -I$(TOP)/common/missing -I$(TOP)/mgmt/lib -I$(TOP)/mgmt/lib/admin
INCLUDES  += $(OPRAINC)
LDFLAGS    = -shared

# ------------------------------------------------------------------------------
# Target FH Common library
# ------------------------------------------------------------------------------

INSTDIR := $(INSTDIR)/opra/plugins
PLUGIN   = $(LIBDIR)/opra_plugin.so

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
