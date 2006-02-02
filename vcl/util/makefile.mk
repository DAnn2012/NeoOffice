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
#     Modified February 2006 by Patrick Luby. NeoOffice is distributed under
#     GPL only under modification term 3 of the LGPL.
#
#*************************************************************************

PRJ=..

PRJNAME=vcl
TARGET=vcl
VERSION=$(UPD)
USE_DEFFILE=TRUE

.IF "$(OS)" == "SOLARIS"
LINKFLAGSRUNPATH=-R/usr/sfw/lib -R\''$$ORIGIN'\'
.ENDIF

# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  makefile.pmk
.INCLUDE :  makefile2.pmk


# --- Allgemein ----------------------------------------------------------

HXXDEPNLST= $(INC)$/accel.hxx       \
            $(INC)$/animate.hxx     \
            $(INC)$/apptypes.hxx    \
            $(INC)$/bitmap.hxx      \
            $(INC)$/bitmapex.hxx    \
            $(INC)$/bmpacc.hxx      \
            $(INC)$/btndlg.hxx      \
            $(INC)$/button.hxx      \
            $(INC)$/ctrl.hxx        \
            $(INC)$/cursor.hxx      \
            $(INC)$/cmdevt.hxx      \
            $(INC)$/decoview.hxx    \
            $(INC)$/dialog.hxx      \
            $(INC)$/dllapi.h        \
            $(INC)$/dockwin.hxx     \
            $(INC)$/edit.hxx        \
            $(INC)$/event.hxx       \
            $(INC)$/field.hxx       \
            $(INC)$/fixed.hxx       \
            $(INC)$/floatwin.hxx    \
            $(INC)$/font.hxx        \
            $(INC)$/fontcvt.hxx     \
            $(INC)$/floatwin.hxx    \
            $(INC)$/graph.hxx       \
            $(INC)$/group.hxx       \
            $(INC)$/help.hxx        \
            $(INC)$/jobset.hxx      \
            $(INC)$/keycodes.hxx    \
            $(INC)$/keycod.hxx      \
            $(INC)$/image.hxx       \
            $(INC)$/lstbox.h        \
            $(INC)$/lstbox.hxx      \
            $(INC)$/mapmod.hxx      \
            $(INC)$/metaact.hxx     \
            $(INC)$/menu.hxx        \
            $(INC)$/menubtn.hxx     \
            $(INC)$/metric.hxx      \
            $(INC)$/morebtn.hxx     \
            $(INC)$/msgbox.hxx      \
            $(INC)$/octree.hxx      \
            $(INC)$/outdev.hxx      \
            $(INC)$/outdev3d.hxx    \
            $(INC)$/pointr.hxx      \
            $(INC)$/ptrstyle.hxx    \
            $(INC)$/prntypes.hxx    \
            $(INC)$/print.hxx       \
            $(INC)$/prndlg.hxx      \
            $(INC)$/region.hxx      \
            $(INC)$/salbtype.hxx    \
            $(INC)$/scrbar.hxx      \
            $(INC)$/slider.hxx      \
            $(INC)$/seleng.hxx      \
            $(INC)$/settings.hxx    \
            $(INC)$/sound.hxx       \
            $(INC)$/sndstyle.hxx    \
            $(INC)$/split.hxx       \
            $(INC)$/splitwin.hxx    \
            $(INC)$/spin.hxx        \
            $(INC)$/spinfld.hxx     \
            $(INC)$/status.hxx      \
            $(INC)$/stdtext.hxx     \
            $(INC)$/sv.h            \
            $(INC)$/svapp.hxx       \
            $(INC)$/syschild.hxx    \
            $(INC)$/sysdata.hxx     \
            $(INC)$/syswin.hxx      \
            $(INC)$/tabctrl.hxx     \
            $(INC)$/tabdlg.hxx      \
            $(INC)$/tabpage.hxx     \
            $(INC)$/toolbox.hxx     \
            $(INC)$/timer.hxx       \
            $(INC)$/virdev.hxx      \
            $(INC)$/wall.hxx        \
            $(INC)$/waitobj.hxx     \
            $(INC)$/wintypes.hxx    \
            $(INC)$/window.hxx      \
            $(INC)$/wrkwin.hxx

.IF "$(linkinc)" != ""
SHL11FILE=  $(MISC)$/app.slo
SHL12FILE=  $(MISC)$/gdi.slo
SHL13FILE=  $(MISC)$/win.slo
SHL14FILE=  $(MISC)$/ctrl.slo
#SHL15FILE=  $(MISC)$/ex.slo
SHL16FILE=  $(MISC)$/salapp.slo
SHL17FILE=  $(MISC)$/salwin.slo
SHL18FILE=  $(MISC)$/salgdi.slo
.ENDIF

LIB1TARGET= $(SLB)$/$(TARGET).lib
LIB1FILES=  $(SLB)$/app.lib     \
            $(SLB)$/gdi.lib     \
            $(SLB)$/win.lib     \
            $(SLB)$/ctrl.lib    \
            $(SLB)$/helper.lib


.IF "$(GUI)" == "UNX" && "$(GUIBASE)" != "java"
LIB1FILES+=$(SLB)$/salplug.lib
.ELSE
LIB1FILES+= \
            $(SLB)$/salwin.lib  \
            $(SLB)$/salgdi.lib  \
            $(SLB)$/salapp.lib

.IF "$(GUIBASE)" != "java"
LIB1FILES+= \
            $(SLB)$/saljava.lib
.ENDIF
.ENDIF

SHL1TARGET= vcl$(VERSION)$(DLLPOSTFIX)
SHL1IMPLIB= ivcl
SHL1STDLIBS+=\
            $(SOTLIB)           \
            $(UNOTOOLSLIB)      \
            $(TOOLSLIB)         \
            $(COMPHELPERLIB)	\
            $(UCBHELPERLIB)     \
            $(CPPUHELPERLIB)    \
            $(CPPULIB)          \
            $(VOSLIB)           \
            $(SALLIB)			\
            $(BASEGFXLIB)		\
            $(ICUUCLIB)			\
            $(ICULELIB)			\
			$(JVMACCESSLIB)
SHL1USE_EXPORTS=ordinal

.IF "$(USE_BUILTIN_RASTERIZER)"!=""
    LIB1FILES +=    $(SLB)$/glyphs.lib
    SHL1STDLIBS+=   $(FREETYPELIB)
.ENDIF # USE_BUILTIN_RASTERIZER


.IF "$(GUI)"!="MAC"
SHL1DEPN=   $(L)$/itools.lib $(L)$/sot.lib
.ENDIF

SHL1LIBS=   $(LIB1TARGET)
.IF "$(GUI)"!="UNX"
SHL1OBJS=   $(SLO)$/salshl.obj
.ELIF "$(OS)"!="FREEBSD"
SHL1STDLIBS+=-ldl
.ENDIF

.IF "$(GUI)" != "MAC"
.IF "$(GUI)" != "UNX"
SHL1RES=    $(RES)$/salsrc.res
.ENDIF
.ENDIF
SHL1DEF=    $(MISC)$/$(SHL1TARGET).def

DEF1NAME    =$(SHL1TARGET)
DEF1DEPN    =   $(HXXDEPNLST) \
                $(LIB1TARGET)
DEF1DES     =VCL
DEFLIB1NAME =vcl

# --- W32 ----------------------------------------------------------------

.IF "$(GUI)" == "WNT"

SHL1STDLIBS += uwinapi.lib      \
               gdi32.lib        \
               winspool.lib     \
               ole32.lib        \
               shell32.lib      \
               advapi32.lib     \
               apsp.lib         \
               imm32.lib

.IF "$(GUI)$(COM)$(CPU)" == "WNTMSCI"
LINKFLAGSSHL += /ENTRY:LibMain@12
.ENDIF
.ENDIF


# --- UNX ----------------------------------------------------------------

.IF "$(GUI)"=="UNX"

.IF "$(OS)"!="MACOSX" && "$(OS)"!="FREEBSD"
SHL1STDLIBS+= -ldl
.ENDIF

.IF "$(GUIBASE)"=="aqua"
SHL1STDLIBS += -framework Cocoa
.ENDIF

.IF "$(GUIBASE)" != "unx"
SHL1STDLIBS += -lX11
.ENDIF

.IF "$(OS)"=="MACOSX"
.IF "$(GUIBASE)" == "java"
SHL1STDLIBS += -framework ApplicationServices -framework Carbon -framework AudioToolbox -framework AudioUnit -framework AppKit
.ELSE
SHL1STDLIBS += -framework Foundation -framework CoreFoundation
.ENDIF
.ENDIF # "$(OS)"=="MACOSX"

.ENDIF          # "$(GUI)"=="UNX"

# UNX sal plugins
.IF "$(GUI)" == "UNX" && "$(GUIBASE)" != "java"

# basic pure X11 plugin
LIB2TARGET=$(SLB)$/ipure_x
LIB2FILES= \
            $(SLB)$/salwin.lib  \
            $(SLB)$/salgdi.lib  \
            $(SLB)$/salapp.lib
SHL2TARGET=vclplug_gen$(UPD)$(DLLPOSTFIX)
SHL2IMPLIB=ipure_x
SHL2LIBS=$(LIB2TARGET)
SHL2DEPN=$(SHL1IMPLIBN) $(SHL1TARGETN)

# libs for generic plugin
SHL2STDLIBS=\
			$(VCLLIB)\
			-lpsp$(VERSION)$(DLLPOSTFIX)\
            $(SOTLIB)           \
            $(UNOTOOLSLIB)      \
            $(TOOLSLIB)         \
            $(COMPHELPERLIB)	\
            $(UCBHELPERLIB)     \
            $(CPPUHELPERLIB)    \
            $(CPPULIB)          \
            $(VOSLIB)           \
            $(SALLIB)

# prepare linking of Xinerama
.IF "$(USE_XINERAMA)" != "NO"

.IF "$(OS)"=="MACOSX"
XINERAMALIBS=-lXinerama
.ELSE
.IF "$(OS)" != "SOLARIS"
.IF "$(XINERAMA_LINK)" == "dynamic"
XINERAMALIBS= -lXinerama
.ELSE
XINERAMALIBS= -Wl,-Bstatic -lXinerama -Wl,-Bdynamic 
.ENDIF # XINERAMA_LINK == dynamic
.ENDIF # OS == SOLARIS
.ENDIF # OS == MACOSX

SHL2STDLIBS += $(XINERAMALIBS)
.ENDIF # USE_XINERAMA != NO

.IF "$(XRENDER_LINK)" == "YES"
SHL2STDLIBS+=`pkg-config --libs xrender`
.ENDIF


.IF "$(ENABLE_PASF)" != ""
.IF "$(OS)"=="MACOSX"
SHL2STDLIBS += -framework CoreAudio -framework AudioToolbox
.ENDIF
SHL2STDLIBS += -lsndfile -lportaudio
.ENDIF # ENABLE_PASF

.IF "$(ENABLE_NAS)" != ""
SHL2STDLIBS += -laudio
.IF "$(OS)"=="SOLARIS"
# needed by libaudio.a
SHL2STDLIBS += -ldl -lnsl -lsocket
.ENDIF # SOLARIS
.ENDIF

.IF "$(GUIBASE)"=="unx"

.IF "$(WITH_LIBSN)"=="YES"
SHL2STDLIBS+=$(LIBSN_LIBS)
.ENDIF

SHL2STDLIBS += -lXext -lSM -lICE -lX11
.IF "$(OS)"!="MACOSX" && "$(OS)"!="FREEBSD"
# needed by salprnpsp.cxx
SHL2STDLIBS+= -ldl
.ENDIF

.ENDIF          # "$(GUIBASE)"=="unx"

# dummy plugin
LIB3TARGET=$(SLB)$/idummy_plug_
LIB3FILES= \
            $(SLB)$/dapp.lib
SHL3TARGET=vclplug_dummy$(UPD)$(DLLPOSTFIX)
SHL3IMPLIB=idummy_plug_
SHL3LIBS=$(LIB3TARGET)
SHL3DEPN=$(SHL1IMPLIBN) $(SHL1TARGETN)

# libs for dummy plugin
SHL3STDLIBS=\
			$(VCLLIB)\
			-lpsp$(VERSION)$(DLLPOSTFIX)\
            $(SOTLIB)           \
            $(UNOTOOLSLIB)      \
            $(TOOLSLIB)         \
            $(COMPHELPERLIB)	\
            $(UCBHELPERLIB)     \
            $(CPPUHELPERLIB)    \
            $(CPPULIB)          \
            $(VOSLIB)           \
            $(SALLIB)

# gtk plugin
.IF "$(ENABLE_GTK)" != ""
PKGCONFIG_MODULES=gtk+-2.0 gthread-2.0
.INCLUDE: pkg_config.mk

LIB4TARGET=$(SLB)$/igtk_plug_
LIB4FILES=\
			$(SLB)$/gtkapp.lib\
			$(SLB)$/gtkgdi.lib\
			$(SLB)$/gtkwin.lib
SHL4TARGET=vclplug_gtk$(UPD)$(DLLPOSTFIX)
SHL4IMPLIB=igtk_plug_
SHL4LIBS=$(LIB4TARGET)
SHL4DEPN=$(SHL1IMPLIBN) $(SHL1TARGETN) $(SHL2IMPLIBN) $(SHL2TARGETN)
# libs for gtk plugin
SHL4STDLIBS+=$(PKGCONFIG_LIBS:s/ -lpangoxft-1.0//)
# hack for faked SO environment
.IF "$(PKGCONFIG_ROOT)"!=""
SHL4SONAME+=-z nodefs
SHL4NOCHECK=TRUE
.ENDIF          # "$(PKGCONFIG_ROOT)"!=""


SHL4STDLIBS+=-l$(SHL2TARGET)
.IF "$(OS)"=="FREEBSD" || "$(OS)"=="MACOSX"
SHL4STDLIBS+=$(SHL3STDLIBS) -lX11
.ELSE
SHL4STDLIBS+=$(SHL3STDLIBS) -lX11 -ldl
.ENDIF # "$(OS)"=="FREEBSD" || "$(OS)"=="MACOSX"
.ENDIF # "$(ENABLE_GTK)" != ""

# KDE plugin
.IF "$(ENABLE_KDE)" != ""
.IF "$(KDE_ROOT)"!=""
SOLARLIB+=-L$(KDE_ROOT)$/lib
KDE_LIBS:=-lkdeui -lkdecore -lqt-mt
.ENDIF 			# "$(KDE_ROOT)"!=""
LIB5TARGET=$(SLB)$/ikde_plug_
LIB5FILES=$(SLB)$/kdeplug.lib
SHL5TARGET=vclplug_kde$(UPD)$(DLLPOSTFIX)
SHL5IMPLIB=ikde_plug_
SHL5LIBS=$(LIB5TARGET)
SHL5DEPN=$(SHL2TARGETN)
# libs for KDE plugin
SHL5STDLIBS=$(KDE_LIBS)
SHL5STDLIBS+=-l$(SHL2TARGET)
.IF "$(OS)"=="FREEBSD" || "$(OS)"=="MACOSX"
SHL5STDLIBS+=$(SHL3STDLIBS) -lX11
.ELSE
SHL5STDLIBS+=$(SHL3STDLIBS) -lX11 -ldl
.ENDIF # "$(OS)"=="FREEBSD" || "$(OS)"=="MACOSX"
.ENDIF # "$(ENABLE_KDE)" != ""

.ENDIF # UNX && !java

.IF "$(GUIBASE)"=="java"
JARCLASSDIRS = com
JARTARGET = $(TARGET).jar
JARCOMPRESS = TRUE
.ENDIF

# --- Allgemein ----------------------------------------------------------

.INCLUDE :  target.mk

