// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    if(GetSize() == 2)
    {
        for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            MapPoint nb = world->GetNeighbour(pos, dir);
            world->DestroyNO(nb, false);
            world->SetNO(nb, new noExtension(this));
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

void noStaticObject::Destroy()
{
    // waren wir ein "Schloss" Objekt?
    if(GetSize() == 2)
    {
        for(const Direction i : {Direction::West, Direction::NorthWest, Direction::NorthEast})
            world->DestroyNO(world->GetNeighbour(pos, i));
    }

    noCoordBase::Destroy();
}

BlockingManner noStaticObject::GetBM() const
{
    return (size == 0) ? BlockingManner::None : BlockingManner::Single;
}

/**
 *  zeichnet das Objekt.
 *
 *  @param[in] id Nr der Grafik
 *  @param[in] file Nr der Datei (0xFFFF map_?_z.lst, 0-5 mis?bobs.lst)
 */
void noStaticObject::Draw(DrawPoint drawPt)
{
    glArchivItem_Bitmap *bitmap = nullptr, *shadow = nullptr;

    if((file == 0xFFFF) && (id == 561))
    {
        LOADER.gateway_cache[GAMECLIENT.GetGlobalAnimation(4, 5, 4, 0) + 1].draw(drawPt);
        return;
    } else if(file == 0xFFFF)
    {
        bitmap = LOADER.GetMapImageN(id);
        shadow = LOADER.GetMapImageN(id + 100);
    } else if(file < 7)
    {
        static const std::array<ResourceId, 7> files = {"mis0bobs", "mis1bobs", "mis2bobs",       "mis3bobs",
                                                        "mis4bobs", "mis5bobs", "charburner_bobs"};
        bitmap = LOADER.GetImageN(files[file], id);
        // Use only shadows where available
        if(file < 6)
            shadow = LOADER.GetImageN(files[file], id + 1);
    } else
        throw std::runtime_error("Invalid file number for static object");

    RTTR_Assert(bitmap);

    // Bild zeichnen
    bitmap->DrawFull(drawPt);

    // Schatten zeichnen
    if(shadow)
        shadow->DrawFull(drawPt, COLOR_SHADOW);
}
