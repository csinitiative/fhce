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
# Global "stuff"
# ------------------------------------------------------------------------------

TOP = .

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Clean locally and pass all major targets on to subdirectories
# ------------------------------------------------------------------------------

SUBDIRS  = common msg feeds mgmt
TESTDIRS = test common feeds
ALLDIRS	 = $(sort $(SUBDIRS) $(TESTDIRS))

all dist rpmdist:
	@for dir in $(SUBDIRS); do   \
		$(MAKE) -C $$dir $@;     \
	done

clean:
	rm -rf $(DISTDIR) $(RPMDIR)
	@for dir in $(ALLDIRS); do   \
		$(MAKE) -C $$dir $@;     \
	done

test: FORCE
	@for dir in $(TESTDIRS); do  \
		$(MAKE) -C $$dir $@;     \
	done
