# Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

configure_file(languageFiles.h.cmake languageFiles.h)
add_testcase(NAME LanguageFiles
    LIBS s25Common mygettext s25util::common Boost::filesystem rttr::vld
    INCLUDES ${CMAKE_CURRENT_BINARY_DIR}
    COST 15
)
add_dependencies(Test_LanguageFiles translations)
