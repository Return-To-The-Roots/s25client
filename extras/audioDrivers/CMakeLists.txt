# Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

rttr_set_output_dir(RUNTIME ${RTTR_DRIVERDIR}/audio)
rttr_set_output_dir(LIBRARY ${RTTR_DRIVERDIR}/audio)

add_subdirectory(SDL)
