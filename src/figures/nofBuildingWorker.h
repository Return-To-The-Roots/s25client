// $Id: nofBuildingWorker.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_BUILDING_WORKER_H_
#define NOF_BUILDING_WORKER_H_

#include "figures/noFigure.h"
#include "gameData/JobConsts.h"

class nobUsual;

/// ReprÃ¤sentiert einen Arbeiter in einem GebÃ¤ude
class nofBuildingWorker : public noFigure
{
    public:

        /// Was der gerade so schÃ¶nes macht
        enum State
        {
            STATE_FIGUREWORK = 0, /// Arbeiten der noFigure (Laufen zum Arbeitsplatz, Rumirren usw)
            STATE_ENTERBUILDING, /// Betreten des GebÃ¤udes
            STATE_WAITING1, /// Warten, bis man anfÃ¤ngt zu produzieren
            STATE_WAITING2, /// Warten nach dem Produzieren, bis man Ware raustrÃ¤gt (nur Handwerker)
            STATE_CARRYOUTWARE, /// Raustragen der Ware
            STATE_WORK, /// Arbeiten
            STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED, /// Warten auf Waren oder weil Produktion eingetellt wurde
            STATE_WALKTOWORKPOINT, /// Zum "Arbeitspunkt" laufen (nur Landarbeiter)
            STATE_WALKINGHOME, /// vom Arbeitspunkt zurÃ¼ck nach Hause laufen (nur Landarbeiter)
            STATE_WAITFORWARESPACE, /// auf einen freien Platz an der Flagge vor dem GebÃ¤ude warten
            STATE_HUNTER_CHASING, /// JÃ¤ger: verfolgt das Tier bis auf eine gewisse Distanz
            STATE_HUNTER_FINDINGSHOOTINGPOINT, /// JÃ¤ger: sucht einen Punkt rund um das Tier, von dem er es abschieÃŸen kann
            STATE_HUNTER_SHOOTING, /// JÃ¤ger: Tier erschieÃŸen
            STATE_HUNTER_WALKINGTOCADAVER, /// JÃ¤ger: Zum Kadaver laufen
            STATE_HUNTER_EVISCERATING, /// JÃ¤ger: Tier ausnehmen
            STATE_CATAPULT_TARGETBUILDING, /// Katapult: Dreht den Katapult oben auf das Ziel zu und schieÃŸt
            STATE_CATAPULT_BACKOFF /// Katapult: beendet schieÃŸen und dreht Katapult in die Ausgangsstellung zurÃ¼ck

        };

    protected:

        State state;

        /// Arbeitsplatz (Haus, in dem er arbeitet)
        nobUsual* workplace;

        // Ware, die er evtl gerade trÃ¤gt
        GoodType ware;

        /// Wieviel er von den letzen 100gf NICHT gearbeitet hat (fÃ¼rs Ausrechnen der ProduktivitÃ¤t)
        unsigned short not_working;
        /// Seit welchem Zeitpunkt (in gf) er ggf. NICHT mehr arbeitet (0xFFFFFFFF = er arbeitet gerade)
        unsigned int since_not_working;

        /// Hat der Bauarbeiter bei seiner Arbeit Sounds von sich gebeben (zu Optimeriungszwecken)
        bool was_sounding;

    protected:

        /// wird von abgeleiteten Klassen aufgerufen, wenn sie die Ware an der Fahne vorm GebÃ¤ude ablegen wollen (oder auch nicht)
        /// also fertig mit Arbeiten sind
        void WorkingReady();
        /// FÃ¤ngt an NICHT zu arbeiten (wird gemessen fÃ¼rs Ausrechnen der ProduktivitÃ¤t)
        void StartNotWorking();
        /// HÃ¶rt auf, nicht zu arbeiten, sprich fÃ¤ngt an zu arbeiten (fÃ¼rs Ausrechnen der ProduktivitÃ¤t)
        void StopNotWorking();
        /// wenn man beim Arbeitsplatz "kÃ¼ndigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
        void AbrogateWorkplace();
        // FÃ¤ngt das "Warten-vor-dem-Arbeiten" an, falls Rohstoffe zum Arbeiten vorhanden sind, ansonsten Warten auf Rohstoffe
        void TryToWork();

    private:

        /// von noFigure aufgerufen
        void Walked(); // wenn man gelaufen ist
        void GoalReached(); // wenn das Ziel erreicht wurde

        /// Malt den Arbeiter beim Arbeiten
        virtual void DrawWorking(int x, int y) = 0;
        /// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren raustrÃ¤gt (bzw rein)
        virtual unsigned short GetCarryID() const = 0;
        /// Laufen an abgeleitete Klassen weiterleiten
        virtual void WalkedDerived() = 0;
        /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
        virtual void WorkAborted();
        /// Arbeitsplatz wurde erreicht
        virtual void WorkplaceReached();

        /// Draws the figure while returning home / entering the building (often carrying wares)
        virtual void DrawReturnStates(const int x, const int y);
        /// Zeichnen der Figur in sonstigen Arbeitslagen
        virtual void DrawOtherStates(const int x, const int y);

        /// nur fÃ¼r Bergarbeiter!
        /// Sucht die NÃ¤he nach einer bestimmten Ressource ab und gibt true zurÃ¼ck, wenn er fÃ¼ndig wird und baut ggf eins
        /// ab, wenn dig = true ist
        bool GetResources(unsigned char type);
        /// Macht das gleiche wie GetResources nur direkt fÃ¼r einen Punkt
        bool GetResourcesOfNode(const MapPoint pt, const unsigned char type);


    public:
        State GetState() { return state; }

        nofBuildingWorker(const Job job, const MapPoint pt, const unsigned char player, nobUsual* workplace);
        nofBuildingWorker(SerializedGameData* sgd, const unsigned obj_id);

        /// AufrÃ¤ummethoden
    protected:  void Destroy_nofBuildingWorker() { Destroy_noFigure(); }
    public:     void Destroy() { Destroy_nofBuildingWorker(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofBuildingWorker(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofBuildingWorker(sgd); }


        void Draw(int x, int y);

        /// Wenn eine neue Ware kommt oder die Produktion wieder erlaubt wurde, wird das aufgerufen
        void GotWareOrProductionAllowed();
        /// Wenn wieder Platz an der Flagge ist und eine Ware wieder rausgetragen werden kann
        bool FreePlaceAtFlag();
        /// Wenn das Haus des Arbeiters abbrennt
        void LostWork();
        /// Rechnet die ProduktivitÃ¤t aus (und setzt den ZÃ¤hler zurÃ¼ck, setzt vorraus, dass das in 100 gf - AbstÃ¤nden aufgerufen wird !!!)
        unsigned short CalcProductivity();
        /// Wird aufgerufen, nachdem die Produktion in dem GebÃ¤ude, wo er arbeitet, verboten wurde
        void ProductionStopped();

    protected:
        bool OutOfRessourcesMsgSent;
};

#endif
