# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sfx2_cloudauth))

$(eval $(call gb_CppunitTest_add_exception_objects,sfx2_cloudauth, \
    sfx2/qa/cppunit/test_cloudauth \
))

$(eval $(call gb_CppunitTest_use_sdk_api,sfx2_cloudauth))

$(eval $(call gb_CppunitTest_use_libraries,sfx2_cloudauth, \
	comphelper \
	cppu \
	cppuhelper \
    sal \
    sfx \
))

$(eval $(call gb_CppunitTest_use_externals,sfx2_cloudauth, \
    boost_headers \
    curl \
))

$(eval $(call gb_CppunitTest_use_custom_headers,sfx2_cloudauth,\
	officecfg/registry \
))

# vim: set noet sw=4 ts=4: 