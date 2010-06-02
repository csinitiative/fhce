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

# ------------------------------------------------------------------------------
# Include the main makefile includes
# ------------------------------------------------------------------------------

TOP = ../../../../..
include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Variables related to target code
# ------------------------------------------------------------------------------

COMMONDIR		= $(TOP)/common
COMMONLIB		= $(COMMONDIR)/$(LIBDIR)/libfh.a

SHRCFGDIR		= ../..
SHRCFGLIB		= $(SHRCFGDIR)/$(LIBDIR)/libfhconfig.a

TARGETDIRS		= $(SHRCFGDIR) $(COMMONDIR)
TARGETLIBS		= $(SHRCFGLIB) $(COMMONLIB)

$(COMMONLIB): FORCE
	$(MAKE) -C $(COMMONDIR)

$(SHRCFGLIB): FORCE
	$(MAKE) -C $(SHRCFGDIR)
	
# ------------------------------------------------------------------------------
# Include the test makefile includes
# ------------------------------------------------------------------------------
	
include $(TOP)/build/test.mk
