// $Id: nofHunter.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_HUNTER_H_
#define NOF_HUNTER_H_

#include "nofBuildingWorker.h"

class noAnimal;


/// Klasse für den Jäger, der Tiere jagt und Nahrung produziert
class nofHunter : public nofBuildingWorker
{
    private:

        /// Tier, das gejagt wird
        noAnimal* animal;
        /// Punkt, von dem aus geschossen wird
        unsigned short shooting_x, shooting_y;
        /// Richtung, in die geschossen wird
        unsigned char shooting_dir;

    private:

        /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn man gelaufen ist
        void WalkedDerived();
        /// Malt den Arbeiter beim Arbeiten
        void DrawWorking(int x, int y);
        /// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
        unsigned short GetCarryID() const { return 89; }

        /// Trifft Vorbereitungen fürs nach Hause - Laufen
        void StartWalkingHome();
        /// Läuft wieder zu seiner Hütte zurück
        void WalkHome();

        /// Prüft, ob der Schießpunkt geeignet ist
        bool IsShootingPointGood(const unsigned short x, const unsigned short y);

        /// Wenn jeweils gelaufen wurde oder ein Event abgelaufen ist, je nach aktuellem Status folgende Funktionen ausführen
        void HandleStateChasing();
        void HandleStateFindingShootingPoint();
        void HandleStateShooting();
        void HandleStateWalkingToCadaver();
        void HandleStateEviscerating();

    public:

        nofHunter(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace);
        nofHunter(SerializedGameData* sgd, const unsigned obj_id);
        ~nofHunter() {/* assert(obj_id != 266501);*/ }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofHunter(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofHunter(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_HUNTER; }

        void HandleDerivedEvent(const unsigned int id);

        /// das Tier ist nicht mehr verfügbar (von selbst gestorben o.Ä.)
        void AnimalLost();
        /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
        void WorkAborted();
};

#endif
