// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "gameTypes/PactTypes.h"
#include "mygettext/mygettext.h"

const helpers::EnumArray<const char*, PactType> PACT_NAMES = {
  gettext_noop("Treaty of alliance"), gettext_noop("Non-aggression pact"), gettext_noop("One-sided alliance")};
