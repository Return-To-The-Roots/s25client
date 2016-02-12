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

#ifndef MessageTypes_h__
#define MessageTypes_h__

/// Post-Nachrichten-Kategorien
enum PostMessageCategory
{
    PMC_MILITARY, // ImagePostMsgWithLocation
    PMC_GEOLOGIST, // PostMsgWithLocation
    PMC_GENERAL, // ImagePostMsgWithLocation
    PMC_SAVEWARNING, // PostMsg
    PMC_DIPLOMACY, // DiplomacyPostQuestion (man braucht vll noch verschiedene?)
    PMC_OTHER  // PostMsg
};

/// Post-Nachrichten-Typen (entspricht den Klassen in PostMsg.h)
enum PostMessageType
{
    PMT_NORMAL,               // PostMsg
    PMT_WITH_LOCATION,        // PostMsgWithLocation
    PMT_IMAGE_WITH_LOCATION,  // ImagePostMsgWithLocation
    PMT_DIPLOMACYQUESTION,             // DiplomacyPostQuestion
    PMT_DIPLOMACYINFO,            // DiplomacyPostInfo
    PMT_SHIP
};

/// Maximale Nachrichtenanzahl im Briefkasten
const unsigned MAX_POST_MESSAGES = 20;

#endif // MessageTypes_h__
