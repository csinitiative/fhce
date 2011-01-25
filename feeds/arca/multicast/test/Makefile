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

SRCS      = $(wildcard *.c)
OBJS	  = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS  	  = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

DIRS = $(OBJDIR) $(LIBDIR) $(DEPDIR)

MGMTDIR   = $(TOP)/mgmt
MSGDIR    = $(TOP)/msg
SHAREDDIR = $(TOP)/common
ARCADIR   = ../common

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES = -I. -I../common
INCLUDES += -I$(SHAREDDIR)/missing -I$(SHAREDDIR)
INCLUDES += -I$(TOP)/feeds/arca/common -I$(TOP)/feeds/arca/codec
INCLUDES += -I$(TOP)/feeds/arca/codec
INCLUDES += -I$(MGMTDIR)/lib
INCLUDES += -I$(MSGDIR)

# -----------------------------------------------------------------------------
# Libaries
# -----------------------------------------------------------------------------

SHAREDLIB = $(SHAREDDIR)/$(LIBDIR)/libfh.a
ARCALIB   = $(ARCADIR)/$(LIBDIR)/libfharca.a

# ------------------------------------------------------------------------------
# Target - Arca unit test library
# ------------------------------------------------------------------------------

LIB = $(LIBDIR)/libtestarca.a

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) unit_test

unit_test : $(LIB) $(SHAREDLIB) $(ARCALIB)
	$(CC) -o $@ $(LIB) $(ARCALIB) $(SHAREDLIB) $(LDFLAGS)

$(LIB): $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

$(ARCALIB): FORCE
	@$(MAKE) -C $(ARCADIR) all


# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(DIRS)

test:

-include $(DEPS)
