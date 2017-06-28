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

#ifndef GameMessage_GameCommand_h__
#define GameMessage_GameCommand_h__

#include "GameMessage.h"
#include "GameCommand.h"
#include <vector>

class Serializer;

struct AsyncChecksum{
    unsigned randChecksum;
    unsigned objCt;
    unsigned objIdCt;
    AsyncChecksum();
    explicit AsyncChecksum(unsigned randChecksum);
    AsyncChecksum(unsigned randChecksum, unsigned objCt, unsigned objIdCt);

    inline bool operator==(const AsyncChecksum& rhs) const;
    inline bool operator!=(const AsyncChecksum& rhs) const;
};

class GameMessage_GameCommand : public GameMessage
{
    public:
        /// Checksumme, die der Spieler übermittelt
        AsyncChecksum checksum;
        /// Die einzelnen GameCommands
        std::vector<gc::GameCommandPtr> gcs;

    public:

        GameMessage_GameCommand(); //-V730
        GameMessage_GameCommand(const unsigned char player, const AsyncChecksum& checksum, const std::vector<gc::GameCommandPtr>& gcs);

        void Serialize(Serializer& ser) const override;
        void Deserialize(Serializer& ser) override;
        void Run(MessageInterface* callback) override;
};

bool AsyncChecksum::operator==(const AsyncChecksum& rhs) const
{
    return randChecksum == rhs.randChecksum &&
               objCt == rhs.objCt &&
             objIdCt == rhs.objIdCt;
}

bool AsyncChecksum::operator!=(const AsyncChecksum& rhs) const
{
    return !(*this == rhs);
}

#endif // GameMessage_GameCommand_h__
