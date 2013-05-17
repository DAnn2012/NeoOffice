##########################################################################
# 
#   $RCSfile$
# 
#   $Revision$
# 
#   last change: $Author$ $Date$
# 
#   The Contents of this file are made available subject to the terms of
#   either of the following licenses
# 
#          - GNU General Public License Version 2.1
# 
#   Patrick Luby, June 2003
# 
#   GNU General Public License Version 2.1
#   =============================================
#   Copyright 2003 Planamesa Inc.
# 
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public
#   License version 2.1, as published by the Free Software Foundation.
# 
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
# 
#   You should have received a copy of the GNU General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#   MA  02111-1307  USA
# 
##########################################################################

PRJ=..$/..$/..

PRJNAME=vcl
TARGET=salapp

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

# --- Files --------------------------------------------------------

.IF "$(GUIBASE)"!="java"

dummy:
	@echo "Nothing to build for GUIBASE $(GUIBASE)"

.ELSE		# "$(GUIBASE)"!="java"

SLOFILES= \
	$(SLO)$/saldata.obj \
	$(SLO)$/saltimer.obj \
	$(SLO)$/salinfo.obj \
	$(SLO)$/salinst.obj \
	$(SLO)$/salinst_cocoa.obj \

SRC1FILES=salinst.src
SRS1NAME=$(TARGET)
RESLIB1NAME=$(TARGET)
RESLIB1SRSFILES=$(SRS)$/salapp.srs

.ENDIF		# "$(GUIBASE)"!="java"

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk

.INCLUDE :  $(PRJ)$/util$/target.pmk
