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
# Set up directory/target variables
# ------------------------------------------------------------------------------

DIRS 		 = $(OBJDIR) $(LIBDIR) $(DEPDIR)
INCLDIRS	 = common mgmt/lib mgmt/lib/admin feeds/shared/config feeds/shared/mirrored_mcast
INCLDIRS	+= feeds/shared/mgmt_thread feeds/shared/line_handler feeds/shared/lookup_tables
INCLDIRS	+= feeds/shared/gap_mgmt

# Basic sources and targets
SRCS      	 = $(wildcard *.c)
OBJS      	 = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS 		 = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES 	 = $(addprefix -I$(TOP)/,$(INCLDIRS))

# ------------------------------------------------------------------------------
# Versioning
# ------------------------------------------------------------------------------

REV_FILE   	 = fh_itch_revision.h

# ------------------------------------------------------------------------------
# Target - ARCA Common library
# ------------------------------------------------------------------------------

LIB = $(LIBDIR)/libfhitch.a

# ------------------------------------------------------------------------------
# Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FASTDIR) $(LIB)

$(LIB): $(REV_FILE) $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

$(DIRS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(REV_FILE):
	@$(MKREVISION) $@

clean:
	rm -rf $(DIRS) $(REV_FILE)
	$(MAKE) -C test $@

test: FORCE
	$(MAKE) -C test all

dist:

rpmdist:

# ------------------------------------------------------------------------------
# Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/$(CODECDIR)/%.o : $(CODECDIR)/%.c
	@$(MAKEDEPEND)
	$(CC) $(CODECCFLGS) -o $@ -c $<

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

-include $(DEPS)
