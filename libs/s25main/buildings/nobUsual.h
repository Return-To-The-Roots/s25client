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

    /// is this an empty cycle? (use wares but produce nothing)
    bool is_emptyCycle;

    ~nobUsual() override;

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GO_Type::NobUsual; }
    unsigned GetMilitaryRadius() const override { return 0; }

    void Draw(DrawPoint drawPt) override;

    bool HasWorker() const;

    /// Event-Handler
    void HandleEvent(unsigned id) override;
    /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
    void AddWare(std::unique_ptr<Ware> ware) override;
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool FreePlaceAtFlag() override;
    /// Eine bestellte Ware konnte doch nicht kommen
    void WareLost(Ware& ware) override;
    /// Wird aufgerufen, wenn ein Arbeiter für das Gebäude gefunden werden konnte
    void GotWorker(Job job, noFigure& worker) override;
    /// Wird vom Arbeiter aufgerufen, wenn er im Gebäude angekommen ist
    void WorkerArrived();
    /// Wird vom Arbeiter aufgerufen, wenn er nicht (mehr) zur Arbeit kommen kann
    void WorkerLost();

    /// Gibt den Warenbestand (eingehende Waren - Rohstoffe) zurück
    unsigned char GetNumWares(unsigned id) const { return numWares[id]; }
    /// Prüft, ob Waren für einen Arbeitsschritt vorhanden sind
    bool WaresAvailable();
    /// Verbraucht Waren
    void ConsumeWares();

    /// Berechnet Punktewertung für Ware type
    unsigned CalcDistributionPoints(GoodType type);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void TakeWare(Ware* ware) override;

    /// Bestellte Waren
    bool AreThereAnyOrderedWares() const;

    /// Gibt Pointer auf Produktivität zurück
    const unsigned short* GetProductivityPointer() const { return &productivity; }
    unsigned short GetProductivity() const { return productivity; }
    const nofBuildingWorker* GetWorker() const { return worker; }

    /// Stoppt/Erlaubt Produktion (visuell)
    void ToggleProductionVirtual() { disableProductionVirtual = !disableProductionVirtual; }
    /// Stoppt/Erlaubt Produktion (real)
    void SetProductionEnabled(bool enabled);
    /// Fragt ab, ob Produktion ausgeschaltet ist (visuell)
    bool IsProductionDisabledVirtual() const { return disableProductionVirtual; }
    /// Fragt ab, ob Produktion ausgeschaltet ist (real)
    bool IsProductionDisabled() const { return disableProduction; }
    /// Called when there are no more resources
    void OnOutOfResources();
    /// Fängt an NICHT zu arbeiten (wird gemessen fürs Ausrechnen der Produktivität)
    void StartNotWorking();
    /// Hört auf, nicht zu arbeiten, sprich fängt an zu arbeiten (fürs Ausrechnen der Produktivität)
    void StopNotWorking();

private:
    /// Calculates the productivity and resets the counter
    unsigned short CalcCurrentProductivity();
};
