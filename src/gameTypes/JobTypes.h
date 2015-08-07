// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef JobTypes_h__
#define JobTypes_h__

#include "mygettext.h"

enum Job
{
	JOB_HELPER            =  0,
	JOB_WOODCUTTER        =  1,
	JOB_FISHER            =  2,
	JOB_FORESTER          =  3,
	JOB_CARPENTER         =  4,
	JOB_STONEMASON        =  5,
	JOB_HUNTER            =  6,
	JOB_FARMER            =  7,
	JOB_MILLER            =  8,
	JOB_BAKER             =  9,
	JOB_BUTCHER           = 10,
	JOB_MINER             = 11,
	JOB_BREWER            = 12,
	JOB_PIGBREEDER        = 13,
	JOB_DONKEYBREEDER     = 14,
	JOB_IRONFOUNDER       = 15,
	JOB_MINTER            = 16,
	JOB_METALWORKER       = 17,
	JOB_ARMORER           = 18,
	JOB_BUILDER           = 19,
	JOB_PLANER            = 20,
	JOB_PRIVATE           = 21,
	JOB_PRIVATEFIRSTCLASS = 22,
	JOB_SERGEANT          = 23,
	JOB_OFFICER           = 24,
	JOB_GENERAL           = 25,
	JOB_GEOLOGIST         = 26,
	JOB_SHIPWRIGHT        = 27,
	JOB_SCOUT             = 28,
	JOB_PACKDONKEY        = 29,
	JOB_BOATCARRIER       = 30,
	JOB_CHARBURNER        = 31,
	JOB_NOTHING           = 32
};

// Anzahl an unterschiedlichen Berufstypen
const unsigned JOB_TYPES_COUNT = JOB_NOTHING;

const std::string JOB_NAMES[JOB_TYPES_COUNT] =
{
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
};
#endif // JobTypes_h__
