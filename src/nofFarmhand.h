// $Id: nofFarmhand.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_FARMHAND_H_
#define NOF_FARMHAND_H_

#include "nofBuildingWorker.h"


/// Ein Landarbeiter geht raus aus seiner Hütte und arbeitet in "freier Natur"
class nofFarmhand : public nofBuildingWorker
{
    protected:

        /// Arbeitsziel, das der Arbeiter ansteuert
        unsigned short dest_x, dest_y;

        enum PointQuality
        {
            PQ_NOTPOSSIBLE, // Work is not possible at this position
            PQ_CLASS1, /// Work is possible, points are prefered to other points
            PQ_CLASS2, /// Work is possible, points are prefered to other points class 2
            PQ_CLASS3 /// Work is possible, points are only chosen if there are no other class 1/2's
        };

    private:

        /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn...
        void WalkedDerived();

        /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
        void WorkAborted();
        /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden (an abgeleitete Klassen)
        virtual void WorkAborted_Farmhand();


        ///// Fängt das "Warten-vor-dem-Arbeiten" an, falls er arbeiten kann (müssen ja bestimmte "Naturobjekte" gegeben sein)
        //void TryToWork();
        /// Findet heraus, ob der Beruf an diesem Punkt arbeiten kann
        bool IsPointAvailable(const unsigned short x, const unsigned short y);
        /// Returns the quality of this working point or determines if the worker can work here at all
        virtual PointQuality GetPointQuality(const unsigned short x, const unsigned short y) = 0;

        /// Läuft zum Arbeitspunkt
        void WalkToWorkpoint();
        /// Trifft Vorbereitungen fürs nach Hause - Laufen
        void StartWalkingHome();
        /// Läuft wieder zu seiner Hütte zurück
        void WalkHome();

        /// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
        virtual void WalkingStarted();
        /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
        virtual void WorkStarted() = 0;
        /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
        virtual void WorkFinished() = 0;

        /// Zeichnen der Figur in sonstigen Arbeitslagen
        virtual void DrawOtherStates(const int x, const int y);

    public:

        nofFarmhand(const Job job, const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace);
        nofFarmhand(SerializedGameData* sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_nofFarmhand() { Destroy_nofBuildingWorker(); }
    public:     void Destroy() { Destroy_nofFarmhand(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofFarmhand(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofFarmhand(sgd); }



        void HandleDerivedEvent(const unsigned int id);


};


#endif
