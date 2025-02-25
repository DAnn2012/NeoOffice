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
#   Modified March 2022 by Patrick Luby. NeoOffice is only distributed
#   under the GNU General Public License, Version 3 as allowed by Section 3.3
#   of the Mozilla Public License, v. 2.0.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(eval $(call gb_Module_Module,dbaccess))

ifneq (,$(filter DBCONNECTIVITY,$(BUILD_TYPE)))

$(eval $(call gb_Module_add_targets,dbaccess,\
	$(if $(filter WNT,$(OS)),Executable_odbcconfig) \
	Library_dba \
	Library_dbaxml \
	Library_dbmm \
	Library_dbu \
	Library_sdbt \
))

$(eval $(call gb_Module_add_l10n_targets,dbaccess,\
    AllLangResTarget_dba \
    AllLangResTarget_dbmm \
    AllLangResTarget_dbu \
    AllLangResTarget_sdbt \
	UIConfig_dbaccess \
	UIConfig_dbapp \
	UIConfig_dbbrowser \
	UIConfig_dbquery \
	UIConfig_dbrelation \
	UIConfig_dbtable \
	UIConfig_dbtdata \
))

ifeq ($(ENABLE_FIREBIRD_SDBC),TRUE)
$(eval $(call gb_Module_add_check_targets,dbaccess,\
    CppunitTest_dbaccess_firebird_test \
))
endif

$(eval $(call gb_Module_add_check_targets,dbaccess,\
	$(if $(filter-out MACOSX,$(OS)$(filter-out X86_64,$(CPUNAME))),CppunitTest_dbaccess_dialog_save) \
	$(if $(filter-out MACOSX,$(OS)$(filter-out X86_64,$(CPUNAME))),CppunitTest_dbaccess_empty_stdlib_save) \
	$(if $(filter-out MACOSX,$(OS)$(filter-out X86_64,$(CPUNAME))),CppunitTest_dbaccess_nolib_save) \
	CppunitTest_dbaccess_macros_test \
))

ifeq ($(ENABLE_JAVA),TRUE)
$(eval $(call gb_Module_add_check_targets,dbaccess,\
	$(if $(filter-out macosx_arm64,$(PLATFORMID)),CppunitTest_dbaccess_hsqldb_test) \
))
endif

# This runs a suite of peformance tests on embedded firebird and HSQLDB.
# Instructions on running the test can be found in qa/unit/embeddedb_performancetest
ifeq ($(ENABLE_FIREBIRD_SDBC),TRUE)
ifeq ($(ENABLE_JAVA),TRUE)
$(eval $(call gb_Module_add_check_targets,dbaccess,\
    CppunitTest_dbaccess_embeddeddb_performancetest \
))
endif
endif

$(eval $(call gb_Module_add_subsequentcheck_targets,dbaccess,\
	JunitTest_dbaccess_complex \
    JunitTest_dbaccess_unoapi \
))

ifneq ($(DISABLE_PYTHON),TRUE)
ifneq ($(ENABLE_JAVA),)
$(eval $(call gb_Module_add_subsequentcheck_targets,dbaccess,\
	PythonTest_dbaccess_python \
))
endif
endif

endif

# vim: set noet sw=4 ts=4:
