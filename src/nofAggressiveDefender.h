// $Id: nofAggressiveDefender.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NOF_AGGRESSIVEDEFENDER_H_
#define NOF_AGGRESSIVEDEFENDER_H_

#include "nofActiveSoldier.h"

class nofAttacker;
class nofPassiveSoldier;

/// Aggressiv-verteidigender Soldat (jemand, der den Angreifer auf offenem Feld entgegenläuft)
class nofAggressiveDefender : public nofActiveSoldier
{
        // Unser Feind-Freund ;)
        friend class nofAttacker;

    private:

        /// Soldaten, der er entgegenrennen soll
        nofAttacker* attacker;
        /// Militärgebäude, das angegriffen wird
        nobBaseMilitary* attacked_goal;

    private:

        /// wenn man gelaufen ist
        void Walked();
        /// Geht nach Haus für MAggressiveDefending-Mission
        void ReturnHomeMissionAggressiveDefending();
        /// Läuft wieter
        void MissAggressiveDefendingWalk();
        /// Sucht sich für MissionAggressiveAttacking ein neues Ziel, wenns keins findet, gehts nach Hause
        void MissionAggressiveDefendingLookForNewAggressor();
        /// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
        void InformTargetsAboutCancelling();

        /// The derived classes regain control after a fight of nofActiveSoldier
        void FreeFightEnded();

    public:

        nofAggressiveDefender(const unsigned short x, const unsigned short y, const unsigned char player,
                              nobBaseMilitary* const home, const unsigned char rank, nofAttacker* const attacker);
        nofAggressiveDefender(nofPassiveSoldier* other, nofAttacker* const attacker);
        nofAggressiveDefender(SerializedGameData* sgd, const unsigned obj_id);

        ~nofAggressiveDefender();



        /// Aufräummethoden
    protected:  void Destroy_nofAggressiveDefender();
    public:     void Destroy() { Destroy_nofAggressiveDefender(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofAggressiveDefender(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofAggressiveDefender(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_AGGRESSIVEDEFENDER; }

        /// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
        void HomeDestroyed();
        /// Wenn er noch in der Warteschleife vom Ausgangsgebäude hängt und dieses zerstört wurde
        void HomeDestroyedAtBegin();
        /// Wenn ein Kampf gewonnen wurde
        void WonFighting();
        /// Wenn ein Kampf verloren wurde (Tod)
        void LostFighting();

        /// Gebäude, das vom aggressiv-verteidigenden Soldaten verteidigt werden sollte, wurde zerstört
        void AttackedGoalDestroyed();
        /// Soldat, der angehalten ist, um auf seinen Angreifer-Kollegen zu warten, soll jetzt weiterlaufen, da er um
        /// das Angriffsgebäude schon wartet
        void MissAggressiveDefendingContinueWalking();
        /// Wenn der jeweils andere Soldat, mit dem man kämpfen wollte, nicht mehr kommen kann
        void AttackerLost();
        /// Ich befinde mich noch im Lagerhaus in der Warteschlange und muss mein HQ etc. verteidigen
        /// Mission muss also abgebrochen werden
        void NeedForHomeDefence();

};


#endif // !NOF_AGGRESSIVEDEFENDER_H_
