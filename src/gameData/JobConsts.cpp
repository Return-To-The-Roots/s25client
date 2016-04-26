// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "gameData/JobConsts.h"
#include "mygettext.h"

const boost::array<std::string, JOB_TYPES_COUNT> JOB_NAMES =
{{
    gettext_noop("Helper"),
    gettext_noop("Woodchopper"),
    gettext_noop("Fisher"),
    gettext_noop("Ranger"),
    gettext_noop("Carpenter"),
    gettext_noop("Stonemason"),
    gettext_noop("Huntsman"),
    gettext_noop("Farmer"),
    gettext_noop("Miller"),
    gettext_noop("Baker"),
    gettext_noop("Butcher"),
    gettext_noop("Miner"),
    gettext_noop("Brewer"),
    gettext_noop("Pig breeder"),
    gettext_noop("Donkey breeder"),
    gettext_noop("Iron founder"),
    gettext_noop("Minter"),
    gettext_noop("Metalworker"),
    gettext_noop("Armorer"),
    gettext_noop("Builder"),
    gettext_noop("Planer"),
    gettext_noop("Private"),
    gettext_noop("Private first class"),
    gettext_noop("Sergeant"),
    gettext_noop("Officer"),
    gettext_noop("General"),
    gettext_noop("Geologist"),
    gettext_noop("Shipwright"),
    gettext_noop("Scout"),
    gettext_noop("Pack donkey"),
    "", // Bootsträger
    gettext_noop("Charburner")
}};
