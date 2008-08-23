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
#     Modified August 2008 by Patrick Luby. NeoOffice is distributed under
#     GPL only under modification term 3 of the LGPL.
#
#*************************************************************************

PRJ=..$/..
PRJNAME=filter
TARGET=pdffilter
GEN_HID=TRUE

ENABLE_EXCEPTIONS=TRUE

# --- Settings ----------------------------------

.INCLUDE : settings.mk

.IF "$(GUIBASE)" == "java"
CDEFS+=-DDLLPOSTFIX=$(DLLPOSTFIX)
.ENDIF		# "$(GUIBASE)" == "java"

# --- Files -------------------------------------

SRS1NAME=$(TARGET)
SRC1FILES =	impdialog.src				\
			pdf.src

SLOFILES=	$(SLO)$/pdfuno.obj			\
			$(SLO)$/pdfdialog.obj		\
			$(SLO)$/impdialog.obj		\
			$(SLO)$/pdffilter.obj		\
			$(SLO)$/pdfexport.obj		

# --- Library -----------------------------------

RESLIB1NAME=$(TARGET)
RESLIB1SRSFILES= $(SRS)$/$(TARGET).srs

SHL1TARGET=$(TARGET)$(UPD)$(DLLPOSTFIX)

SHL1STDLIBS=\
	$(SVTOOLLIB)		\
	$(TKLIB)			\
	$(VCLLIB)			\
	$(SVLLIB)			\
	$(SFX2LIB)			\
	$(UNOTOOLSLIB)		\
	$(TOOLSLIB)			\
	$(COMPHELPERLIB)	\
	$(CPPUHELPERLIB)	\
	$(CPPULIB)			\
	$(SALLIB)

SHL1DEPN=
SHL1IMPLIB=	i$(SHL1TARGET)
SHL1LIBS=	$(SLB)$/$(TARGET).lib
SHL1DEF=	$(MISC)$/$(SHL1TARGET).def
SHL1VERSIONMAP=exports.map

DEF1NAME=$(SHL1TARGET)

# --- Targets ----------------------------------

.INCLUDE : target.mk
