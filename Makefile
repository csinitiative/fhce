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

TOP = .

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Clean locally and pass all major targets on to subdirectories
# ------------------------------------------------------------------------------

SUBDIRS   = common msg feeds mgmt
TESTDIRS  = test common feeds
ALLDIRS	  = $(sort $(SUBDIRS) $(TESTDIRS))
DISTDIRS  = mgmt feeds/itch/multicast/v1 feeds/bats/multicast/v1 feeds/directedge/v1
DISTDIRS += feeds/opra/fast/v2 feeds/arca/multicast/v1 feeds/arca/trade/v1

all:
	@for dir in $(SUBDIRS); do   \
		$(MAKE) -C $$dir $@;     \
	done

dist tar rpmdist:
	@for dir in $(DISTDIRS); do  \
		$(MAKE) -C $$dir $@;     \
	done

clean:
	rm -rf $(DISTDIR) $(TARDIR) $(RPMDIR)
	@for dir in $(ALLDIRS); do   \
		$(MAKE) -C $$dir $@;     \
	done

test: FORCE
	@for dir in $(TESTDIRS); do  \
		$(MAKE) -C $$dir $@;     \
	done
