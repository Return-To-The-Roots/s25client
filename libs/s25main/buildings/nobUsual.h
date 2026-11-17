// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBuilding.h"
#include "gameTypes/GoodTypes.h"
#include <array>
#include <list>
#include <vector>

class Ware;
class nofBuildingWorker;
class SerializedGameData;
class noFigure;
class noRoadNode;
class GameEvent;

// Gewöhnliches Gebäude mit einem Arbeiter und Waren
class nobUsual : public noBuilding
{
protected:
    /// Der Typ, der hier arbeitet
    nofBuildingWorker* worker;
    /// Produktivität
    unsigned short productivity;
    /// Produktion eingestellt? (letzteres nur visuell, um Netzwerk-Latenzen zu verstecken)
    bool disableProduction, disableProductionVirtual;
    /// Warentyp, den er zuletzt bestellt hatte (bei >1 Waren)
    unsigned char lastOrderedWare;
    /// Rohstoffe, die zur Produktion benötigt werden
    std::array<uint8_t, 3> numWares;
    /// Bestellte Waren
    std::vector<std::list<Ware*>> orderedWares;
    /// Bestell-Ware-Event
    const GameEvent* orderware_ev;
    /// Rechne-Produktivität-aus-Event
    const GameEvent* productivity_ev;
    /// Letzte Produktivitäten (Durchschnitt = Gesamt produktivität), vorne das neuste !
    std::array<uint16_t, 6> lastProductivities;
    /// How many GFs he did not work since the last productivity calculation
    unsigned short numGfNotWorking;
    /// Since which GF he did not work (0xFFFFFFFF = currently working)
    unsigned sinceNotWorking;
    /// Did we notify the player that we are out of resources?
    bool outOfRessourcesMsgSent;

    friend class SerializedGameData;
    friend class BuildingFactory;
    nobUsual(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobUsual(SerializedGameData& sgd, unsigned obj_id);

    void DestroyBuilding() override;

public:
    /// Wird gerade gearbeitet oder nicht?
    bool is_working;

    ~nobUsual() override;

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const override { return GO_Type::NobUsual; }
    unsigned getMilitaryRadius() const override { return 0; }

    void draw(DrawPoint drawPt) override;

    bool hasWorker() const;

    /// Event-Handler
    void handleEvent(unsigned id) override;
    /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
    void addWare(std::unique_ptr<Ware> ware) override;
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool freePlaceAtFlag() override;
    /// Eine bestellte Ware konnte doch nicht kommen
    void wareLost(Ware& ware) override;
    /// Wird aufgerufen, wenn ein Arbeiter für das Gebäude gefunden werden konnte
    void gotWorker(Job job, noFigure& worker) override;
    /// Wird vom Arbeiter aufgerufen, wenn er im Gebäude angekommen ist
    void workerArrived();
    /// Wird vom Arbeiter aufgerufen, wenn er nicht (mehr) zur Arbeit kommen kann
    void workerLost();

    /// Gibt den Warenbestand (eingehende Waren - Rohstoffe) zurück
    unsigned char getNumWares(unsigned id) const { return numWares[id]; }
    /// Prüft, ob Waren für einen Arbeitsschritt vorhanden sind
    bool waresAvailable();
    /// Verbraucht Waren
    void consumeWares();

    /// Berechnet Punktewertung für Ware type
    unsigned calcDistributionPoints(GoodType type);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void takeWare(Ware* ware) override;

    /// Bestellte Waren
    bool areThereAnyOrderedWares() const;

    unsigned short getProductivity() const;
    const nofBuildingWorker* getWorker() const { return worker; }

    /// Stoppt/Erlaubt Produktion (visuell)
    void toggleProductionVirtual() { disableProductionVirtual = !disableProductionVirtual; }
    /// Stoppt/Erlaubt Produktion (real)
    void SetProductionEnabled(bool enabled);
    /// Fragt ab, ob Produktion ausgeschaltet ist (visuell)
    bool isProductionDisabledVirtual() const { return disableProductionVirtual; }
    /// Fragt ab, ob Produktion ausgeschaltet ist (real)
    bool isProductionDisabled() const { return disableProduction; }
    /// Called when there are no more resources
    void onOutOfResources();
    /// Fängt an NICHT zu arbeiten (wird gemessen fürs Ausrechnen der Produktivität)
    void startNotWorking();
    /// Hört auf, nicht zu arbeiten, sprich fängt an zu arbeiten (fürs Ausrechnen der Produktivität)
    void stopNotWorking();

private:
    /// Calculates the productivity and resets the counter
    unsigned short calcCurrentProductivity();
};
