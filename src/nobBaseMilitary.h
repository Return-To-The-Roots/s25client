// $Id: nobBaseMilitary.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOB_BASEMILITARY_H_
#define NOB_BASEMILITARY_H_

#include "noBuilding.h"
#include "BuildingConsts.h"
#include "GameConsts.h"
#include "EventManager.h"

class nofSoldier;
class nofActiveSoldier;
class nofAttacker;
class nofAggressiveDefender;
class nofDefender;


/// allgemeine Basisklasse für alle Militärgebäude (HQ,normale Militärgebäude, Häfen,einschließlich Lagerhäuser,
/// weil die auch viele Merkmale davon haben, aber sind eigentlich keine Militärgebäude)
class nobBaseMilitary : public noBuilding
{
    protected:

        /// Liste von Figuren, die das Gebäude verlassen wollen (damit nicht alle auf einmal rauskommen)
        std::list<noFigure*> leave_house;
        /// Event, damit nicht alle auf einmal rauskommen
        EventManager::EventPointer leaving_event;
        /// Geht gerade jemand raus? (damit nicht alle auf einmal rauskommen), für Lager- und Militärhäuser
        bool go_out;
        /// Gibt "Alter" des Gebäudes an, je größer, desto älter ist es! (als Vergleich was zuerst gebaut wurde bei
        /// Militärgebäuden + HQs und Häfen
        unsigned age;
        /// Truppen, die zwar gerade nicht im Haus sind, aber eigentlich dazu gehören und grade auf Mission sind, wo sie evtl
        /// wieder zurückkkehren könnten (Angriff, Verteidigung etc.)
        list<nofActiveSoldier*> troops_on_mission;
        /// Liste von Soldaten, die dieses Gebäude angreifen
        list<nofAttacker*> aggressors;
        /// Liste von aggressiven Verteidigern, die dieses Gebäude mit verteidigen
        list<nofAggressiveDefender*> aggressive_defenders;
        /// Soldat, der grad dieses Gebäude verteidigt
        nofDefender* defender;

    private:

        /// die abgeleiteten Klassen sollen einen Soldaten als Verteidiger zur Verfügung stellen, wenn sie keinen haben
        /// wird 0 zurückgegeben!
        virtual nofDefender* ProvideDefender(nofAttacker* const attacker) = 0;

    public:

        nobBaseMilitary(const BuildingType type, const unsigned short x, const unsigned short y, const unsigned char player, const Nation nation);
        nobBaseMilitary(SerializedGameData* sgd, const unsigned obj_id);
        virtual ~nobBaseMilitary();

        /// Aufräummethoden
    protected:  void Destroy_nobBaseMilitary();
    public:     void Destroy() { Destroy_nobBaseMilitary(); }

        /// Serialisierungsfunktionen
    protected: void Serialize_nobBaseMilitary(SerializedGameData* sgd) const;
    public: void Serialize(SerializedGameData* sgd) const { Serialize_nobBaseMilitary(sgd); }


        /// Figur hinzufügen, die rausgehen will (damit nicht alle auf einmal rauskommen), für Lager- und Militärhäuser)
        void AddLeavingFigure(noFigure* fig);

        /// Liefert Militärradius des Gebäudes (falls es ein Militärgebäude ist)
        virtual MapCoord GetMilitaryRadius() const { return 0; }

        /// Gibt Verteidiger zurück
        nofDefender* GetDefender() const { return defender; }

        /// Das Alter wird immer verglichen
        /// absteigend sortieren, da jünger <=> age größer
        bool operator < (const nobBaseMilitary& other) const { return obj_id > other.GetObjId(); }

        /// Meldet ein neues "Rausgeh"-Event an, falls gerade keiner rausgeht
        /// (damit nicht alle auf einmal rauskommen), für Lager- und Militärhäuser)
        void AddLeavingEvent();

        /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
        virtual void AddActiveSoldier(nofActiveSoldier* soldier) = 0;

        /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
        virtual nofAggressiveDefender* SendDefender(nofAttacker* attacker) = 0;

        /// Soldaten zur Angreifer-Liste hinzufügen und wieder entfernen
        void LinkAggressor(nofAttacker* soldier) { aggressors.push_back(soldier); }
        virtual void UnlinkAggressor(nofAttacker* soldier) { aggressors.erase(soldier); }

        /// Soldaten zur Aggressiven-Verteidiger-Liste hinzufügen und wieder entfernen
        void LinkAggressiveDefender(nofAggressiveDefender* soldier) { aggressive_defenders.push_back(soldier); }
        void UnlinkAggressiveDefender(nofAggressiveDefender* soldier) { aggressive_defenders.erase(soldier); }


        /// Wird aufgerufen, wenn ein Soldat nicht mehr kommen kann
        virtual void SoldierLost(nofSoldier* soldier) = 0;

        /// Sucht einen Angreifer auf das Gebäude, der gerade nichts zu tun hat und Lust hat zum Kämpfen
        /// und in der Nähe von x,y ist (wird von aggressiven Verteidigern aufgerufen), wenn keiner gefunden wird, wird 0
        /// zurückgegeben
        nofAttacker* FindAggressor(nofAggressiveDefender* defender);
        /// Sucht für einen Angreifer den nächsten (bzw. genau den) Platz zur Fahne, damit die sich darum postieren und
        /// warten
        void FindAnAttackerPlace(unsigned short& ret_x, unsigned short& ret_y, unsigned short& ret_radius, nofAttacker* soldier);
        /// Sucht einen Nachrücker, der weiter hinten steht, auf diesen Posten und schickt diesen auch los
        bool SendSuccessor(const unsigned short x, const unsigned short y, const unsigned short radius, const unsigned char dir);

        /// Gibt zurück, ob es noch einenen Verteidiger in dieser Hütte gibt, wenn ja wird dieser losgeschickt,
        /// aggressor ist der Angreifer an der Fahne, mit dem er kämpfen soll
        bool CallDefender(nofAttacker* attacker);
        /// Sucht einen Angreifer auf dieses Gebäude und schickt ihn ggf. zur Flagge zum Kämpfen
        nofAttacker* FindAttackerNearBuilding();
        /// Wird aufgerufen nach einem Kampf an einer Flagge, der evtl. die anderen Soldaten gehindert hat, zur Flagge
        /// zu kommen, diese Funktion sucht nach solchen Soldaten schickt einen ggf. zur Flagge, um anzugreifen
        void CheckArrestedAttackers();
        /// Der Verteidiger ist entweder tot oder wieder reingegegangen
        void NoDefender() { defender = 0; }
        /// Bricht einen aktuell von diesem Haus gestarteten Angriff/aggressive Verteidigung ab, d.h. setzt die Soldaten
        /// aus der Warteschleife wieder in das Haus --> wenn Angreifer an der Fahne ist und Verteidiger rauskommen soll
        void CancelJobs();

        /// Sind noch Truppen drinne, die dieses Gebäude verteidigen können
        virtual bool DefendersAvailable() const = 0;

        bool IsUnderAttack() const {return(aggressors.size() > 0);};

        /// Debugging
        bool Test(nofAttacker* attacker);
        bool TestOnMission(nofActiveSoldier* soldier);

        // Vergleicht Gebäude anhand ihrer Bauzeit, um eine geordnete Reihenfolge hinzubekommen
        static bool Compare(const nobBaseMilitary* const one, const nobBaseMilitary* const two)
        { return (*one) < (*two); }
};




#endif //! NOB_BASEMILITARY_H_
