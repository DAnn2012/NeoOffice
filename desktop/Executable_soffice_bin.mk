# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
# 
#   Modified November 2016 by Patrick Luby. NeoOffice is only distributed
#   under the GNU General Public License, Version 3 as allowed by Section 3.3
#   of the Mozilla Public License, v. 2.0.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(eval $(call gb_Executable_Executable,soffice_bin))

$(eval $(call gb_Executable_set_targettype_gui,soffice_bin,YES))

$(eval $(call gb_Executable_set_include,soffice_bin,\
    $$(INCLUDE) \
    -I$(SRCDIR)/desktop/source/inc \
))

ifneq ($(strip $(PRODUCT_NAME)),)
$(eval $(call gb_Executable_add_defs,soffice_bin,\
    -DPRODUCT_NAME='"$(PRODUCT_NAME)"' \
))
endif	# PRODUCT_NAME != ""

ifneq ($(strip $(PRODUCT_MIN_OSVERSION)),)
$(eval $(call gb_Executable_add_defs,soffice_bin,\
    -DPRODUCT_MIN_OSVERSION='"$(PRODUCT_MIN_OSVERSION)"' \
))
endif	# PRODUCT_MIN_OSVERSION != ""

ifneq ($(strip $(PRODUCT_MAX_OSVERSION)),)
$(eval $(call gb_Executable_add_defs,soffice_bin,\
    -DPRODUCT_MAX_OSVERSION='"$(PRODUCT_MAX_OSVERSION)"' \
))
endif	# PRODUCT_MAX_OSVERSION != ""

ifneq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Executable_use_libraries,soffice_bin,\
    sal \
    sofficeapp \
	$(gb_UWINAPI) \
))
endif	# GUIBASE != java

$(eval $(call gb_Executable_add_cobjects,soffice_bin,\
    desktop/source/app/main \
))

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Executable_add_objcxxobjects,soffice_bin,\
    desktop/source/app/main_java \
))

$(eval $(call gb_Executable_add_cobjects,soffice_bin,\
    desktop/source/app/main_java_init \
))
endif	# GUIBASE == java

ifeq ($(OS),MACOSX)

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Executable_use_system_darwin_frameworks,soffice_bin,\
    AppKit \
))

gb_Executable_TARGETTYPEFLAGS := \
    $(filter-out -bind_at_load,$(gb_Executable_TARGETTYPEFLAGS)) \
    -Wl,-rpath,@executable_path/../Frameworks \
    -Wl,-rpath,/usr/lib \
    -Wl,-rpath,/usr/local/lib
else	# GUIBASE == java
$(eval $(call gb_Executable_set_ldflags,\
    $(filter-out -bind_at_load,$$(LDFLAGS)) \
))
endif	# GUIBASE == java

endif

ifeq ($(OS),WNT)

$(eval $(call gb_Executable_use_static_libraries,soffice_bin,\
    ooopathutils \
    winextendloaderenv \
))

$(eval $(call gb_Executable_set_targettype_gui,soffice_bin,YES))

$(eval $(call gb_Executable_add_nativeres,soffice_bin,sofficebin/officeloader))

ifeq ($(COM),MSC)

$(eval $(call gb_Executable_add_ldflags,soffice_bin,\
    /STACK:10000000 \
))

endif

endif

ifeq ($(strip $(GUIBASE)),java)
ifeq ($(OS),MACOSX)
ifneq ($(strip $(ENABLE_ASAN)),)
$(eval $(call gb_Executable_add_cflags,soffice_bin,\
    -fsanitize=address \
))
$(eval $(call gb_Executable_add_libs,soffice_bin,\
    -fsanitize=address \
))
endif	# ENABLE_ASAN != ""
endif	# OS == MACOSX
endif	# GUIBASE == java

# vim: set ts=4 sw=4 et:
