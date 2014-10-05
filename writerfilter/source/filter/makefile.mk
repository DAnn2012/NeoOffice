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
PRJ=..$/..
PRJNAME=writerfilter
TARGET=filter
GEN_HID=TRUE

ENABLE_EXCEPTIONS=TRUE

# --- Settings ----------------------------------

.INCLUDE : settings.mk
.INCLUDE :  $(PRJ)$/inc$/writerfilter.mk

.IF "$(UPD)" == "310"
INCLOCAL += -I$(PRJ)$/..$/oox/inc -I$(PRJ)$/..$/sal/inc
.ENDIF		# "$(UPD)" == "310"

# --- Files -------------------------------------

SLOFILES=           $(SLO)$/WriterFilter.obj \
                    $(SLO)$/WriterFilterDetection.obj \
                    $(SLO)$/ImportFilter.obj \
                    $(SLO)$/RtfFilter.obj


# --- Targets ----------------------------------

.INCLUDE : target.mk



