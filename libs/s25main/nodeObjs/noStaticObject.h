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
                   NodalObjectType type = NOP_OBJECT);
    noStaticObject(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override { Destroy_noStaticObject(); }

    /// gibt die Item-ID zurück (nr in der jeweiligen File)
    unsigned short GetItemID() const { return id; }
    /// gibt die Nr der File zurück)
    unsigned short GetItemFile() const { return file; }
    /// gibt die Größe des Objekts zurück.
    unsigned char GetSize() const { return size; }

    BlockingManner GetBM() const override;

    /// zeichnet das Objekt.
    void Draw(DrawPoint drawPt) override;

    /// Serialisierungsfunktionen
protected:
    void Serialize_noStaticObject(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noStaticObject(sgd); }

    GO_Type GetGOT() const override { return GOT_STATICOBJECT; }

protected:
    void Destroy_noStaticObject();

    unsigned short id;
    unsigned short file;
    unsigned char size;
};
