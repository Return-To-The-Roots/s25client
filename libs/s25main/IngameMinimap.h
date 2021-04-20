// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Minimap.h"
#include "gameTypes/MapTypes.h"
#include <vector>

class GameWorldViewer;

class IngameMinimap : public Minimap
{
    /// Referenz auf den GameWorldViewer
    const GameWorldViewer& gwv;
    /// Speichert die einzelnen Veränderungen eines jeden Mappunktes, damit nicht unnötigerweise
    /// in einem GF mehrmals der Mappunkt verändert wird
    std::vector<bool> nodes_updated;
    /// Liste mit allen Punkten, die geändert werden müssen
    std::vector<MapPoint> nodesToUpdate;

    /// Für jeden einzelnen Knoten speichern, welches Objekt hier dominiert, also wessen Pixel angezeigt wird
    enum class DrawnObject
    {
        Invalid,
        Invisible, /// im im vollständigem Dunklen
        Terrain,   /// Nur Terrain oder Baum und Granit ohne irgendwas
        Player,    /// Nur Terrain oder Baum und Granit mit Spielerterritorium dazu
        Buidling,  /// Gebäude
        Road       /// Straße
    };

    std::vector<DrawnObject> dos;

    /// Einzelne Dinge anzeigen oder nicht anzeigen
    bool territory; /// Länder der Spieler
    bool houses;    /// Häuser
    bool roads;     /// Straßen

public:
    IngameMinimap(const GameWorldViewer& gwv);

    /// Merkt, vor dass ein bestimmter Punkt aktualisiert werden soll
    void UpdateNode(MapPoint pt);

    /// Updatet die gesamte Minimap
    void UpdateAll();

    /// Die einzelnen Dinge umschalten
    void ToggleTerritory();
    void ToggleHouses();
    void ToggleRoads();

protected:
    /// Berechnet die Farbe für einen bestimmten Pixel der Minimap (t = Terrain1 oder 2)
    unsigned CalcPixelColor(MapPoint pt, unsigned t) override;
    /// Berechnet für einen bestimmten Punkt und ein Dreieck die normale Terrainfarbe
    unsigned CalcTerrainColor(MapPoint pt, unsigned t);
    /// Prüft ob an einer Stelle eine Straße gezeichnet werden muss
    bool IsRoad(MapPoint pt, Visibility visibility);
    /// Berechnet Spielerfarbe mit in eine gegebene Farbe mit ein (player muss mit +1 gegeben sein!)
    unsigned CombineWithPlayerColor(unsigned color, unsigned char player) const;
    /// Zusätzliche Dinge, die die einzelnen Maps vor dem Zeichenvorgang zu tun haben
    /// in dem Falle: Karte aktualisieren
    void BeforeDrawing() override;
    /// Alle Punkte Updaten, bei denen das DrawnObject gleich dem übergebenen drawn_object ist
    void UpdateAll(DrawnObject drawn_object);
};
