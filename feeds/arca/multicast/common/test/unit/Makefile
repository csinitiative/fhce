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

# ------------------------------------------------------------------------------
# Include the main makefile includes
# ------------------------------------------------------------------------------

TOP = ../../../../../..
include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Variables related to target code
# ------------------------------------------------------------------------------

COMMONDIR		= $(TOP)/common
COMMONLIB		= $(COMMONDIR)/$(LIBDIR)/libfh.a

ARCACOMMONDIR	= ../..
ARCACOMMONLIB	= $(ARCACOMMONDIR)/$(LIBDIR)/libfharca.a

$(COMMONLIB): FORCE
	$(MAKE) -C $(COMMONDIR)

$(ARCACOMMONLIB): FORCE
	$(MAKE) -C $(ARCACOMMONDIR)

TARGETDIRS		= $(ARCACOMMONDIR) $(COMMONDIR)
TARGETLIBS		= $(ARCACOMMONLIB) $(COMMONLIB)

# ------------------------------------------------------------------------------
# Include the test makefile includes
# ------------------------------------------------------------------------------

include $(TOP)/build/test.mk

