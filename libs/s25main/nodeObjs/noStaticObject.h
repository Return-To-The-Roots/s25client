// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"
#include "ogl/ITexture.h"
class SerializedGameData;

class noStaticObject : public noCoordBase
{
public:
    struct Textures
    {
        ITexture *bmp, *shadow;
    };

    noStaticObject(MapPoint pos, unsigned short id, unsigned short file = 0xFFFF, unsigned char size = 1,
                   NodalObjectType type = NodalObjectType::Object);
    noStaticObject(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;
    GO_Type GetGOT() const override { return GO_Type::Staticobject; }

    /// gibt die Item-ID zurück (nr in der jeweiligen File)
    unsigned short GetItemID() const { return id; }
    /// gibt die Nr der File zurück)
    unsigned short GetItemFile() const { return file; }
    /// gibt die Größe des Objekts zurück.
    unsigned char GetSize() const { return size; }

    BlockingManner GetBM() const override;

    /// zeichnet das Objekt.
    void Draw(DrawPoint drawPt) override;

    static Textures getTextures(unsigned short file, unsigned short id);
    bool IsAnimated() const;

protected:
    static bool IsOpenGateway(unsigned short file, unsigned short id);
    unsigned short id;
    unsigned short file;
    unsigned char size;
    Textures textures{};
};
