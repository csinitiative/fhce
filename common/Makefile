#  This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
# 
#  CSI FH is free software: you can redistribute it and/or modify it under the terms of the
#  GNU Lesser General Public License as published by the Free Software Foundation, either version 3
#  of the License, or (at your option) any later version.
#  
#  CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
#  even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public License
#  along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.

TOP = ..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

CSRCS = $(wildcard *.c)
LSRCS = $(wildcard *.l)
YSRCS = $(wildcard *.y)

OBJS  = $(addprefix $(OBJDIR)/,$(CSRCS:.c=.o) $(YSRCS:.y=.o) $(LSRCS:.l=.o))
DEPS  = $(addprefix $(DEPDIR)/,$(CSRCS:.c=.P) $(YSRCS:.y=.P) $(LSRCS:.l=.P))

DIRS  = $(OBJDIR) $(LIBDIR) $(DEPDIR) $(SRCDIR)

# ------------------------------------------------------------------------------
# Target FH Common library
# ------------------------------------------------------------------------------

LIB = $(LIBDIR)/libfh.a

# ------------------------------------------------------------------------------
# Set YY_NO_INPUT to prevent a warning about "input" being unused when
# compiling Flex generated lexer
# ------------------------------------------------------------------------------

CFLAGS += -DYY_NO_INPUT

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(LIB)

$(LIB): $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

$(DIRS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o : %.l
	$(LEX) -o$(SRCDIR)/$(<:.l=.c) -c $<
	$(CC) $(CFLAGS) -o $@ -c $(SRCDIR)/$(<:.l=.c)
	
$(OBJDIR)/%.o : %.y
	$(YACC) -d -o $(SRCDIR)/$(<:.y=.c) $<
	$(CC) $(CFLAGS) -o $@ -c $(SRCDIR)/$(<:.y=.c)
	
clean:
	rm -rf $(DIRS)
	$(MAKE) -C test $@

dist:

rpmdist:

test: FORCE
	$(MAKE) -C test all

-include $(DEPS)
