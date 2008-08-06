#*************************************************************************
#
#   $RCSfile$
#
#   $Revision$
#
#   last change: $Author$ $Date$
#
#   The Contents of this file are made available subject to
#   the terms of GNU General Public License Version 2.1.
#
#
#     GNU General Public License Version 2.1
#     =============================================
#     Copyright 2005 by Sun Microsystems, Inc.
#     901 San Antonio Road, Palo Alto, CA 94303, USA
#
#     This library is free software; you can redistribute it and/or
#     modify it under the terms of the GNU General Public
#     License version 2.1, as published by the Free Software Foundation.
#
#     This library is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#     General Public License for more details.
#
#     You should have received a copy of the GNU General Public
#     License along with this library; if not, write to the Free Software
#     Foundation, Inc., 59 Temple Place, Suite 330, Boston,
#     MA  02111-1307  USA
#
#*************************************************************************

PRJ=..

PRJNAME=	ucbhelper
TARGET=		ucbhelper

# --- Settings -----------------------------------------------------

.INCLUDE :	settings.mk
.INCLUDE :	..$/version.mk

# --- Files --------------------------------------------------------

LIB1TARGET=	$(SLB)$/$(TARGET).lib
LIB1FILES=	$(SLB)$/client.lib \
		$(SLB)$/provider.lib

SHL1TARGET=	$(TARGET)$(UCBHELPER_MAJOR)$(COMID)
SHL1STDLIBS = \
    $(CPPUHELPERLIB) \
    $(CPPULIB) \
    $(SALHELPERLIB) \
    $(SALLIB) \
    $(VOSLIB)

.IF "$(GUIBASE)"=="java"
SHL1STDLIBS += -framework SystemConfiguration
.ENDIF		# "$(GUIBASE)"=="java"

SHL1DEPN=
SHL1IMPLIB=	i$(TARGET)
SHL1USE_EXPORTS=ordinal
SHL1LIBS=	$(LIB1TARGET)
SHL1DEF=	$(MISC)$/$(SHL1TARGET).def

DEF1NAME=	$(SHL1TARGET)
DEF1DEPN=	$(MISC)$/$(SHL1TARGET).flt
DEFLIB1NAME=	$(TARGET)
DEF1DES=	Universal Content Broker - Helpers

# --- Targets ------------------------------------------------------

.INCLUDE :	target.mk

# --- Filter -----------------------------------------------------------

$(MISC)$/$(SHL1TARGET).flt : ucbhelper.flt
	@echo ------------------------------
	@echo Making: $@
	@+$(TYPE) ucbhelper.flt > $@

