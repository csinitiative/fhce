#  This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
#
#  CSI FH is free software: you can redistribute it and/or modify it under the terms of the
#  GNU General Public License as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#
#  CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.

TOP = ../../..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Set up directory/target variables
# ------------------------------------------------------------------------------

DIRS 		 = $(OBJDIR) $(LIBDIR) $(DEPDIR)
INCLDIRS	 = common mgmt/lib mgmt/lib/admin feeds/shared/config feeds/shared/tcp_feed
INCLDIRS	+= feeds/shared/lookup_tables feeds/shared/mgmt_thread

# Basic sources and targets
SRCS      	 = $(wildcard *.c)
OBJS      	 = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS 		 = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES 	 = $(addprefix -I$(TOP)/,$(INCLDIRS))

# ------------------------------------------------------------------------------
# Target - Direct Egde Common library
# ------------------------------------------------------------------------------

LIB = $(LIBDIR)/libfhtcplh.a

# ------------------------------------------------------------------------------
# Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FASTDIR) $(LIB)

$(LIB): $(REV_FILE) $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

clean:
	rm -rf $(DIRS)

test:

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

