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
#     Modified December 2005 by Patrick Luby. NeoOffice is distributed under
#     GPL only under modification term 3 of the LGPL.
#
#*************************************************************************
PRJ=..$/..

PRJNAME=			framework
TARGET=				fwk_dispatch
USE_DEFFILE=		TRUE
ENABLE_EXCEPTIONS=	TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  		settings.mk

# --- defines ------------------------------------------------------

CDEFS+=-DCOMPMOD_NAMESPACE=framework

.IF "$(GUIBASE)"=="java"
CDEFS+=-DDLLPOSTFIX=$(DLLPOSTFIX)
.ENDIF		# "$(GUIBASE)"=="java"

# --- Generate -----------------------------------------------------

SLOFILES=			$(SLO)$/interceptionhelper.obj				\
					$(SLO)$/dispatchprovider.obj				\
					$(SLO)$/dispatchinformationprovider.obj	    \
                    $(SLO)$/loaddispatcher.obj                  \
					$(SLO)$/menudispatcher.obj					\
					$(SLO)$/helpagentdispatcher.obj				\
					$(SLO)$/mailtodispatcher.obj				\
					$(SLO)$/soundhandler.obj					\
					$(SLO)$/interaction.obj						\
                    $(SLO)$/servicehandler.obj                  \
                    $(SLO)$/closedispatcher.obj                 \
                    $(SLO)$/systemexec.obj						\
                    $(SLO)$/popupmenudispatcher.obj

#                    $(SLO)$/basedispatcher.obj                  \
#                    $(SLO)$/blankdispatcher.obj                 \
#                    $(SLO)$/createdispatcher.obj                \
#                    $(SLO)$/selfdispatcher.obj                  \

# --- Targets ------------------------------------------------------

.INCLUDE :			target.mk
