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

#ifndef NOB_BASEMILITARY_H_
#define NOB_BASEMILITARY_H_

#include "buildings/noBuilding.h"
#include <boost/container/flat_set.hpp>
#include <list>

class nofSoldier;
class nofActiveSoldier;
class nofAttacker;
class nofAggressiveDefender;
class nofDefender;
class SerializedGameData;
class noFigure;
class GameEvent;

/// allgemeine Basisklasse für alle Militärgebäude (HQ,normale Militärgebäude, Häfen,einschließlich Lagerhäuser,
/// weil die auch viele Merkmale davon haben, aber sind eigentlich keine Militärgebäude)
class nobBaseMilitary : public noBuilding
{
protected:
    /// Liste von Figuren, die das Gebäude verlassen wollen (damit nicht alle auf einmal rauskommen)
    std::list<noFigure*> leave_house;
    /// Event, damit nicht alle auf einmal rauskommen
    const GameEvent* leaving_event;
    /// Geht gerade jemand raus? (damit nicht alle auf einmal rauskommen), für Lager- und Militärhäuser
    bool go_out;
    /// Truppen, die zwar gerade nicht im Haus sind, aber eigentlich dazu gehören und grade auf Mission sind, wo sie evtl
    /// wieder zurückkkehren könnten (Angriff, Verteidigung etc.)
    std::list<nofActiveSoldier*> troops_on_mission;
    /// Liste von Soldaten, die dieses Gebäude angreifen
    std::list<nofAttacker*> aggressors;
    /// Liste von aggressiven Verteidigern, die dieses Gebäude mit verteidigen
    std::list<nofAggressiveDefender*> aggressive_defenders;
    /// Soldat, der grad dieses Gebäude verteidigt
    nofDefender* defender_;

public:
    nobBaseMilitary(const BuildingType type, const MapPoint pt, unsigned char player, const Nation nation);
    nobBaseMilitary(SerializedGameData& sgd, unsigned obj_id);
    ~nobBaseMilitary() override;

protected:
    void DestroyBuilding() override;
    void Serialize_nobBaseMilitary(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nobBaseMilitary(sgd); }

    const std::list<noFigure*>& GetLeavingFigures() const { return leave_house; }

    /// Gibt Verteidiger zurück
    const nofDefender* GetDefender() const { return defender_; }

    /// Compares according to build time (Age): Bigger objIds are "younger"
    bool operator<(const nobBaseMilitary& other) const { return GetObjId() > other.GetObjId(); }

    /// Meldet ein neues "Rausgeh"-Event an, falls gerade keiner rausgeht
    /// (damit nicht alle auf einmal rauskommen), für Lager- und Militärhäuser)
    void AddLeavingEvent();

    /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
    virtual void AddActiveSoldier(nofActiveSoldier* soldier) = 0;

    /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
    virtual nofAggressiveDefender* SendAggressiveDefender(nofAttacker* attacker) = 0;

    /// Soldaten zur Angreifer-Liste hinzufügen und wieder entfernen
    void LinkAggressor(nofAttacker* soldier) { aggressors.push_back(soldier); }
    virtual void UnlinkAggressor(nofAttacker* soldier)
    {
        RTTR_Assert(IsAggressor(soldier));
        aggressors.remove(soldier);
    }

    /// Soldaten zur Aggressiven-Verteidiger-Liste hinzufügen und wieder entfernen
    void LinkAggressiveDefender(nofAggressiveDefender* soldier) { aggressive_defenders.push_back(soldier); }
    void UnlinkAggressiveDefender(nofAggressiveDefender* soldier)
    {
        RTTR_Assert(IsAggressiveDefender(soldier));
        aggressive_defenders.remove(soldier);
    }

    /// Wird aufgerufen, wenn ein Soldat nicht mehr kommen kann
    virtual void SoldierLost(nofSoldier* soldier) = 0;

    /// Sucht einen Angreifer auf das Gebäude, der gerade nichts zu tun hat und Lust hat zum Kämpfen
    /// und in der Nähe von x,y ist (wird von aggressiven Verteidigern aufgerufen), wenn keiner gefunden wird, wird 0
    /// zurückgegeben
    nofAttacker* FindAggressor(nofAggressiveDefender* defender);
    /// Sucht für einen Angreifer den nächsten (bzw. genau den) Platz zur Fahne, damit die sich darum postieren und
    /// warten
    MapPoint FindAnAttackerPlace(unsigned short& ret_radius, nofAttacker* soldier);
    /// Sucht einen Nachrücker, der weiter hinten steht, auf diesen Posten und schickt diesen auch los
    bool SendSuccessor(const MapPoint pt, unsigned short radius);

    /// Gibt zurück, ob es noch einenen Verteidiger in dieser Hütte gibt, wenn ja wird dieser losgeschickt,
    /// aggressor ist der Angreifer an der Fahne, mit dem er kämpfen soll
    bool CallDefender(nofAttacker* attacker);
    /// Sucht einen Angreifer auf dieses Gebäude und schickt ihn ggf. zur Flagge zum Kämpfen
    nofAttacker* FindAttackerNearBuilding();
    /// Wird aufgerufen nach einem Kampf an einer Flagge, der evtl. die anderen Soldaten gehindert hat, zur Flagge
    /// zu kommen, diese Funktion sucht nach solchen Soldaten schickt einen ggf. zur Flagge, um anzugreifen
    void CheckArrestedAttackers();
    /// Der Verteidiger ist entweder tot oder wieder reingegegangen
    void NoDefender() { defender_ = nullptr; }
    /// Bricht einen aktuell von diesem Haus gestarteten Angriff/aggressive Verteidigung ab, d.h. setzt die Soldaten
    /// aus der Warteschleife wieder in das Haus --> wenn Angreifer an der Fahne ist und Verteidiger rauskommen soll
    void CancelJobs();

    /// Sind noch Truppen drinne, die dieses Gebäude verteidigen können
    virtual bool DefendersAvailable() const = 0;

    bool IsUnderAttack() const { return !aggressors.empty(); };
    /// Return whether this building can be attacked by the given player.
    virtual bool IsAttackable(unsigned playerIdx) const;

    /// Debugging
    bool IsAggressor(nofAttacker* attacker) const;
    bool IsAggressiveDefender(nofAggressiveDefender* soldier) const;
    bool IsOnMission(nofActiveSoldier* soldier) const;
    const std::list<nofAggressiveDefender*>& GetAggresiveDefenders() const { return aggressive_defenders; }

    // Vergleicht Gebäude anhand ihrer Bauzeit, um eine geordnete Reihenfolge hinzubekommen
    struct Comparer
    {
        bool operator()(const nobBaseMilitary* const one, const nobBaseMilitary* const two) const { return (*one) < (*two); }
    };

protected:
    /// The building shall provide a soldier for defense. Return nullptr if none available
    virtual nofDefender* ProvideDefender(nofAttacker* const attacker) = 0;
    /// Add a figure that will leave the house
    void AddLeavingFigure(noFigure* fig);
};

class sortedMilitaryBlds : public boost::container::flat_set<nobBaseMilitary*, nobBaseMilitary::Comparer>
{};

#endif //! NOB_BASEMILITARY_H_
