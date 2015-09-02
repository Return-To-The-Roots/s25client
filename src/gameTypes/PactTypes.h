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

#ifndef PactTypes_h__
#define PactTypes_h__

#include <string>

/// Types of pacts
enum PactType
{
    TREATY_OF_ALLIANCE = 0,
    NON_AGGRESSION_PACT
};

/// Number of the various pacts
const unsigned PACTS_COUNT = 2;

/// Names of the possible pacts
const std::string PACT_NAMES[PACTS_COUNT] =
{
    gettext_noop("Treaty of alliance"),
    gettext_noop("Non-aggression pact")
};

#endif // PactTypes_h__
