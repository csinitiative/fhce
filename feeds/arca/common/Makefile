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
# Set up variables
# ------------------------------------------------------------------------------
DIRS             = $(OBJDIR) $(LIBDIR) $(DEPDIR)

CODECDIR  = $(TOP)/feeds/arca/codec

LIB = $(LIBDIR)/libfharcacommon.a

#include directories reqd for build
INCLDIRS         = common common/missing mgmt/lib msg
INCLUDES         = $(addprefix -I$(TOP)/,$(INCLDIRS))
INCLUDES         += -I$(CODECDIR)
#the source files
SRCS             = $(wildcard *.c)
#an object file in objdir for each source file that exists
OBJS             = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))

# ------------------------------------------------------------------------------
# Default make rule; the default is to make a library in lib_dir
#   the default library includes all the objects made from these src files
#   This directory is shared by all arca objects and this library is not
#   distributed. Its only purpose is to force a clean source build/link
# ------------------------------------------------------------------------------

$(LIB): $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@


# ------------------------------------------------------------------------------
# Define named targets with their make rules
# ------------------------------------------------------------------------------

all: $(DIRS) $(LIB)

clean:
	rm -rf $(DIRS)

test:

# ------------------------------------------------------------------------------
# Define explicit make rules
# ------------------------------------------------------------------------------

# rule to make an obj in obj_dir for each src file in this dir
$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

-include $(DEPS)
