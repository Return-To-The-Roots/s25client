# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_test(NAME s25client_showHelp COMMAND s25client --help
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(s25client_showHelp PROPERTIES
  PASS_REGULAR_EXPRESSION "--version"
)

add_test(NAME s25client_showVersion COMMAND s25client --version
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(s25client_showVersion PROPERTIES
  PASS_REGULAR_EXPRESSION "^Return To The Roots v[0-9.]+-[0-9a-f]+\n"
)

add_test(NAME s25client_invalidOption COMMAND s25client --nonExistantOption
         WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(s25client_invalidOption PROPERTIES
  WILL_FAIL TRUE
  FAIL_REGULAR_EXPRESSION "Error: .*nonExistantOption"
)