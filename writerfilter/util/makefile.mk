#**************************************************************
#  
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#  
#    http://www.apache.org/licenses/LICENSE-2.0
#  
#  Unless required by applicable law or agreed to in writing,
#  software distributed under the License is distributed on an
#  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#  KIND, either express or implied.  See the License for the
#  specific language governing permissions and limitations
#  under the License.
#  
#**************************************************************
PRJ=..
PRJNAME=writerfilter
.IF "$(GUI)" == "OS2"
TARGET=wfilt
.ELSE
TARGET=writerfilter
.ENDIF
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

CDEFS+=-DWRITERFILTER_DLLIMPLEMENTATION

.IF "$(UPD)" == "310"
I18NPAPERLIB=-li18npaper$(DLLPOSTFIX)

# Link to modified libcomp, libi18npaper, and liboox
SOLARLIB:=-L$(PRJ)$/..$/comphelper$/$(INPATH)$/lib -L$(PRJ)$/..$/i18npool$/$(INPATH)$/lib -L$(PRJ)$/..$/oox$/$(INPATH)$/lib $(SOLARLIB)
SOLARLIBDIR:=$(PRJ)$/..$/comphelper$/$(INPATH)$/lib -L$(PRJ)$/..$/i18npool$/$(INPATH)$/lib -L$(PRJ)$/..$/oox$/$(INPATH)$/lib -L$(SOLARLIBDIR)
.ENDIF		# "$(UPD)" == "310"

# --- Files --------------------------------------------------------

LIB1TARGET=$(SLB)$/$(TARGET).lib
LIB1FILES=  \
    $(SLB)$/ooxml.lib \
    $(SLB)$/doctok.lib \
	$(SLB)$/resourcemodel.lib \
    $(SLB)$/dmapper.lib \
    $(SLB)$/filter.lib

SHL1LIBS=$(SLB)$/$(TARGET).lib


SHL1TARGET=$(TARGET)$(DLLPOSTFIX)
SHL1STDLIBS=\
    $(I18NISOLANGLIB) \
    $(I18NPAPERLIB) \
    $(SOTLIB) \
    $(TOOLSLIB) \
    $(UNOTOOLSLIB) \
    $(CPPUHELPERLIB)    \
    $(COMPHELPERLIB)    \
    $(CPPULIB)          \
    $(SALLIB)			\
    $(OOXLIB)


SHL1DEPN=
SHL1IMPLIB= i$(SHL1TARGET)
SHL1DEF=    $(MISC)$/$(SHL1TARGET).def
SHL1VERSIONMAP=$(SOLARENV)/src/component.map

DEF1NAME=$(SHL1TARGET)


# --- Targets ------------------------------------------------------

.INCLUDE :	target.mk

.IF "$(UPD)" != "310"
ALLTAR : $(MISC)/writerfilter.component

$(MISC)/writerfilter.component .ERRREMOVE : \
        $(SOLARENV)/bin/createcomponent.xslt writerfilter.component
    $(XSLTPROC) --nonet --stringparam uri \
        '$(COMPONENTPREFIX_BASIS_NATIVE)$(SHL1TARGETN:f)' -o $@ \
        $(SOLARENV)/bin/createcomponent.xslt writerfilter.component
ALLTAR : $(MISC)/writerfilter.component
.ENDIF		# "$(UPD)" != "310"
