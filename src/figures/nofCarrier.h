// $Id: nofCarrier.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_CARRIER_H_
#define NOF_CARRIER_H_

#include "figures/noFigure.h"
#include "GlobalVars.h"

class RoadSegment;
class Ware;
class noRoadNode;
class glSmartBitmap;

enum CarrierState
{
    CARRS_FIGUREWORK = 0, // Aufgaben der Figur
    CARRS_WAITFORWARE, // auf Weg auf Ware warten
    CARRS_GOTOMIDDLEOFROAD, // zur Mitte seines Weges gehen
    CARRS_FETCHWARE, // Ware holen
    CARRS_CARRYWARE, // Ware zur Flagge tragen
    CARRS_CARRYWARETOBUILDING, // Ware zum GebÃ¤ude schaffen
    CARRS_LEAVEBUILDING, // kommt aus GebÃ¤ude wieder raus (bzw kommt von Baustelle zurÃ¼ck) zum Weg
    CARRS_WAITFORWARESPACE, // wartet vor der Flagge auf einen freien Platz
    CARRS_GOBACKFROMFLAG, // geht von der Flagge zurÃ¼ck, weil kein Platz mehr frei war
    CARRS_BOATCARRIER_WANDERONWATER // Rumirren der BootstrÃ¤ger auf dem Wasser, d.h. Paddeln zum
    // nÃ¤chsten Ufer, nachdem der Wasserweg zerstÃ¶rt wurde
};

// Stellt einen TrÃ¤cer da
class nofCarrier : public noFigure
{
    public:

        /// TrÃ¤ger-"Typ"
        enum CarrierType
        {
            CT_NORMAL, // Normaler TrÃ¤ger
            CT_DONKEY, // Esel
            CT_BOAT // TrÃ¤ger mit Boot
        };

    private:

        CarrierType ct;
        /// Was der TrÃ¤ger gerade so treibt
        CarrierState state;
        /// Ist er dick?
        bool fat;
        // Weg, auf dem er arbeitet
        RoadSegment* workplace;
        /// Ware, die er gerade trÃ¤gt (0 = nichts)
        Ware* carried_ware;
        /// Rechne-ProduktivitÃ¤t-aus-Event
        EventManager::EventPointer productivity_ev;
        // Letzte errechnete ProduktivitÃ¤t
        unsigned productivity;
        /// Wieviel GF von einer bestimmten Anzahl in diesem Event-Zeitraum gearbeitet wurde
        unsigned worked_gf;
        /// Zu welchem GF das letzte Mal das Arbeiten angefangen wurde
        unsigned since_working_gf;
        /// Bestimmt GF der nÃ¤chsten TrÃ¤geranimation
        unsigned next_animation;
        /// For boat carriers: path to the shore
        std::vector<unsigned char> * shore_path;

    private:

        void GoalReached();
        void Walked();
        void AbrogateWorkplace();
        void HandleDerivedEvent(const unsigned int id);

        /// Nach dem Tragen der Ware, guckt der TrÃ¤ger an beiden Flagge, obs Waren gibt, holt/trÃ¤gt diese ggf oder geht ansonsten wieder in die Mitte
        void LookForWares();
        /// Nimmt eine Ware auf an der aktuellen Flagge und dreht sich um, um sie zu tragen (fetch_dir ist die Richtung der Waren, die der TrÃ¤ger aufnehmen will)
        void FetchWare(const bool swap_wares);

        /// PrÃ¼ft, ob die getragene Ware dann von dem Weg zum GebÃ¤ude will
        bool WantInBuilding(bool* calculated);

        /// FÃ¼r ProduktivitÃ¤tsmessungen: fÃ¤ngt an zu arbeiten
        void StartWorking();
        /// FÃ¼r ProduktivitÃ¤tsmessungen: hÃ¶rt auf zu arbeiten
        void StopWorking();

        /// Bestimmt neuen Animationszeitpunkt
        void SetNewAnimationMoment();

        /// Boat carrier paddles to the coast after his road was destroyed
        void WanderOnWater();

    public:

        nofCarrier(const CarrierType ct, const MapPoint pt, const unsigned char player, RoadSegment* workplace, noRoadNode* const goal);
        nofCarrier(SerializedGameData* sgd, const unsigned obj_id);

        ~nofCarrier();

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofCarrier(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofCarrier(sgd); }

        /// AufrÃ¤ummethoden
    protected:  void Destroy_nofCarrier();
    public:     void Destroy() { Destroy_nofCarrier(); }

        GO_Type GetGOT() const { return GOT_NOF_CARRIER; }

        /// Gibt TrÃ¤ger-Typ zurÃ¼ck
        CarrierType GetCarrierType() const { return ct; }
        /// Was macht der TrÃ¤ger gerade?
        CarrierState GetCarrierState() const { return state; }
        /// Gibt TrÃ¤ger-ProduktivitÃ¤t in % zurÃ¼ck
        unsigned GetProductivity() const { return productivity; }

        void Draw(int x, int y);

        /// Wird aufgerufen, wenn der Weg des TrÃ¤gers abgerissen wurde
        void LostWork();

        /// Wird aufgerufen, wenn der Arbeitsplatz des TrÃ¤gers durch eine Flagge geteilt wurde
        /// der TrÃ¤ger sucht sich damit einen der beiden als neuen Arbeitsplatz, geht zur Mitte und ruft einen neuen TrÃ¤ger
        /// fÃ¼r das 2. WegstÃ¼ck
        void RoadSplitted(RoadSegment* rs1, RoadSegment* rs2);

        /// Sagt dem TrÃ¤ger Bescheid, dass es an einer Flagge noch eine Ware zu transportieren gibt
        bool AddWareJob(const noRoadNode* rn);
        /// Das Gegnteil von AddWareJob: wenn eine Ware nicht mehr transportiert werden will, sagt sie dem TrÃ¤ger Bescheid,
        /// damit er nicht unnÃ¶tig dort hinlÃ¤uft zur Flagge
        void RemoveWareJob();

        /// Benachrichtigt den TrÃ¤ger, wenn an einer auf seinem Weg an einer Flagge wieder ein freier Platz ist
        /// gibt zurÃ¼ck, ob der TrÃ¤ger diesen freien Platz auch nutzen wird
        bool SpaceAtFlag(const bool flag);

        /// Gibt erste Flagge des Arbeitsweges zurÃ¼ck, falls solch einer existiert
        noRoadNode* GetFirstFlag() const;
        noRoadNode* GetSecondFlag() const;

        /// Wird aufgerufen, wenn die StraÃŸe unter der Figur geteilt wurde (fÃ¼r abgeleitete Klassen)
        void CorrectSplitData_Derived();
};

#endif
