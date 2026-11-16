// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noStaticObject.h"
#include "noExtension.h"

#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include <stdexcept>

/**
 *  Konstruktor von @p noStaticObject.
 *
 *  @param[in] id   Nr der Grafik
 *  @param[in] file Nr der Datei (0xFFFF map_?_z.lst, 0-5 mis?bobs.lst)
 *  @param[in] size Größe des Objekts
 *  @param[in] type Typ des Objekts
 */
noStaticObject::noStaticObject(const MapPoint pos, unsigned short id, unsigned short file, unsigned char size,
                               NodalObjectType type)
    : noCoordBase(type, pos), id(id), file(file), size(size)
{
    // sind wir ein "Schloss" Objekt?
    if(getSize() == 2)
    {
        for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            MapPoint nb = world->getNeighbour(pos, dir);
            world->destroyNO(nb, false);
            world->setNO(nb, new noExtension(this));
        }
    }
}

noStaticObject::noStaticObject(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), id(sgd.PopUnsignedShort()), file(sgd.PopUnsignedShort()), size(sgd.PopUnsignedChar())
{}

void noStaticObject::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedShort(id);
    sgd.PushUnsignedShort(file);
    sgd.PushUnsignedChar(size);
}

void noStaticObject::destroy()
{
    // waren wir ein "Schloss" Objekt?
    if(getSize() == 2)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
            world->destroyNO(world->getNeighbour(pos, i));
    }

    noCoordBase::destroy();
}

BlockingManner noStaticObject::getBM() const
{
    return (size == 0) ? BlockingManner::None : BlockingManner::Single;
}

/**
 *  zeichnet das Objekt.
 *
 *  @param[in] id Nr der Grafik
 *  @param[in] file Nr der Datei (0xFFFF map_?_z.lst, 0-5 mis?bobs.lst)
 */
void noStaticObject::draw(DrawPoint drawPt)
{
    if(isAnimated() || !textures.bmp)
    {
        textures = getTextures(file, id);
        RTTR_Assert(textures.bmp);
    }

    // Bild zeichnen
    textures.bmp->drawFull(drawPt);

    // Schatten zeichnen
    if(textures.shadow)
        textures.shadow->drawFull(drawPt, COLOR_SHADOW);
}

noStaticObject::Textures noStaticObject::getTextures(unsigned short file, unsigned short id)
{
    Textures textures{};
    if(isOpenGateway(file, id))
        textures.bmp = &LOADER.gateway_cache[GAMECLIENT.getGlobalAnimation(4, 5, 4, 0) + 1];
    else if(file == 0xFFFF)
        textures = {LOADER.getMapTexture(id), LOADER.getMapTexture(id + 100)};
    else if(file < 8)
    {
        static const std::array<ResourceId, 8> files = {"mis0bobs", "mis1bobs", "mis2bobs",        "mis3bobs",
                                                        "mis4bobs", "mis5bobs", "charburner_bobs", "wine_bobs"};
        textures.bmp = LOADER.getTextureN(files[file], id);
        // Use only shadows where available
        if(file < 6)
            textures.shadow = LOADER.getTextureN(files[file], id + 1);
        else if(file == 7)
            textures.shadow = LOADER.getTextureN(files[file], id + 10);
    } else
        throw std::runtime_error("Invalid file number for static object");

    return textures;
}

bool noStaticObject::isAnimated() const
{
    return isOpenGateway(file, id);
}

bool noStaticObject::isOpenGateway(unsigned short file, unsigned short id)
{
    return file == 0xFFFF && id == 561;
}
