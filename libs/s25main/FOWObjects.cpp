// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FOWObjects.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/noBaseBuilding.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "gameData/BuildingConsts.h"
#include "s25util/colors.h"

/// Berechnet die dunklere Spielerfarbe zum Zeichnen
unsigned CalcPlayerFOWDrawColor(const unsigned color)
{
    // Farbkomponenten extrahieren
    unsigned red = GetRed(color), green = GetGreen(color), blue = GetBlue(color);

    // "Skalieren"
    red = red * FOW_DRAW_COLOR_BRIGHTNESS / 0xFF;
    green = green * FOW_DRAW_COLOR_BRIGHTNESS / 0xFF;
    blue = blue * FOW_DRAW_COLOR_BRIGHTNESS / 0xFF;

    // Fertige Farbe zurückgeben
    return MakeColor(0xFF, red, green, blue);
}

////////////////////////////////////////////////////////////////////////////////////
// fowBuilding

fowBuilding::fowBuilding(const BuildingType type, const Nation nation) : type(type), nation(nation) {}

fowBuilding::fowBuilding(SerializedGameData& sgd) : type(sgd.Pop<BuildingType>()), nation(sgd.Pop<Nation>()) {}

void fowBuilding::Serialize(SerializedGameData& sgd) const
{
    sgd.PushEnum<uint8_t>(type);
    sgd.PushEnum<uint8_t>(nation);
}

void fowBuilding::Draw(DrawPoint drawPt) const
{
    noBaseBuilding::GetBuildingImage(type, nation).DrawFull(drawPt, FOW_DRAW_COLOR);
}

////////////////////////////////////////////////////////////////////////////////////
// fowBuildingSite

fowBuildingSite::fowBuildingSite(const bool planing, const BuildingType type, const Nation nation,
                                 const unsigned char build_progress)
    : planing(planing), type(type), nation(nation), build_progress(build_progress)
{}

fowBuildingSite::fowBuildingSite(SerializedGameData& sgd)
    : planing(sgd.PopBool()), type(sgd.Pop<BuildingType>()), nation(sgd.Pop<Nation>()),
      build_progress(sgd.PopUnsignedChar())
{}

void fowBuildingSite::Serialize(SerializedGameData& sgd) const
{
    sgd.PushBool(planing);
    sgd.PushEnum<uint8_t>(type);
    sgd.PushEnum<uint8_t>(nation);
    sgd.PushUnsignedChar(build_progress);
}

void fowBuildingSite::Draw(DrawPoint drawPt) const
{
    if(planing)
    {
        // Baustellenschild mit Schatten zeichnen
        LOADER.GetNationImage(nation, 450)->DrawFull(drawPt, FOW_DRAW_COLOR);
        LOADER.GetNationImage(nation, 451)->DrawFull(drawPt, COLOR_SHADOW);
    } else
    {
        // Baustellenstein und -schatten zeichnen
        LOADER.GetNationImage(nation, 455)->DrawFull(drawPt, FOW_DRAW_COLOR);
        LOADER.GetNationImage(nation, 456)->DrawFull(drawPt, COLOR_SHADOW);

        // bis dahin gebautes Haus zeichnen

        // ausrechnen, wie weit er ist
        unsigned progressRaw, progressBld;
        unsigned maxProgressRaw, maxProgressBld;

        if(BUILDING_COSTS[type].stones)
        {
            // Haus besteht aus Steinen und Brettern
            maxProgressRaw = BUILDING_COSTS[type].boards * 8;
            maxProgressBld = BUILDING_COSTS[type].stones * 8;
        } else
        {
            // Haus besteht nur aus Brettern, dann 50:50
            maxProgressBld = maxProgressRaw = BUILDING_COSTS[type].boards * 4;
        }
        progressRaw = std::min<unsigned>(build_progress, maxProgressRaw);
        progressBld = ((build_progress > maxProgressRaw) ? (build_progress - maxProgressRaw) : 0);

        LOADER.building_cache[nation][type].skeleton.drawPercent(drawPt, progressRaw * 100 / maxProgressRaw,
                                                                 FOW_DRAW_COLOR);
        LOADER.building_cache[nation][type].building.drawPercent(drawPt, progressBld * 100 / maxProgressBld,
                                                                 FOW_DRAW_COLOR);
    }
}

////////////////////////////////////////////////////////////////////////////////////
// fowFlag

fowFlag::fowFlag(const unsigned playerColor, const Nation nation, const FlagType flag_type)
    : color(CalcPlayerFOWDrawColor(playerColor)), nation(nation), flag_type(flag_type)
{}

fowFlag::fowFlag(SerializedGameData& sgd)
    : color(sgd.PopUnsignedInt()), nation(sgd.Pop<Nation>()), flag_type(sgd.Pop<FlagType>())
{}

void fowFlag::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedInt(color);
    sgd.PushEnum<uint8_t>(nation);
    sgd.PushEnum<uint8_t>(flag_type);
}

void fowFlag::Draw(DrawPoint drawPt) const
{
    LOADER.flag_cache[nation][flag_type][0].draw(drawPt, FOW_DRAW_COLOR, color);
}

////////////////////////////////////////////////////////////////////////////////////
// fowTree

fowTree::fowTree(const unsigned char type, const unsigned char size) : type(type), size(size) {}

fowTree::fowTree(SerializedGameData& sgd) : type(sgd.PopUnsignedChar()), size(sgd.PopUnsignedChar()) {}

void fowTree::Serialize(SerializedGameData& sgd) const
{
    sgd.PushUnsignedChar(type);
    sgd.PushUnsignedChar(size);
}

void fowTree::Draw(DrawPoint drawPt) const
{
    if(size == 3)
    {
        // Ausgewachsen
        LOADER.GetMapImageN(200 + type * 15)->DrawFull(drawPt, FOW_DRAW_COLOR);
        LOADER.GetMapImageN(350 + type * 15)->DrawFull(drawPt, COLOR_SHADOW);
    } else
    {
        LOADER.GetMapImageN(208 + type * 15 + size)->DrawFull(drawPt, FOW_DRAW_COLOR);
        LOADER.GetMapImageN(358 + type * 15 + size)->DrawFull(drawPt, COLOR_SHADOW);
    }
}

////////////////////////////////////////////////////////////////////////////////////
// fowGranite

fowGranite::fowGranite(const GraniteType type, const unsigned char state) : type(type), state(state) {}

fowGranite::fowGranite(SerializedGameData& sgd) : type(sgd.Pop<GraniteType>()), state(sgd.PopUnsignedChar()) {}

void fowGranite::Serialize(SerializedGameData& sgd) const
{
    sgd.PushEnum<uint8_t>(type);
    sgd.PushUnsignedChar(state);
}

void fowGranite::Draw(DrawPoint drawPt) const
{
    LOADER.granite_cache[type][state].DrawFull(drawPt, FOW_DRAW_COLOR);
}
