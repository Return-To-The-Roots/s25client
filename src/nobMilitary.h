// $Id: nobMilitary.h 9564 2014-12-30 10:53:04Z marcus $
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

#ifndef NOB_MILITARYBUILDING_H_
#define NOB_MILITARYBUILDING_H_

#include "nobBaseMilitary.h"
#include <list>

class nofPassiveSoldier;
class nofActiveSoldier;
class nofAttacker;
class nofAggressiveDefender;
class nofDefender;
class Ware;
class iwMilitaryBuilding;

/// Stellt ein Militärgebäude beliebiger Größe (also von Baracke bis Festung) dar
class nobMilitary : public nobBaseMilitary
{
        /// wurde das Gebäude gerade neu gebaut (muss also die Landgrenze beim Eintreffen von einem Soldaten neu berechnet werden?)
        bool new_built;
        /// Anzahl der Goldmünzen im Gebäude
        unsigned char coins;
        /// Gibt an, ob Goldmünzen gesperrt worden (letzteres nur visuell, um Netzwerk-Latenzen zu verstecken)
        bool disable_coins, disable_coins_virtual;
        /// Entfernung zur freindlichen Grenze (woraus sich dann die Besatzung ergibt) von 0-3, 0 fern, 3 nah, 2 Hafen!
        unsigned char frontier_distance;
        /// Größe bzw Typ des Militärgebäudes (0 = Baracke, 3 = Festung)
        unsigned char size;
        /// Bestellte Soldaten
        list<nofPassiveSoldier*> ordered_troops;
        /// Bestellter Goldmünzen
        list<Ware*> ordered_coins;
        /// Gibt an, ob gerade die Eroberer in das Gebäude gehen (und es so nicht angegegriffen werden sollte)
        bool capturing;
        /// Anzahl der Soldaten, die das Militärgebäude gerade noch einnehmen
        unsigned capturing_soldiers;
        /// List of soldiers who are on the way to capture the military building
        /// but who are still quite far away (didn't stand around the building)
        std::list<nofAttacker*> far_away_capturers;
        /// Gold-Bestell-Event
        EventManager::EventPointer goldorder_event;
        /// Beförderung-Event
        EventManager::EventPointer upgrade_event;
        /// Is the military building regulating its troops at the moment? (then block furthere RegulateTroop calls)
        bool is_regulating_troops;
        /// This building was captured by its current owner. This flag is set once and never to be changed again
        bool captured_not_built;
    public:

        /// Soldatenbesatzung
        list<nofPassiveSoldier*> troops;

        // Das Fenster braucht ja darauf Zugriff
        friend class iwMilitaryBuilding;

    private:

        /// Bestellungen (sowohl Truppen als auch Goldmünzen) zurücknehmen
        void CancelOrders();
        /// Wählt je nach Militäreinstellungen (Verteidigerstärke) einen passenden Soldaten aus
        list<nofPassiveSoldier*>::iterator ChooseSoldier();
        /// Stellt Verteidiger zur Verfügung
        nofDefender* ProvideDefender(nofAttacker* const attacker);
        /// Will/kann das Gebäude noch Münzen bekommen?
        bool WantCoins();
        /// Prüft, ob Goldmünzen und Soldaten, die befördert werden können, vorhanden sind und meldet ggf. ein
        /// Beförderungsevent an
        void PrepareUpgrading();

    public:

        nobMilitary(const BuildingType type, const unsigned short x, const unsigned short y, const unsigned char player, const Nation nation);
        nobMilitary(SerializedGameData* sgd, const unsigned obj_id);

        ~nobMilitary();

        /// Aufräummethoden
    protected:  void Destroy_nobMilitary();
    public:     void Destroy() { Destroy_nobMilitary(); }

        /// Serialisierungsfunktionen
    protected: void Serialize_nobMilitary(SerializedGameData* sgd) const;
    public: void Serialize(SerializedGameData* sgd) const { Serialize_nobMilitary(sgd); }

        GO_Type GetGOT() const { return GOT_NOB_MILITARY; }

        void Draw(int x, int y);
        void HandleEvent(const unsigned int id);

        /// Wurde das Militärgebäude neu gebaut und noch nicht besetzt und kann somit abgerissen werden bei Land-verlust?
        bool IsNewBuilt() const { return new_built; }

        /// Liefert Militärradius des Gebäudes
        MapCoord GetMilitaryRadius() const;

        /// Sucht feindliche Miitärgebäude im Umkreis und setzt die frontier_distance entsprechend (sowohl selber als
        /// auch von den feindlichen Gebäuden) und bestellt somit ggf. neue Soldaten, exception wird nicht mit einbezogen
        void LookForEnemyBuildings(const nobBaseMilitary* const exception = NULL);

        /// Wird von gegnerischem Gebäude aufgerufen, wenn sie neu gebaut worden sind und es so ein neues Gebäude im Umkreis gibt
        /// setzt frontier_distance neu falls möglich und sendet ggf. Verstärkung
        void NewEnemyMilitaryBuilding(const unsigned short distance);
        bool IsUseless() const;
        /// Gibt Distanz zurück
        unsigned char GetFrontierDistance() const { return frontier_distance; }

        /// Berechnet die gewünschte Besatzung je nach Grenznähe
        int CalcTroopsCount();
        /// Reguliert die Besatzung des Gebäudes je nach Grenznähe, bestellt neue Soldaten und schickt überflüssige raus
        void RegulateTroops();
        /// Gibt aktuelle Besetzung zurück
        unsigned GetTroopsCount() const { return troops.size(); }


        /// Wird aufgerufen, wenn eine neue Ware zum dem Gebäude geliefert wird (in dem Fall nur Goldstücke)
        void TakeWare(Ware* ware);
        /// Legt eine Ware am Objekt ab (an allen Straßenknoten (Gebäude, Baustellen und Flaggen) kann man Waren ablegen
        void AddWare(Ware* ware);
        /// Eine bestellte Ware konnte doch nicht kommen
        void WareLost(Ware* ware);
        /// Wird aufgerufen, wenn von der Fahne vor dem Gebäude ein Rohstoff aufgenommen wurde
        bool FreePlaceAtFlag();

        /// Berechnet, wie dringend eine Goldmünze gebraucht wird, in Punkten, je höher desto dringender
        unsigned CalcCoinsPoints();

        /// Wird aufgerufen, wenn ein Soldat kommt
        void GotWorker(Job job, noFigure* worker);
        /// Fügt aktiven Soldaten (der aus von einer Mission) zum Militärgebäude hinzu
        void AddActiveSoldier(nofActiveSoldier* soldier);
        /// Fügt passiven Soldaten (der aus einem Lagerhaus kommt) zum Militärgebäude hinzu
        void AddPassiveSoldier(nofPassiveSoldier* soldier);
        /// Soldat konnte nicht kommen
        void SoldierLost(nofSoldier* soldier);
        /// Soldat ist jetzt auf Mission
        void SoldierOnMission(nofPassiveSoldier* passive_soldier, nofActiveSoldier* active_soldier);

        /// Schickt einen Verteidiger raus, der einem Angreifer in den Weg rennt
        nofAggressiveDefender* SendDefender(nofAttacker* attacker);

        /// Gibt die Anzahl der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
        unsigned GetSoldiersForAttack(const MapCoord dest_x, const MapCoord dest_y, const unsigned char player_attacker) const;
        /// Gibt die Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
        void GetSoldiersForAttack(const MapCoord dest_x, const MapCoord dest_y,
                                  const unsigned char player_attacker, std::vector<nofPassiveSoldier*> * soldiers) const;
        /// Gibt die Stärke der Soldaten zurück, die für einen Angriff auf ein bestimmtes Ziel zur Verfügung stehen
        unsigned GetSoldiersStrengthForAttack(const MapCoord dest_x, const MapCoord dest_y,
                                              const unsigned char player_attacker, unsigned& soldiers_count) const;
        /// Gibt die Stärke eines Militärgebäudes zurück
        unsigned GetSoldiersStrength() const;

        /// Gebäude wird vom Gegner eingenommen, player ist die neue Spieler-ID
        void Capture(const unsigned char new_owner);
        /// Das Gebäude wurde bereits eingenommen, hier wird geprüft, ob noch weitere Soldaten für die Besetzung
        /// notwendig sind, wenn ja wird ein neuer Soldat gerufen, wenn nein, werden alle restlichen nach Hause
        /// geschickt
        void NeedOccupyingTroops(const unsigned char new_owner);
        /// Sagt dem Gebäude schonmal, dass es eingenommen wird, wenn er erste Eroberer gerade in das Gebäude reinläuft
        /// (also noch bevor er drinnen ist!) - damit da nicht zusätzliche Soldaten reinlaufen
        void PrepareCapturing() { capturing = true; ++capturing_soldiers; }

        /// Wird das Gebäude gerade eingenommen?
        bool IsCaptured() const { return capturing; }
        /// Gebäude wird nicht mehr eingenommen (falls anderer Soldat zuvor reingekommen ist beim Einnehmen)
        void StopCapturing() { capturing = false; }
        /// Sagt, dass ein erobernder Soldat das Militärgebäude erreicht hat
        void CapturingSoldierArrived() { --capturing_soldiers; }
        /// A far-away capturer arrived at the building/flag and starts the capturing
        void FarAwayAttackerReachedGoal(nofAttacker* attacker);

        /// Stoppt/Erlaubt Goldzufuhr (visuell)
        void StopGoldVirtual() { disable_coins_virtual = !disable_coins_virtual; }
        /// Stoppt/Erlaubt Goldzufuhr (real)
        void StopGold();
        /// Fragt ab, ob Goldzufuhr ausgeschaltet ist (visuell)
        bool IsGoldDisabledVirtual() const { return disable_coins_virtual; }
        /// Fragt ab, ob Goldzufuhr ausgeschaltet ist (real)
        bool IsGoldDisabled() const { return disable_coins; }
		/// is there a max rank soldier in the building?
		unsigned HasMaxRankSoldier() const;

        /// Sucht sämtliche Lagerhäuser nach Goldmünzen ab und bestellt ggf. eine, falls eine gebraucht wird
        void SearchCoins();

        /// Gebäude wird von einem Katapultstein getroffen
        void HitOfCatapultStone();

        /// Sind noch Truppen drinne, die dieses Gebäude verteidigen können
        bool DefendersAvailable() const { return (GetTroopsCount() > 0); }

		/// send all soldiers of the highest rank home (if highest=lowest keep 1)
		void SendSoldiersHome();
		/// order new troops
		void OrderNewSoldiers();

        /// Darf das Militärgebäude abgerissen werden (Abriss-Verbot berücksichtigen)?
        bool IsDemolitionAllowed() const;

        bool WasCapturedOnce() const {return(captured_not_built);}

        virtual void UnlinkAggressor(nofAttacker* soldier);
};


#endif
