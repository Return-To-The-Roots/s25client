# Copyright (C) 2005 - 2024 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(samplerate_cpp INTERFACE)
target_include_directories(samplerate_cpp INTERFACE include)
target_link_libraries(samplerate_cpp INTERFACE SampleRate::samplerate)
target_compile_features(samplerate_cpp INTERFACE cxx_std_17)
enable_warnings(samplerate_cpp)
