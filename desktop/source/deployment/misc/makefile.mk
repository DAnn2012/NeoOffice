#*************************************************************************
#
# Copyright 2008 by Sun Microsystems, Inc.
#
# $RCSfile$
#
# $Revision$
#
# This file is part of NeoOffice.
#
# NeoOffice is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3
# only, as published by the Free Software Foundation.
#
# NeoOffice is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU General Public License
# version 3 along with NeoOffice.  If not, see
# <http://www.gnu.org/licenses/gpl-3.0.txt>
# for a copy of the GPLv3 License.
#
# Modified June 2007 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************

PRJ = ..$/..$/..

PRJNAME = desktop
TARGET = deployment_misc
USE_DEFFILE = TRUE
ENABLE_EXCEPTIONS = TRUE
VISIBILITY_HIDDEN=TRUE

.IF "$(GUI)"=="OS2"
TARGET = deplmisc
.ENDIF

.INCLUDE : settings.mk

.IF "$(PRODUCT_NAME)" != ""
CDEFS += -DPRODUCT_NAME='"$(PRODUCT_NAME)"'
.ENDIF

# Reduction of exported symbols:
CDEFS += -DDESKTOP_DEPLOYMENTMISC_DLLIMPLEMENTATION

.IF "$(SYSTEM_DB)" == "YES"
CFLAGS+=-DSYSTEM_DB -I$(DB_INCLUDES)
.ENDIF

SRS1NAME = $(TARGET)
SRC1FILES = \
	dp_misc.src

.IF "$(GUI)"=="OS2"
SHL1TARGET = $(TARGET)
.ELSE
SHL1TARGET = deploymentmisc$(DLLPOSTFIX)
.ENDIF
SHL1OBJS = \
        $(SLO)$/dp_misc.obj \
        $(SLO)$/dp_resource.obj \
        $(SLO)$/dp_identifier.obj \
        $(SLO)$/dp_interact.obj \
        $(SLO)$/dp_ucb.obj \
        $(SLO)$/db.obj \
        $(SLO)$/dp_version.obj \
        $(SLO)$/dp_descriptioninfoset.obj \
        $(SLO)$/dp_dependencies.obj \
        $(SLO)$/dp_platform.obj
        
SHL1STDLIBS = \
    $(BERKELEYLIB) \
    $(CPPUHELPERLIB) \
    $(CPPULIB) \
    $(SALLIB) \
    $(TOOLSLIB) \
    $(UCBHELPERLIB) \
    $(UNOTOOLSLIB) \
    $(XMLSCRIPTLIB)
.IF "$(GUI)"=="OS2"
SHL1IMPLIB = ideploymentmisc$(DLLPOSTFIX)
LIB1TARGET = $(SLB)$/_deplmisc.lib
LIB1OBJFILES = $(SHL1OBJS)
DEFLIB1NAME = _deplmisc
.ELSE
SHL1IMPLIB = i$(SHL1TARGET)
.ENDIF
DEF1NAME = $(SHL1TARGET)

SLOFILES = $(SHL1OBJS)

.INCLUDE : ..$/target.pmk
.INCLUDE : target.mk

