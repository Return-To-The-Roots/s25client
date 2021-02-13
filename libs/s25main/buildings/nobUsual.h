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
    /// Der Typ, der hier arbeitet
    nofBuildingWorker* worker;
    /// Produktivität
    unsigned short productivity;
    /// Produktion eingestellt? (letzteres nur visuell, um Netzwerk-Latenzen zu verstecken)
    bool disable_production, disable_production_virtual;
    /// Warentyp, den er zuletzt bestellt hatte (bei >1 Waren)
    unsigned char last_ordered_ware;
    /// Rohstoffe, die zur Produktion benötigt werden
    std::array<uint8_t, 3> numWares;
    /// Bestellte Waren
    std::vector<std::list<Ware*>> ordered_wares;
    /// Bestell-Ware-Event
    const GameEvent* orderware_ev;
    /// Rechne-Produktivität-aus-Event
    const GameEvent* productivity_ev;
    /// Letzte Produktivitäten (Durchschnitt = Gesamt produktivität), vorne das neuste !
    std::array<uint16_t, 6> last_productivities;
    /// How many GFs he did not work since the last productivity calculation
    unsigned short numGfNotWorking;
    /// Since which GF he did not work (0xFFFFFFFF = currently working)
    unsigned since_not_working;
    /// Did we notify the player that we are out of resources?
    bool outOfRessourcesMsgSent;

protected:
    friend class SerializedGameData;
    friend class BuildingFactory;
    nobUsual(BuildingType type, MapPoint pos, unsigned char player, Nation nation);
    nobUsual(SerializedGameData& sgd, unsigned obj_id);

public:
    /// Wird gerade gearbeitet oder nicht?
    bool is_working;

    ~nobUsual() override;

protected:
    void DestroyBuilding() override;

public:
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GO_Type::NobUsual; }
    unsigned GetMilitaryRadius() const override { return 0; }

    void Draw(DrawPoint drawPt) override;

    bool HasWorker() const;

    /// Event-Handler
    void HandleEvent(unsigned id) override;
    /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
    void AddWare(Ware*& ware) override;
    /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
    bool FreePlaceAtFlag() override;
    /// Eine bestellte Ware konnte doch nicht kommen
    void WareLost(Ware* ware) override;
    /// Wird aufgerufen, wenn ein Arbeiter für das Gebäude gefunden werden konnte
    void GotWorker(Job job, noFigure* worker) override;
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

    /// Berechnet Punktewertung für Ware type, start ist der Produzent, von dem die Ware kommt
    unsigned CalcDistributionPoints(noRoadNode* start, GoodType type);

    /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (nicht wenn sie bestellt wurde vom Gebäude!)
    void TakeWare(Ware* ware) override;

    /// Bestellte Waren
    bool AreThereAnyOrderedWares() const
    {
        for(const auto& ordered_ware : ordered_wares)
            if(!ordered_ware.empty())
                return true;
        return false;
    }

    /// Gibt Pointer auf Produktivität zurück
    const unsigned short* GetProductivityPointer() const { return &productivity; }
    unsigned short GetProductivity() const { return productivity; }
    const nofBuildingWorker* GetWorker() const { return worker; }

    /// Stoppt/Erlaubt Produktion (visuell)
    void ToggleProductionVirtual() { disable_production_virtual = !disable_production_virtual; }
    /// Stoppt/Erlaubt Produktion (real)
    void SetProductionEnabled(bool enabled);
    /// Fragt ab, ob Produktion ausgeschaltet ist (visuell)
    bool IsProductionDisabledVirtual() const { return disable_production_virtual; }
    /// Fragt ab, ob Produktion ausgeschaltet ist (real)
    bool IsProductionDisabled() const { return disable_production; }
    /// Called when there are no more resources
    void OnOutOfResources();
    /// Fängt an NICHT zu arbeiten (wird gemessen fürs Ausrechnen der Produktivität)
    void StartNotWorking();
    /// Hört auf, nicht zu arbeiten, sprich fängt an zu arbeiten (fürs Ausrechnen der Produktivität)
    void StopNotWorking();

private:
    /// Calculates the productivity and resets the counter
    unsigned short CalcProductivity();
};
