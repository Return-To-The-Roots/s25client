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

#ifndef IngameMinimap_h__
#define IngameMinimap_h__

#include "Minimap.h"
#include "gameTypes/MapTypes.h"
#include <vector>

class GameWorldViewer;

class IngameMinimap: public Minimap
{
    /// Referenz auf den GameWorldViewer
    const GameWorldViewer& gwv;
    /// Speichert die einzelnen Veränderungen eines jeden Mappunktes, damit nicht unnötigerweise
    /// in einem GF mehrmals der Mappunkt verändert wird
    std::vector<bool> nodes_updated;
    /// Liste mit allen Punkten, die geändert werden müssen
    std::vector<MapPoint> nodesToUpdate;


    /// Für jeden einzelnen Knoten speichern, welches Objekt hier dominiert, also wessen Pixel angezeigt wird
    enum DrawnObject
    {
        DO_INVALID = 0,
        DO_INVISIBLE, /// im im vollständigem Dunklen
        DO_TERRAIN, /// Nur Terrain oder Baum und Granit ohne irgendwas
        DO_PLAYER, /// Nur Terrain oder Baum und Granit mit Spielerterritorium dazu
        DO_BUILDING, /// Gebäude
        DO_ROAD /// Straße
    };

    std::vector<DrawnObject> dos;

    /// Einzelne Dinge anzeigen oder nicht anzeigen
    bool territory; /// Länder der Spieler
    bool houses; /// Häuser
    bool roads; /// Straßen

public:
    IngameMinimap(const GameWorldViewer& gwv);


    /// Merkt, vor dass ein bestimmter Punkt aktualisiert werden soll
    void UpdateNode(const MapPoint pt);

    /// Updatet die gesamte Minimap
    void UpdateAll();

    /// Die einzelnen Dinge umschalten
    void ToggleTerritory();
    void ToggleHouses();
    void ToggleRoads();

protected:

    /// Berechnet die Farbe für einen bestimmten Pixel der Minimap (t = Terrain1 oder 2)
    unsigned CalcPixelColor(const MapPoint pt, const unsigned t) override;
    /// Berechnet für einen bestimmten Punkt und ein Dreieck die normale Terrainfarbe
    unsigned CalcTerrainColor(const MapPoint pt, const unsigned t);
    /// Prüft ob an einer Stelle eine Straße gezeichnet werden muss
    bool IsRoad(const MapPoint pt, const Visibility visibility);
    /// Berechnet Spielerfarbe mit in eine gegebene Farbe mit ein (player muss mit +1 gegeben sein!)
    unsigned CombineWithPlayerColor(const unsigned color, const unsigned char player) const;
    /// Zusätzliche Dinge, die die einzelnen Maps vor dem Zeichenvorgang zu tun haben
    /// in dem Falle: Karte aktualisieren
    void BeforeDrawing() override;
    /// Alle Punkte Updaten, bei denen das DrawnObject gleich dem übergebenen drawn_object ist
    void UpdateAll(const DrawnObject drawn_object);
};

#endif // IngameMinimap_h__
