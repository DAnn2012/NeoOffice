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

PRJ=..$/..

PRJNAME=dtrans
TARGET=dtransjava
TARGETTYPE=CUI

ENABLE_EXCEPTIONS=TRUE
COMP1TYPELIST=$(TARGET)
LIBTARGET=NO

ENVCDEFS += -Iinc

.IF "$(GUIBASE)"=="java"
# Link to modified $(VCLLIB)
SOLARLIB:=-L$(PRJ)$/..$/vcl$/$(INPATH)$/lib $(SOLARLIB)
.ENDIF

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

# ------------------------------------------------------------------

.IF "$(GUIBASE)"!="java"

dummy:
	@echo "Nothing to build for GUIBASE $(GUIBASE)"
 
.ELSE		# "$(GUIBASE)"!="java"

SLOFILES=\
	$(SLO)$/DTransClipboard.obj \
	$(SLO)$/DTransTransferable.obj \
	$(SLO)$/HtmlFmtFlt.obj \
	$(SLO)$/java_clipboard.obj \
	$(SLO)$/java_dnd.obj \
	$(SLO)$/java_dndcontext.obj \
	$(SLO)$/java_service.obj

SHL1TARGET= $(TARGET)$(DLLPOSTFIX)

SHL1STDLIBS= \
		$(SALLIB)	\
		$(TOOLSLIB)	\
		$(VCLLIB)	\
		$(VOSLIB)	\
		$(CPPULIB) 	\
		$(CPPUHELPERLIB)	\
		$(COMPHELPERLIB)	\
		-framework AppKit

SHL1DEPN=
SHL1IMPLIB=		i$(SHL1TARGET) 
SHL1OBJS=		$(SLOFILES)

APP1NOSAL=TRUE
APP1TARGET=testjavacb
APP1OBJS=$(SLO)$/test_javacb.obj
APP1STDLIBS=\
		$(SALLIB)	\
		$(TOOLSLIB)	\
		$(CPPULIB)			\
		$(CPPUHELPERLIB)	\
		$(COMPHELPERLIB)	\
		$(VCLLIB)	\
		-framework QuickTime

.ENDIF		# "$(GUIBASE)"!="java"

# --- Targets ------------------------------------------------------

.INCLUDE :	target.mk
