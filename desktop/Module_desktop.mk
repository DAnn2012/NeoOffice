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

$(eval $(call gb_Module_Module,desktop))

$(eval $(call gb_Module_add_targets,desktop,\
    CustomTarget_desktop_unopackages_install \
    GeneratedPackage_desktop_unopackages_install \
    Library_deployment \
    Library_deploymentgui \
    Library_deploymentmisc \
    Library_offacc \
    Library_sofficeapp \
    $(if $(ENABLE_HEADLESS),,Library_spl) \
    Package_branding \
    $(if $(CUSTOM_BRAND_DIR),Package_branding_custom) \
))

$(eval $(call gb_Module_add_l10n_targets,desktop,\
    AllLangResTarget_deployment \
    AllLangResTarget_deploymentgui \
    AllLangResTarget_dkt \
    UIConfig_deployment \
))

ifneq (,$(filter DESKTOP,$(BUILD_TYPE)))
$(eval $(call gb_Module_add_targets,desktop,\
    Executable_soffice_bin \
    $(if $(filter $(GUIBASE),java),,Executable_unopkg_bin) \
    Library_migrationoo2 \
    Library_migrationoo3 \
    Library_unopkgapp \
    Package_scripts \
))

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Module_add_targets,desktop,\
    Executable_soffice2_bin \
    Executable_soffice3_bin \
))
endif	# GUIBASE == java

ifneq ($(OS),MACOSX)
ifneq ($(OS),WNT)
$(eval $(call gb_Module_add_targets,desktop,\
    Pagein_calc \
    Pagein_common \
    Pagein_draw \
    Pagein_impress \
    Pagein_writer \
    CustomTarget_soffice \
    Package_sbase_sh \
    Package_scalc_sh \
    Package_sdraw_sh \
    Package_simpress_sh \
    Package_smath_sh \
    Package_swriter_sh \
    Package_soffice_sh \
))
endif
endif
endif

ifeq ($(OS),WNT)

ifneq ($(ENABLE_CRASHDUMP),)
$(eval $(call gb_Module_add_targets,desktop,\
    Executable_crashrep_com \
))
endif

$(eval $(call gb_Module_add_targets,desktop,\
    StaticLibrary_winextendloaderenv \
    StaticLibrary_winlauncher \
    Executable_quickstart \
    Executable_sbase \
    Executable_scalc \
    Executable_sdraw \
    Executable_simpress \
    Executable_smath \
    Executable_soffice \
    Executable_sweb \
    Executable_swriter \
    Executable_unoinfo \
    Executable_unopkg \
    Executable_unopkg_com \
    WinResTarget_quickstart \
    WinResTarget_sbase \
    WinResTarget_scalc \
    WinResTarget_sdraw \
    WinResTarget_simpress \
    WinResTarget_soffice \
    WinResTarget_sofficebin \
    WinResTarget_smath \
    WinResTarget_sweb \
    WinResTarget_swriter \
))

else ifeq ($(OS),MACOSX)

$(eval $(call gb_Module_add_targets,desktop,\
    Package_desktop_install \
))

else ifeq ($(OS),ANDROID)

else ifeq ($(OS),IOS)

else

$(eval $(call gb_Module_add_targets,desktop,\
    Executable_oosplash \
))

endif

ifneq (,$(filter Extension_test-active,$(MAKECMDGOALS)))
$(eval $(call gb_Module_add_targets,desktop, \
    Extension_test-active \
    Jar_active_java \
    Library_active_native \
))
endif

ifneq (,$(filter Extension_test-passive,$(MAKECMDGOALS)))
$(eval $(call gb_Module_add_targets,desktop, \
    Extension_test-passive \
    Jar_passive_java \
    Library_passive_native \
    Pyuno_passive_python \
    Rdb_passive_generic \
    Rdb_passive_platform \
))
endif

# vim: set ts=4 sw=4 et:
