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
#     Modified November 2007 by Patrick Luby. NeoOffice is distributed under
#     GPL only under modification term 3 of the LGPL.
#
#*************************************************************************

PRJ=.

PRJNAME=ooo_rhino
TARGET=ooo_rhino

.IF "$(SOLAR_JAVA)"!=""
# --- Settings -----------------------------------------------------

.INCLUDE :	settings.mk

# --- Files --------------------------------------------------------

TARFILE_NAME=rhino15R4
TARFILE_ROOTDIR=rhino1_5R4

ADDITIONAL_FILES=makefile.mk \
	toolsrc/org/mozilla/javascript/tools/debugger/AbstractCellEditor.java \
	toolsrc/org/mozilla/javascript/tools/debugger/AbstractTreeTableModel.java \
	toolsrc/org/mozilla/javascript/tools/debugger/JTreeTable.java \
	toolsrc/org/mozilla/javascript/tools/debugger/OfficeScriptInfo.java \
	toolsrc/org/mozilla/javascript/tools/debugger/TreeTableModelAdapter.java \
	toolsrc/org/mozilla/javascript/tools/debugger/TreeTableModel.java

PATCH_FILE_NAME=rhino1_5R4.patch

BUILD_ACTION=dmake $(MFLAGS) $(CALLMACROS)

# --- Targets ------------------------------------------------------

.INCLUDE : set_ext.mk
.INCLUDE : target.mk
.INCLUDE : tg_ext.mk

.IF "$(GUIBASE)" == "java"
BACK_PATH:=$(BACK_PATH)..$/..$/rhino$/
.ENDIF		# "$(GUIBASE)" == "java"

.ELSE
all:
        @echo java disabled
.ENDIF
