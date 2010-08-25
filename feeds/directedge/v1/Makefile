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

TOP = ../../..

include $(TOP)/build/defs.mk

# ------------------------------------------------------------------------------
# Targets
# ------------------------------------------------------------------------------

SRCS   			 = $(wildcard *.c)
OBJS   			 = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
DEPS   			 = $(addprefix $(DEPDIR)/,$(SRCS:.c=.P))

FH_BIN 			 = $(BINDIR)/fhedge
FH_CFG   		 = ../etc/directedge.conf

INSTDIR			:= $(INSTDIR)/directedge
INSTBINDIR    	 = $(INSTDIR)/bin/
INSTETCDIR    	 = $(INSTDIR)/etc/
INSTPLGDIR    	 = $(INSTDIR)/plugins/

DIRS   			 = $(OBJDIR) $(BINDIR) $(DEPDIR)

# ------------------------------------------------------------------------------
# Distribution "stuff"
# ------------------------------------------------------------------------------

PKGFILES = build common mgmt msg scripts feeds/directedge feeds/shared plugins/sample/directedge
PKGNAME  = directedgev1

# ------------------------------------------------------------------------------
# Linked libraries
# ------------------------------------------------------------------------------

DIREDGEDIR  		= ../common
DIREDGELIB  		= $(DIREDGEDIR)/$(LIBDIR)/libfhedge.a

SHAREDDIR	 		= $(TOP)/common
SHAREDLIB	 		= $(SHAREDDIR)/$(LIBDIR)/libfh.a

SHRCFGDIR 	 		= $(TOP)/feeds/shared/config
SHRCFGLIB 	 		= $(SHRCFGDIR)/$(LIBDIR)/libfhconfig.a

SHRMGMTTHREADDIR	= $(TOP)/feeds/shared/mgmt_thread
SHRMGMTTHREADLIB	= $(SHRMGMTTHREADDIR)/$(LIBDIR)/libfhmgmtthread.a

SHRLHDIR			= $(TOP)/feeds/shared/tcp_feed
SHRLHLIB			= $(SHRLHDIR)/$(LIBDIR)/libfhtcplh.a

SHRLKPDIR			= $(TOP)/feeds/shared/lookup_tables
SHRLKPLIB			= $(SHRLKPDIR)/$(LIBDIR)/libfhlookup.a

MGMTDIR				= $(TOP)/mgmt/lib
MGMTLIB				= $(MGMTDIR)/$(LIBDIR)/libfhmgmt.a

MSGDIR 				= $(TOP)/msg
MSGLIB 				= $(MSGDIR)/$(LIBDIR)/libfhmsg.a

FH_LIBS   			= $(DIREDGELIB) $(SHRLHLIB) $(SHRMGMTTHREADLIB) $(SHRCFGLIB)
FH_LIBS			   += $(SHRLKPLIB) $(MSGLIB) $(MGMTLIB) $(SHAREDLIB)

# ------------------------------------------------------------------------------
# Compile flags and includes
# ------------------------------------------------------------------------------

INCLUDES  = -I$(SHAREDDIR) -I$(SHAREDDIR)/missing -I$(DIREDGEDIR)

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

$(DIREDGELIB): FORCE
	@$(MAKE) -C $(DIREDGEDIR) all

$(MGMTLIB): FORCE
	@$(MAKE) -C $(MGMTDIR) all

$(MSGLIB): FORCE
	@$(MAKE) -C $(MSGDIR) all

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

# include packaging targets
include $(TOP)/build/dist.mk

clean:
	rm -rf $(DIRS)

test:

-include $(DEPS)
