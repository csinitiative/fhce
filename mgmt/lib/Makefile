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

ADMDIR = admin
ADMSRC = $(notdir $(wildcard $(ADMDIR)/*.c))

SRCS = $(wildcard *.c) $(ADMSRC)
OBJS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

DIRS = $(OBJDIR) $(LIBDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

SHAREDDIR = $(TOP)/common

INCLUDES = -I. -I$(SHAREDDIR) -I$(SHAREDDIR)/missing

# ------------------------------------------------------------------------------
# Target FH Common library
# ------------------------------------------------------------------------------

LIB = $(LIBDIR)/libfhmgmt.a

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o : $(ADMDIR)/%.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(OBJDIR) $(DEPDIR) $(LIBDIR)

dist:

-include $(DEPS)



