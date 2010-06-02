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
# Compile flags and includes
# ------------------------------------------------------------------------------

SHAREDDIR = $(TOP)/common
SHAREDLIB = $(SHAREDDIR)/$(LIBDIR)/libfh.a

INCLUDES  = -fPIC -I$(SHAREDDIR)

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: test_plugin.so test_program

test_plugin.so: test_plugin.o $(SHAREDLIB)
	$(CC) -o $@ test_plugin.o $(SHAREDLIB) $(LDFLAGS) -shared

test_program: test_program.o $(SHAREDLIB)
	$(CC) -o $@ test_program.o $(SHAREDLIB) $(LDFLAGS)

test_plugin.o: test_plugin.c
	$(CC) $(INCLUDES) -o $@ -c test_plugin.c

test_program.o: test_program.c
	$(CC) $(INCLUDES) -o $@ -c test_program.c

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

clean:
	rm -rf *.o test_program test_plugin.so
