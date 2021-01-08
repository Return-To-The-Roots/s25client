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

#include "DrawPoint.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/Nation.h"
#include <cstdint>

class SerializedGameData;

/// Typen für die FOW Objekte
enum class FoW_Type : uint8_t
{
    Nothing,
    Building,
    Buildingsite,
    Flag,
    Tree,
    Granite
};
constexpr auto maxEnumValue(FoW_Type)
{
    return FoW_Type::Granite;
}

/// Helligkeit der Objekte beim Zeichnen
const unsigned FOW_DRAW_COLOR_BRIGHTNESS = 0x80;
/// Farbe für das Zeichnen
const unsigned FOW_DRAW_COLOR = 0xFF808080;

/// Berechnet die dunklere Spielerfarbe zum Zeichnen
unsigned CalcPlayerFOWDrawColor(unsigned color);

/// Visuelles Objekt im Nebel, nur zur sichtbaren "Erinnerung",
/// was ein bestimmter Spieler gesehen hat
class FOWObject
{
public:
    virtual ~FOWObject();
    /// An x,y zeichnen.
    virtual void Draw(DrawPoint drawPt) const = 0;
    /// Serialisierungsfunktion.
    virtual void Serialize(SerializedGameData& sgd) const = 0;
    /// Gibt Typ zurück
    virtual FoW_Type GetType() const = 0;
};

/// Gebäude im Nebel
class fowBuilding : public FOWObject
{
private:
    /// Typ des Gebäudes
    const BuildingType type;
    /// Volk des Gebäudes (muss extra gespeichert werden, da ja auch z.B. fremde Gebäude erobert werden können)
    const Nation nation;

public:
    fowBuilding(BuildingType type, Nation nation);
    fowBuilding(SerializedGameData& sgd);
    void Serialize(SerializedGameData& sgd) const override;
    void Draw(DrawPoint drawPt) const override;
    FoW_Type GetType() const override { return FoW_Type::Building; }
};

/// Baustelle
class fowBuildingSite : public FOWObject
{
private:
    /// Wird planiert?
    const bool planing;
    /// Typ des Gebäudes
    const BuildingType type;
    /// Volk des Gebäudes (muss extra gespeichert werden, da ja auch z.B. fremde Gebäude erobert werden können)
    const Nation nation;
    /// Gibt den Baufortschritt an, wie hoch das Gebäude schon gebaut ist, gemessen in 8 Stufen für jede verbaute Ware
    const unsigned char build_progress;

public:
    fowBuildingSite(bool planing, BuildingType type, Nation nation, unsigned char build_progress);
    fowBuildingSite(SerializedGameData& sgd);
    void Serialize(SerializedGameData& sgd) const override;
    void Draw(DrawPoint drawPt) const override;
    FoW_Type GetType() const override { return FoW_Type::Buildingsite; }
};

/// Flagge
class fowFlag : public FOWObject
{
private:
    const unsigned color;
    const Nation nation;
    /// Flaggenart
    const FlagType flag_type;

public:
    fowFlag(unsigned playerColor, Nation nation, FlagType flag_type);
    fowFlag(SerializedGameData& sgd);
    void Serialize(SerializedGameData& sgd) const override;
    void Draw(DrawPoint drawPt) const override;
    FoW_Type GetType() const override { return FoW_Type::Flag; }
};

/// Baum
class fowTree : public FOWObject
{
private:
    /// Typ des Baumes (also welche Baumart)
    const unsigned char type;
    /// Größe des Baumes (0-2, 3 = aufgewachsen!)
    const unsigned char size;

public:
    fowTree(unsigned char type, unsigned char size);
    fowTree(SerializedGameData& sgd);
    void Serialize(SerializedGameData& sgd) const override;
    void Draw(DrawPoint drawPt) const override;
    FoW_Type GetType() const override { return FoW_Type::Tree; }
};

/// Granitblock
class fowGranite : public FOWObject
{
private:
    const GraniteType type;    /// Welcher Typ ( gibt 2 )
    const unsigned char state; /// Status, 0 - 5, von sehr wenig bis sehr viel

public:
    fowGranite(GraniteType type, unsigned char state);
    fowGranite(SerializedGameData& sgd);
    void Serialize(SerializedGameData& sgd) const override;
    void Draw(DrawPoint drawPt) const override;
    FoW_Type GetType() const override { return FoW_Type::Granite; }
};
