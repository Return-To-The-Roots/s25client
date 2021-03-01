// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "noCoordBase.h"
class SerializedGameData;

class noStaticObject : public noCoordBase
{
public:
    noStaticObject(MapPoint pos, unsigned short id, unsigned short file = 0xFFFF, unsigned char size = 1,
                   NodalObjectType type = NodalObjectType::Object);
    noStaticObject(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;
    GO_Type GetGOT() const final { return GO_Type::Staticobject; }

    /// gibt die Item-ID zurück (nr in der jeweiligen File)
    unsigned short GetItemID() const { return id; }
    /// gibt die Nr der File zurück)
    unsigned short GetItemFile() const { return file; }
    /// gibt die Größe des Objekts zurück.
    unsigned char GetSize() const { return size; }

    BlockingManner GetBM() const override;

    /// zeichnet das Objekt.
    void Draw(DrawPoint drawPt) override;

protected:
    unsigned short id;
    unsigned short file;
    unsigned char size;
};
