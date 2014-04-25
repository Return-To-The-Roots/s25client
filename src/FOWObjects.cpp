// $Id: FOWObjects.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "FOWObjects.h"
#include "SerializedGameData.h"
#include "GameClient.h"
#include "BuildingConsts.h"




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

FOWObject::~FOWObject()
{
}

////////////////////////////////////////////////////////////////////////////////////
// fowNothing


fowNothing::fowNothing()
{}
fowNothing::fowNothing(SerializedGameData* sgd)
{}
void fowNothing::Serialize(SerializedGameData* sgd) const
{}
void fowNothing::Draw(int x, int y) const
{}

////////////////////////////////////////////////////////////////////////////////////
// fowBuilding


fowBuilding::fowBuilding(const BuildingType type, const Nation nation) : type(type), nation(nation)
{}

fowBuilding::fowBuilding(SerializedGameData* sgd) :
    type(BuildingType(sgd->PopUnsignedChar())),
    nation(Nation(sgd->PopUnsignedChar()))
{}

void fowBuilding::Serialize(SerializedGameData* sgd) const
{
    sgd->PushUnsignedChar(static_cast<unsigned char>(type));
    sgd->PushUnsignedChar(static_cast<unsigned char>(nation));
}

void fowBuilding::Draw(int x, int y) const
{
    if (type == BLD_CHARBURNER)
    {
        LOADER.GetImageN("charburner", nation * 8 + 1)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
    }
    else
    {
        LOADER.GetNationImageN(nation, 250 + 5 * type)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
        // ACHTUNG nicht jedes Gebäude hat einen Schatten !!
        if(LOADER.GetNationImageN(nation, 250 + 5 * type + 1))
            LOADER.GetNationImageN(nation, 250 + 5 * type + 1)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }
}

////////////////////////////////////////////////////////////////////////////////////
// fowBuildingSite


fowBuildingSite::fowBuildingSite(const bool planing, const BuildingType type, const Nation nation, const unsigned char build_progress)
    : planing(planing), type(type), nation(nation), build_progress(build_progress)
{}

fowBuildingSite::fowBuildingSite(SerializedGameData* sgd) :
    planing(sgd->PopBool()),
    type(BuildingType(sgd->PopUnsignedChar())),
    nation(Nation(sgd->PopUnsignedChar())),
    build_progress(sgd->PopUnsignedChar())
{}

void fowBuildingSite::Serialize(SerializedGameData* sgd) const
{
    sgd->PushBool(planing);
    sgd->PushUnsignedChar(static_cast<unsigned char>(type));
    sgd->PushUnsignedChar(static_cast<unsigned char>(nation));
    sgd->PushUnsignedChar(build_progress);
}

void fowBuildingSite::Draw(int x, int y) const
{
    if(planing)
    {
        // Baustellenschild mit Schatten zeichnen
        LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation, 450)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
        LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation, 451)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }
    else
    {
        // Baustellenstein und -schatten zeichnen
        LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation, 455)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
        LOADER.GetNationImageN(GAMECLIENT.GetLocalPlayer()->nation, 456)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);


        // bis dahin gebautes Haus zeichnen

        // Rohbau

        // ausrechnen, wie weit er ist
        unsigned int p1 = 0, p2 = 0;

        if(BUILDING_COSTS[nation][type].stones)
        {
            // Haus besteht aus Steinen und Brettern
            p1 = min<unsigned int>(build_progress, BUILDING_COSTS[nation][type].boards * 8);
            p2 = BUILDING_COSTS[nation][type].boards * 8;
        }
        else
        {
            // Haus besteht nur aus Brettern, dann 50:50
            p1 = min<unsigned int>(build_progress, BUILDING_COSTS[nation][type].boards * 4);
            p2 = BUILDING_COSTS[nation][type].boards * 4;
        }

        glArchivItem_Bitmap* image;
        unsigned short progress, build_height;

        // Normal
        image = LOADER.GetNationImageN(nation, 250 + 5 * type + 2);
        if(image)
        {
            progress = p1 * image->getHeight() / p2;
            build_height = image->getHeight() - progress;
            if(progress != 0)
                image->Draw(x, y + build_height, 0, 0, 0, build_height, 0, progress, FOW_DRAW_COLOR);
        }

        // Schatten
        image = LOADER.GetNationImageN(nation, 250 + 5 * type + 3);
        if(image)
        {
            progress = p1 * image->getHeight() / p2;
            build_height = image->getHeight() - progress;
            if(progress != 0)
                image->Draw(x, y + build_height, 0, 0, 0, build_height, 0, progress, COLOR_SHADOW);
        }

        // Das richtige Haus
        if(BUILDING_COSTS[nation][type].stones)
        {
            // Haus besteht aus Steinen und Brettern
            p1 = ((build_progress >  BUILDING_COSTS[nation][type].boards * 8) ? (build_progress - BUILDING_COSTS[nation][type].boards * 8) : 0);
            p2 = BUILDING_COSTS[nation][type].stones * 8;
        }
        else
        {
            // Haus besteht nur aus Brettern, dann 50:50
            p1 = ((build_progress >  BUILDING_COSTS[nation][type].boards * 4) ? (build_progress - BUILDING_COSTS[nation][type].boards * 4) : 0);
            p2 = BUILDING_COSTS[nation][type].boards * 4;
        }

        // Normal
        image = LOADER.GetNationImageN(nation, 250 + 5 * type);
        if(image)
        {
            progress = p1 * image->getHeight() / p2;
            build_height = image->getHeight() - progress;
            if(progress != 0)
                image->Draw(x, y + build_height, 0, 0, 0, build_height, 0, progress, FOW_DRAW_COLOR);
        }


        image = LOADER.GetNationImageN(nation, 250 + 5 * type + 1);
        if(image)
        {
            progress = p1 * image->getHeight() / p2;
            build_height = image->getHeight() - progress;
            if(progress != 0)
                image->Draw(x, y + build_height, 0, 0, 0, build_height, 0, progress, COLOR_SHADOW);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////
// fowFlag


fowFlag::fowFlag(const unsigned char player, const FlagType flag_type) : player(player), flag_type(flag_type)
{}

fowFlag::fowFlag(SerializedGameData* sgd) :
    player(sgd->PopUnsignedChar()),
    flag_type(FlagType(sgd->PopUnsignedChar()))
{}

void fowFlag::Serialize(SerializedGameData* sgd) const
{
    sgd->PushUnsignedChar(player);
    sgd->PushUnsignedChar(static_cast<unsigned char>(flag_type));
}

void fowFlag::Draw(int x, int y) const
{
    // Flagge
    LOADER.GetNationImageN(GAMECLIENT.GetPlayer(player)->nation, 100 + flag_type * 20)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR, CalcPlayerFOWDrawColor(COLORS[GAMECLIENT.GetPlayer(player)->color]));
    // Schatten
    LOADER.GetNationImageN(GAMECLIENT.GetPlayer(player)->nation, 100 + flag_type * 20 + 10)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
}

////////////////////////////////////////////////////////////////////////////////////
// fowTree

fowTree::fowTree(const unsigned char type, const unsigned char size) : type(type), size(size)
{}

fowTree::fowTree(SerializedGameData* sgd) :
    type(sgd->PopUnsignedChar()),
    size(sgd->PopUnsignedChar())
{}

void fowTree::Serialize(SerializedGameData* sgd) const
{
    sgd->PushUnsignedChar(type);
    sgd->PushUnsignedChar(size);
}

void fowTree::Draw(int x, int y) const
{
    if(size == 3)
    {
        // Ausgewachsen
        LOADER.GetMapImageN(200 + type * 15)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
        LOADER.GetMapImageN(350 + type * 15)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }
    else
    {
        LOADER.GetMapImageN(208 + type * 15 + size)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
        LOADER.GetMapImageN(358 + type * 15 + size)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }
}

////////////////////////////////////////////////////////////////////////////////////
// fowGranite

fowGranite::fowGranite(const GraniteType type, const unsigned char state) : type(type), state(state)
{}

fowGranite::fowGranite(SerializedGameData* sgd) :
    type(GraniteType(sgd->PopUnsignedChar())),
    state(sgd->PopUnsignedChar())
{}

void fowGranite::Serialize(SerializedGameData* sgd) const
{
    sgd->PushUnsignedChar(static_cast<unsigned char>(type));
    sgd->PushUnsignedChar(state);
}

void fowGranite::Draw(int x, int y) const
{
    LOADER.GetMapImageN(516 + type * 6 + state)->Draw(x, y, 0, 0, 0, 0, 0, 0, FOW_DRAW_COLOR);
    LOADER.GetMapImageN(616 + type * 6 + state)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
}
