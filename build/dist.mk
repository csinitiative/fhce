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
# Targets used for distribution/packaging
# ------------------------------------------------------------------------------

$(TARDIR):
	@if [ ! -d $(TOP)/$@ ]; then mkdir -p $(TOP)/$@; fi

filecopy: $(TARDIR)
	@echo --\> copying all relevant files to dist directory
	@if [ ! -d $(TOP)/$(TARDIR) ]; then mkdir -p $(TOP)/$(TARDIR); fi
	@rm -rf $(TOP)/$(TARDIR)/fhce
	@cd $(TOP) && for dir in $(PKGFILES); do												\
		find $$dir -type d $(FINDFLAGS) -print | xargs -I %% mkdir -p $(TARDIR)/fhce/%%;	\
		find $$dir $(FINDFLAGS) ! -type d -print | xargs -I %% cp %% $(TARDIR)/fhce/%%;		\
	done

tar: filecopy requireversion $(RPMDIRS)
	@echo --\> generating $(PKGNAME) package source tarball
	@$(TAR) $(TARFLAGS) -C $(TOP)/$(TARDIR) -f 											\
		$(RPMDIR)/SOURCES/csifh-$(PKGNAME)-$(VERSION).tar.gz fhce

spec: $(RPMDIRS)
	@echo --\> copying $(PKGNAME) spec file to RPM build directory
	@cp -f $(TOP)/rpm/csifh-$(PKGNAME).spec $(RPMDIR)/SPECS/

rpmdist: tar spec $(RPMDIRS)
	rpmbuild $(RPMFLAGS) $(RPMDIR)/SPECS/csifh-$(PKGNAME).spec