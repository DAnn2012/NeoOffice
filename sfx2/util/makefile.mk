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

PRJ=..

ENABLE_EXCEPTIONS=TRUE
PRJNAME=sfx2
TARGET=sfx
#sfx.hid generieren
GEN_HID=TRUE
GEN_HID_OTHER=TRUE
USE_DEFFILE=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Allgemein ----------------------------------------------------

LIB1TARGET= $(SLB)$/$(TARGET).lib
LIB1FILES=  $(SLB)$/appl.lib		\
            $(SLB)$/explorer.lib	\
            $(SLB)$/doc.lib			\
            $(SLB)$/view.lib		\
            $(SLB)$/control.lib		\
            $(SLB)$/notify.lib		\
            $(SLB)$/menu.lib		\
            $(SLB)$/inet.lib		\
            $(SLB)$/toolbox.lib		\
            $(SLB)$/statbar.lib		\
            $(SLB)$/dialog.lib		\
            $(SLB)$/bastyp.lib		\
            $(SLB)$/config.lib

HELPIDFILES=\
			..\inc\sfxsids.hrc	\
			..\source\inc\helpid.hrc

.IF "$(GUI)"!="UNX"
LIB2TARGET= $(LB)$/$(TARGET).lib
LIB2FILES=  $(LB)$/isfx.lib
.ENDIF

SHL1TARGET= sfx$(UPD)$(DLLPOSTFIX)
SHL1IMPLIB= isfx
SHL1USE_EXPORTS=ordinal

SHL1STDLIBS+=\
        $(FWELIB) \
		$(BASICLIB) \
		$(XMLSCRIPTLIB) \
		$(SVTOOLLIB) \
		$(TKLIB) \
		$(VCLLIB) \
		$(SVLLIB)	\
		$(SOTLIB) \
        $(UNOTOOLSLIB) \
		$(TOOLSLIB) \
		$(I18NISOLANGLIB) \
		$(SYSSHELLLIB) \
		$(COMPHELPERLIB) \
        $(UCBHELPERLIB) \
		$(CPPUHELPERLIB) \
		$(CPPULIB) \
		$(VOSLIB) \
		$(SALLIB) \
		$(SJLIB)

.IF "$(GUI)"=="WNT"

SHL1STDLIBS+=\
		uwinapi.lib \
		advapi32.lib \
		shell32.lib \
		gdi32.lib \
		ole32.lib \
		uuid.lib

.ENDIF # WNT

.IF "$(OS)"=="MACOSX"
.IF "$(GUIBASE)"=="java"
SHL1STDLIBS += -framework AppKit
.ENDIF
.ENDIF

.IF "$(GUI)"!="MAC"
SHL1DEPN += $(shell $(FIND) $(SLO) -type f -name "*.OBJ" -print)
.ENDIF

.IF "$(SOLAR_JAVA)" != ""
SHL1DEPN+= \
			$(L)$/sj.lib
.ENDIF

SHL1LIBS=   $(LIB1TARGET)

SHL1OBJS=   $(SLO)$/sfxdll.obj

SHL1DEF=    $(MISC)$/$(SHL1TARGET).def

DEF1NAME	=$(SHL1TARGET)
DEFLIB1NAME	=sfx
DEF1DES		=Sfx

SFXSRSLIST=\
		$(SRS)$/appl.srs \
		$(SRS)$/sfx.srs \
		$(SRS)$/doc.srs \
		$(SRS)$/view.srs \
		$(SRS)$/config.srs \
		$(SRS)$/menu.srs \
		$(SRS)$/dialog.srs \
                $(SRS)$/bastyp.srs

RESLIB1NAME=$(TARGET)
RESLIB1IMAGES=$(PRJ)$/res
RESLIB1SRSFILES=$(SFXSRSLIST)

# --- Targets ------------------------------------------------------


.INCLUDE :  target.mk
