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

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;
    GO_Type getGOT() const override { return GO_Type::Staticobject; }

    /// gibt die Item-ID zurück (nr in der jeweiligen File)
    unsigned short getItemID() const { return id; }
    /// gibt die Nr der File zurück)
    unsigned short getItemFile() const { return file; }
    /// gibt die Größe des Objekts zurück.
    unsigned char getSize() const { return size; }

    BlockingManner getBM() const override;

    /// zeichnet das Objekt.
    void draw(DrawPoint drawPt) override;

    static Textures getTextures(unsigned short file, unsigned short id);
    bool isAnimated() const;

protected:
    static bool isOpenGateway(unsigned short file, unsigned short id);
    unsigned short id;
    unsigned short file;
    unsigned char size;
    Textures textures{};
};
