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
# Macro definition
# ------------------------------------------------------------------------------

%define instdir /opt/csi/fh/itch
%define makedir feeds/itch/multicast/v1
%define plugdir plugins/sample/itch


# ------------------------------------------------------------------------------
# Introduction section
# ------------------------------------------------------------------------------

summary: TotalView-ITCH 3.1 feed handler component of the FeedHandler suite
name: csifh-itchv1
requires: csifh-core >= %{version}
requires: expect
provides: csifh-itch
version: %{version}
release: 1
group: Feed Handlers
vendor: Collaborative Software Foundation
url: http://openmarketdata.org
license: LGPLv3
source: %{name}-%{version}.tar.gz
%description
The Collaborative Software Foundation Feed Handler is an open market data
feed handler. It is designed to be very fast while simultaneously being
flexible and extensible.

This package contains the ITCH v1 feed handler(s).


# ------------------------------------------------------------------------------
# Prep section (prepare source for build/packaging)
# ------------------------------------------------------------------------------

%prep
%setup -n %{tgzroot}


# ------------------------------------------------------------------------------
# Build section
# ------------------------------------------------------------------------------

%build
make -C %{makedir}
make -C %{plugdir}


# ------------------------------------------------------------------------------
# Install section
# ------------------------------------------------------------------------------

%install
INSTROOT=%{buildroot} make -C %{makedir} dist
INSTROOT=%{buildroot} make -C %{plugdir} dist


# ------------------------------------------------------------------------------
# Clean section
# ------------------------------------------------------------------------------

%clean
rm -rf $RPM_BUILD_ROOT


# ------------------------------------------------------------------------------
# Files section
# ------------------------------------------------------------------------------

%files
%{instdir}/bin
%config %{instdir}/etc
%dir %{instdir}/plugins


# ------------------------------------------------------------------------------
# Post-install script
# ------------------------------------------------------------------------------

%post



# ------------------------------------------------------------------------------
# Pre-uninstall script
# ------------------------------------------------------------------------------

%preun
rm -rf %{instdir}/bin/fhitch
