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

CC = gcc
#CFLAGS = -Wall -g
#turn off warnings; maximize optimization
CFLAGS = -O3
PROGRAM = AB_Fast_opt
SRCS = AB_Fast.c newFastDecode.c
HDRS = 	AB_Fast.h ArcaL2Msg.h
OBJS = AB_Fast.o newFastDecode.o
LIBS = -lrt -lpthread -L/usr/local/lib

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS)  -o $(PROGRAM) $(LIBS)

clean:
	- rm -f $(PROGRAM) $(OBJS) *.o *.P core

install: all

test:

