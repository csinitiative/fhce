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
# Global "stuff"
# ------------------------------------------------------------------------------

TOP = ..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Distribution "stuff"
# ------------------------------------------------------------------------------

PKGFILES = build common mgmt msg scripts
PKGNAME  = core

# ------------------------------------------------------------------------------
# Pass major targets along to subdirs (except RPM building)
# ------------------------------------------------------------------------------

SUBDIRS = cli lib fhctl fhmgr

all clean dist:
	@for dir in $(SUBDIRS); do  \
		$(MAKE) -C $$dir $@;    \
	done

# include packaging targets
include $(TOP)/build/dist.mk
