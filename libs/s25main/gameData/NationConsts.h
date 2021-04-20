// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "mygettext/mygettext.h"
#include "gameTypes/Nation.h"
#include <s25util/warningSuppression.h>

constexpr helpers::EnumArray<const char*, Nation> SUPPRESS_UNUSED NationNames = {
  gettext_noop("Africans"), gettext_noop("Japanese"), gettext_noop("Romans"), gettext_noop("Vikings"),
  gettext_noop("Babylonians")};

/// Konvertierungstabelle von RttR-Nation-Indizes in Original-S2-Nation-Indizes
constexpr helpers::EnumArray<unsigned char, Nation> SUPPRESS_UNUSED NATION_RTTR_TO_S2 = {
  3, 2, 0, 1, 0 /* Babylonians get the roman figures where no others are used */
};
