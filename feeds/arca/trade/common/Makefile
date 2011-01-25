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
# source files common to ARCA feed handlers can be found here
ARCA_COMMON      = $(TOP)/feeds/arca/common
ARCA_OBJS        = $(ARCA_COMMON)/$(OBJDIR)
#include directories may be found here
INCLDIRS	 = common common/missing feeds/arca/common mgmt/lib msg
# build outputs go here
DIRS             = $(OBJDIR) $(LIBDIR) $(DEPDIR)
DIRS             += $(ARCA_OBJS)

# arca sources and targets
# feed handler common sources are in csi-fh/common
ARCA_COMMON_SRCS = $(wildcard $(ARCA_COMMON)/*.c)
SRCS      	 = $(wildcard *.c)
ALL_SRCS         = $(SRCS) $(ARCA_COMMON_SRCS)
OBJS      	 = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
ARCA_COMMON_NMS  = $(ARCA_COMMON_SRCS:.c=.o)
ARCA_COMMON_NAMES= $(notdir $(ARCA_COMMON_NMS))
ARCA_COMMON_OBJS = $(addprefix $(ARCA_OBJS)/,$(ARCA_COMMON_NAMES))
DEPS 		 = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))
DEPS             += $(addprefix $(DEPDIR)/,$(ARCA_COMMON_SRCS:.c=.P))

# FAST codec sources and targets
CODECDIR	 = $(TOP)/feeds/arca/codec
CODECSRCS	 = $(wildcard $(CODECDIR)/*.c)
CODECOBJS	 = AB_Fast.o newFastDecode.o
CODECDEPS        = $(CODECOBJS:.o=.P)
OBJS		+= $(addprefix $(CODECDIR)/,$(CODECOBJS))
DEPS 		+= $(addprefix $(CODECDIR)/,$(CODECDEPS))

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES 	 = $(addprefix -I$(TOP)/,$(INCLDIRS))
INCLUDES	+= -I$(CODECDIR)

# ------------------------------------------------------------------------------
# Versioning
# ------------------------------------------------------------------------------

REV_FILE   = fh_arcatrade_revision.h

# ------------------------------------------------------------------------------
# Target - ARCA Book library
# ------------------------------------------------------------------------------

LIB = $(LIBDIR)/libfharcatrade.a

# ------------------------------------------------------------------------------
# Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FASTDIR) $(LIB)

$(LIB): $(REV_FILE) $(OBJS) $(ARCA_COMMON_OBJS)
	$(AR) rc $@ $(OBJS) $(ARCA_COMMON_OBJS)
	$(RANLIB) $@

$(ARCA_OBJS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(REV_FILE):
	@$(MKREVISION) $@

clean:
	rm -rf $(DIRS) $(REV_FILE)

dist rpmdist test:

# ------------------------------------------------------------------------------
# Build the object files
# ------------------------------------------------------------------------------

$(CODECDIR)/$(CODECOBJS) : $(CODECDIR)/$(CODESRCS)
	make -C $(CODECDIR)

$(ARCA_OBJS)/%.o : $(ARCA_COMMON)/%.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $(ARCA_OBJS)/$(@F) -c $<

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

-include $(DEPS)
