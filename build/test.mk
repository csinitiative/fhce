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

# ------------------------------------------------------------------------------
# Set up some common strings that will be used in later makefiles
# ------------------------------------------------------------------------------

SRCS		:= $(wildcard *.c)

OBJS		:= $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
BINS		:= $(addprefix $(BINDIR)/,$(patsubst %_test.c,%.test,$(SRCS)))

DIRS		:= $(OBJDIR) $(BINDIR)

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

TESTDIR 	 = $(TOP)/test/unit/lib
TESTLIB		 = $(TESTDIR)/$(LIBDIR)/libfhtest.a

LIBS		 = $(TESTLIB) $(TARGETLIBS)

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

# remove optimization flags since that will mess with debugging information and
# increase debugging level to the maximum
CFLAGS	 	:= $(patsubst -O%,,$(CFLAGS))
CFLAGS	 	:= $(patsubst -D__OPTIMIZE__,,$(CFLAGS))
CFLAGS	 	:= $(patsubst -g,-g3,$(CFLAGS))

CFLAGS		+= $(addprefix -I,$(TARGETDIRS)) -I$(TESTDIR)
LDFLAGS		+= -lbfd -liberty

# ------------------------------------------------------------------------------
# Rules to build tests
# ------------------------------------------------------------------------------

all: $(LIBS) $(DIRS) $(BINS)

$(BINDIR)/%.test: $(OBJDIR)/%_test.o
	$(CC) $< $(LIBS) $(LDFLAGS) -o $@

$(OBJDIR)/%_test.o : %_test.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(DIRS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

clean: FORCE
	rm -rf $(DIRS)
	
.PRECIOUS: $(LIBS)

.SECONDARY: $(OBJS)
	
# ------------------------------------------------------------------------------
# Rules to build library dependencies
# ------------------------------------------------------------------------------

$(TESTLIB): FORCE
	@$(MAKE) -C $(TESTDIR)
