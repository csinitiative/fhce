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
# Macro definition
# ------------------------------------------------------------------------------

%define makedir mgmt


# ------------------------------------------------------------------------------
# Introduction section
# ------------------------------------------------------------------------------

summary: Common components of the CSI FH feed handler suite
name: csifh-core
version: %{version}
release: 1
group: Feed Handlers
vendor: Collaborative Software Initiative
url: https://csi-fh.csinitiative.net
license: LGPLv3
source: %{name}-%{version}.tar.gz
%description
The Collaborative Software Initiative Feed Handler is an open market data
feed handler. It is designed to be very fast while simultaneously being
flexible and extensible.

This package contains core functionality needed by all other CSI FH packages.


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


# ------------------------------------------------------------------------------
# Install section
# ------------------------------------------------------------------------------

%install
INSTROOT=%{buildroot} make -C %{makedir} dist


# ------------------------------------------------------------------------------
# Clean section
# ------------------------------------------------------------------------------

%clean
rm -rf $RPM_BUILD_ROOT


# ------------------------------------------------------------------------------
# Files section
# ------------------------------------------------------------------------------

%files
/opt/csi/fh/mgmt/bin
%config /opt/csi/fh/mgmt/etc
%dir /opt/csi/fh/mgmt/plugins
