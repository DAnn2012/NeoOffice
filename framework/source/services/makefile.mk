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
# Modified January 2008 by Patrick Luby. NeoOffice is distributed under
# GPL only under modification term 2 of the LGPL.
#
#*************************************************************************
PRJ=..$/..

PRJNAME=			framework
TARGET=				fwk_services
USE_DEFFILE=		TRUE
ENABLE_EXCEPTIONS=	TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  		settings.mk

.IF "$(GUIBASE)"=="java"
CDEFS+=-DDLLPOSTFIX=$(DLLPOSTFIX)
.ENDIF      # "$(GUIBASE)"=="java"

# --- Generate -----------------------------------------------------

SLOFILES=\
        $(SLO)$/desktop.obj \
        $(SLO)$/frame.obj \
        $(SLO)$/urltransformer.obj \
        $(SLO)$/mediatypedetectionhelper.obj \
        $(SLO)$/substitutepathvars.obj \
        $(SLO)$/pathsettings.obj \
        $(SLO)$/backingcomp.obj \
        $(SLO)$/backingwindow.obj \
        $(SLO)$/dispatchhelper.obj \
        $(SLO)$/license.obj \
        $(SLO)$/modulemanager.obj \
        $(SLO)$/autorecovery.obj \
        $(SLO)$/sessionlistener.obj \
        $(SLO)$/taskcreatorsrv.obj \
        $(SLO)$/uriabbreviation.obj \
        $(SLO)$/tabwindowservice.obj

SRS1NAME=$(TARGET)
SRC1FILES= fwk_services.src

# --- Targets ------------------------------------------------------

.INCLUDE :			target.mk

