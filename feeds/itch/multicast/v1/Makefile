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

TOP = ../../../..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

SRCS   			 = $(wildcard *.c)
OBJS   			 = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS   			 = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

FH_BIN 			 = $(BINDIR)/fhitch
FH_CFG   		 = ../etc/itch.conf

INSTDIR			:= $(INSTDIR)/itch
INSTBINDIR    	 = $(INSTDIR)/bin/
INSTETCDIR    	 = $(INSTDIR)/etc/
INSTPLGDIR    	 = $(INSTDIR)/plugins/

DIRS   			 = $(OBJDIR) $(BINDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Tar "stuff"
# ------------------------------------------------------------------------------

TAROUT   = $(RPMDIR)/SOURCES/csifh-itchv1-$(VERSION).tar.gz
TARBASE  = $(notdir $(shell cd $(TOP) && pwd))
TARFILES = build common mgmt msg scripts feeds/itch feeds/shared

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

ITCHDIR  	 		= ../common
ITCHLIB  	 		= $(ITCHDIR)/$(LIBDIR)/libfhitch.a

SHAREDDIR	 		= $(TOP)/common
SHAREDLIB	 		= $(SHAREDDIR)/$(LIBDIR)/libfh.a

SHRCFGDIR 	 		= $(TOP)/feeds/shared/config
SHRCFGLIB 	 		= $(SHRCFGDIR)/$(LIBDIR)/libfhconfig.a

SHRMMCASTDIR 		= $(TOP)/feeds/shared/mirrored_mcast
SHRMMCASTLIB 		= $(SHRMMCASTDIR)/$(LIBDIR)/libfhmmcast.a

SHRMGMTTHREADDIR 	= $(TOP)/feeds/shared/mgmt_thread
SHRMGMTTHREADLIB 	= $(SHRMGMTTHREADDIR)/$(LIBDIR)/libfhmgmtthread.a

SHRLHDIR		 	= $(TOP)/feeds/shared/line_handler
SHRLHLIB		 	= $(SHRLHDIR)/$(LIBDIR)/libfhlh.a

SHRLKPDIR		 	= $(TOP)/feeds/shared/lookup_tables
SHRLKPLIB		 	= $(SHRLKPDIR)/$(LIBDIR)/libfhlookup.a

SHRGAPDIR		 	= $(TOP)/feeds/shared/gap_mgmt
SHRGAPLIB		 	= $(SHRGAPDIR)/$(LIBDIR)/libfhgap.a

MGMTDIR				= $(TOP)/mgmt/lib
MGMTLIB				= $(MGMTDIR)/$(LIBDIR)/libfhmgmt.a

MSGDIR 				= $(TOP)/msg
MSGLIB 				= $(MSGDIR)/$(LIBDIR)/libfhmsg.a

FH_LIBS   			= $(ITCHLIB) $(SHRMMCASTLIB) $(SHRMGMTTHREADLIB) $(SHRCFGLIB)
FH_LIBS			   += $(SHRLHLIB) $(SHRLKPLIB) $(SHRGAPLIB) $(MSGLIB) $(MGMTLIB) $(SHAREDLIB)


# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES  = -I$(SHAREDDIR) -I$(SHAREDDIR)/missing -I$(ITCHDIR)

# ------------------------------------------------------------------------------
# --- Generic make targets
# ------------------------------------------------------------------------------

all: $(DIRS) $(FH_BIN)

$(FH_BIN): $(OBJS) $(FH_LIBS)
	$(CC) -o $@ $(OBJS) $(FH_LIBS) $(LDFLAGS)

$(SHAREDLIB): FORCE
	@$(MAKE) -C $(SHAREDDIR) all

$(SHRCFGLIB): FORCE
	@$(MAKE) -C $(SHRCFGDIR) all

$(SHRMMCASTLIB): FORCE
	@$(MAKE) -C $(SHRMMCASTDIR) all

$(SHRMGMTTHREADLIB): FORCE
	@$(MAKE) -C $(SHRMGMTTHREADDIR) all

$(SHRLHLIB): FORCE
	@$(MAKE) -C $(SHRLHDIR) all

$(SHRLKPLIB): FORCE
	@$(MAKE) -C $(SHRLKPDIR) all

$(SHRGAPLIB): FORCE
	@$(MAKE) -C $(SHRGAPDIR) all

$(ITCHLIB): FORCE
	@$(MAKE) -C $(ITCHDIR) all

$(MGMTLIB): FORCE
	@$(MAKE) -C $(MGMTDIR) all

$(MSGLIB): FORCE
	@$(MAKE) -C $(MSGDIR) all

$(DIRS):
	@if [ ! -d $@ ]; then mkdir -p $@; fi

# ------------------------------------------------------------------------------
# --- Build the object files
# ------------------------------------------------------------------------------

$(OBJDIR)/%.o : %.c
	@$(MAKEDEPEND)
	$(CC) $(CFLAGS) -o $@ -c $<

dist: all
	install $(INSTFLAGS) -d $(INSTBINDIR)
	install $(INSTFLAGS) -d $(INSTETCDIR)
	install $(INSTFLAGS) -d $(INSTPLGDIR)
	install $(INSTFLAGS) $(FH_BIN)    $(INSTBINDIR)
	@if [ ! -f $(INSTETCDIR)$(FH_CFG) ]; then                   \
		install $(INSTFLAGS) $(FH_CFG) $(INSTETCDIR);           \
		echo "install $(INSTFLAGS) $(FH_CFG) $(INSTETCDIR)";	\
	fi

tar: requireversion $(RPMDIRS)
	@echo "Generating itchv1 package source tarball..."
	@$(TAR) $(TARFLAGS) -C $(TOP)/.. -f $(TAROUT) $(addprefix $(TARBASE)/,$(TARFILES))

spec: $(RPMDIRS)
	@echo "Copying spec file to RPM build directory..."
	@cp -f $(TOP)/rpm/csifh-itchv1.spec $(RPMDIR)/SPECS/

rpmdist: tar spec $(RPMDIRS)
	rpmbuild $(RPMFLAGS) $(RPMDIR)/SPECS/csifh-itchv1.spec

clean:
	rm -rf $(DIRS)
	$(MAKE) -C test $@

test: FORCE
	$(MAKE) -C test all

-include $(DEPS)
