// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "../resources/ResourceId.h"
#include "mygettext/mygettext.h"
#include <s25util/warningSuppression.h>
#include <helpers/make_array.h>

struct PortraitInfo
{
    const char* name;
    ResourceId resourceId;
    unsigned int resourceIndex;
};

constexpr auto SUPPRESS_UNUSED Portraits = helpers::make_array<PortraitInfo>(
  PortraitInfo{gettext_noop("Octavianus"), ResourceId("io"), 263},
  PortraitInfo{gettext_noop("Julius"), ResourceId("io"), 251},
  PortraitInfo{gettext_noop("Brutus"), ResourceId("io"), 252},
  PortraitInfo{gettext_noop("Erik"), ResourceId("io"), 261}, PortraitInfo{gettext_noop("Knut"), ResourceId("io"), 256},
  PortraitInfo{gettext_noop("Olof"), ResourceId("io"), 255},
  PortraitInfo{gettext_noop("Yamauchi"), ResourceId("io"), 254},
  PortraitInfo{gettext_noop("Tsunami"), ResourceId("io"), 253},
  PortraitInfo{gettext_noop("Hakirawashi"), ResourceId("io"), 260},
  PortraitInfo{gettext_noop("Shaka"), ResourceId("io"), 258}, PortraitInfo{gettext_noop("Todo"), ResourceId("io"), 262},
  PortraitInfo{gettext_noop("Mnga Tscha"), ResourceId("io"), 257},
  PortraitInfo{gettext_noop("Nabonidus"), ResourceId("io_new"), 7});
