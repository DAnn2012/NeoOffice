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
#   Modified December 2016 by Patrick Luby. NeoOffice is only distributed
#   under the GNU General Public License, Version 3 as allowed by Section 3.3
#   of the Mozilla Public License, v. 2.0.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

$(eval $(call gb_Library_Library,ucbhelper))

$(eval $(call gb_Library_use_sdk_api,ucbhelper))

$(eval $(call gb_Library_use_external,ucbhelper,boost_headers))

$(eval $(call gb_Library_use_libraries,ucbhelper,\
    cppu \
    cppuhelper \
    sal \
    salhelper \
	$(gb_UWINAPI) \
))

$(eval $(call gb_Library_add_defs,ucbhelper,\
    -DUCBHELPER_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_add_exception_objects,ucbhelper,\
    ucbhelper/source/client/activedatasink \
    ucbhelper/source/client/activedatastreamer \
    ucbhelper/source/client/commandenvironment \
    ucbhelper/source/client/content \
    ucbhelper/source/client/fileidentifierconverter \
    ucbhelper/source/client/interceptedinteraction \
    ucbhelper/source/client/proxydecider \
	ucbhelper/source/provider/authenticationfallback \
    ucbhelper/source/provider/cancelcommandexecution \
    ucbhelper/source/provider/contenthelper \
    ucbhelper/source/provider/contentidentifier \
    ucbhelper/source/provider/contentinfo \
    ucbhelper/source/provider/fd_inputstream \
    ucbhelper/source/provider/getcomponentcontext \
    ucbhelper/source/provider/interactionrequest \
    ucbhelper/source/provider/propertyvalueset \
    ucbhelper/source/provider/providerhelper \
    ucbhelper/source/provider/registerucb \
    ucbhelper/source/provider/resultset \
    ucbhelper/source/provider/resultsethelper \
    ucbhelper/source/provider/resultsetmetadata \
    ucbhelper/source/provider/simpleauthenticationrequest \
    ucbhelper/source/provider/simplecertificatevalidationrequest \
    ucbhelper/source/provider/simpleinteractionrequest \
    ucbhelper/source/provider/simpleioerrorrequest \
    ucbhelper/source/provider/simplenameclashresolverequest \
    ucbhelper/source/provider/std_inputstream \
    ucbhelper/source/provider/std_outputstream \
))

ifeq ($(strip $(GUIBASE)),java)
$(eval $(call gb_Library_use_system_darwin_frameworks,ucbhelper,\
    CoreFoundation \
    SystemConfiguration \
))
endif	# GUIBASE == java

# vim: set noet sw=4 ts=4:

