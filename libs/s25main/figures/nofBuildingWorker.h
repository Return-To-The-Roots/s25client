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

#ifndef NOF_BUILDING_WORKER_H_
#define NOF_BUILDING_WORKER_H_

#include "figures/noFigure.h"
#include "gameTypes/GoodTypes.h"

class nobUsual;
class nobBaseWarehouse;
class SerializedGameData;

/// Repräsentiert einen Arbeiter in einem Gebäude
class nofBuildingWorker : public noFigure
{
public:
    /// Was der gerade so schönes macht
    enum State
    {
        STATE_FIGUREWORK = 0,                       /// Arbeiten der noFigure (Laufen zum Arbeitsplatz, Rumirren usw)
        STATE_ENTERBUILDING,                        /// Betreten des Gebäudes
        STATE_WAITING1,                             /// Warten, bis man anfängt zu produzieren
        STATE_WAITING2,                             /// Warten nach dem Produzieren, bis man Ware rausträgt (nur Handwerker)
        STATE_CARRYOUTWARE,                         /// Raustragen der Ware
        STATE_WORK,                                 /// Arbeiten
        STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED, /// Warten auf Waren oder weil Produktion eingetellt wurde
        STATE_WALKTOWORKPOINT,                      /// Zum "Arbeitspunkt" laufen (nur Landarbeiter)
        STATE_WALKINGHOME,                          /// vom Arbeitspunkt zurück nach Hause laufen (nur Landarbeiter)
        STATE_WAITFORWARESPACE,                     /// auf einen freien Platz an der Flagge vor dem Gebäude warten
        STATE_HUNTER_CHASING,                       /// Jäger: verfolgt das Tier bis auf eine gewisse Distanz
        STATE_HUNTER_FINDINGSHOOTINGPOINT,          /// Jäger: sucht einen Punkt rund um das Tier, von dem er es abschießen kann
        STATE_HUNTER_SHOOTING,                      /// Jäger: Tier erschießen
        STATE_HUNTER_WALKINGTOCADAVER,              /// Jäger: Zum Kadaver laufen
        STATE_HUNTER_EVISCERATING,                  /// Jäger: Tier ausnehmen
        STATE_CATAPULT_TARGETBUILDING,              /// Katapult: Dreht den Katapult oben auf das Ziel zu und schießt
        STATE_CATAPULT_BACKOFF,                     /// Katapult: beendet schießen und dreht Katapult in die Ausgangsstellung zurück
        STATE_HUNTER_WAITING_FOR_ANIMAL_READY,      /// Hunter: Arrived at shooting pos and waiting for animal to be ready to be shot
    };

protected:
    State state;

    /// Arbeitsplatz (Haus, in dem er arbeitet)
    nobUsual* workplace;

    // Ware, die er evtl gerade trägt
    helpers::OptionalEnum<GoodType> ware;

    /// Hat der Bauarbeiter bei seiner Arbeit Sounds von sich gegeben (zu Optimeriungszwecken)
    bool was_sounding;

protected:
    /// wird von abgeleiteten Klassen aufgerufen, wenn sie die Ware an der Fahne vorm Gebäude ablegen wollen (oder auch nicht)
    /// also fertig mit Arbeiten sind
    void WorkingReady();
    /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
    void AbrogateWorkplace() override;
    /// Tries to start working.
    /// Checks preconditions (production enabled, wares available...) and starts the pre-Work-Waiting period if ok
    void TryToWork();
    /// Returns true, when there are enough wares available for working.
    /// Note: On false, we will wait for the next ware or production change till checking again
    virtual bool AreWaresAvailable() const;

private:
    /// von noFigure aufgerufen
    void Walked() override;      // wenn man gelaufen ist
    void GoalReached() override; // wenn das Ziel erreicht wurde

protected:
    /// Malt den Arbeiter beim Arbeiten
    virtual void DrawWorking(DrawPoint drawPt) = 0;
    static constexpr unsigned short CARRY_ID_CARRIER_OFFSET = 100;
    /// Ask derived class for an ID into JOBS.BOB when the figure is carrying a ware
    /// Use GD_* + CARRY_ID_CARRIER_OFFSET for using carrier graphics
    virtual unsigned short GetCarryID() const = 0;
    /// Laufen an abgeleitete Klassen weiterleiten
    virtual void WalkedDerived() = 0;
    /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
    virtual void WorkAborted();
    /// Arbeitsplatz wurde erreicht
    virtual void WorkplaceReached();

    /// Draws the figure while returning home / entering the building (often carrying wares)
    virtual void DrawWalkingWithWare(DrawPoint drawPt);
    /// Zeichnen der Figur in sonstigen Arbeitslagen
    virtual void DrawOtherStates(DrawPoint drawPt);

public:
    State GetState() { return state; }

    nofBuildingWorker(Job job, MapPoint pos, unsigned char player, nobUsual* workplace);
    nofBuildingWorker(Job job, MapPoint pos, unsigned char player, nobBaseWarehouse* goalWh);
    nofBuildingWorker(SerializedGameData& sgd, unsigned obj_id);

    /// Aufräummethoden
protected:
    void Destroy_nofBuildingWorker()
    {
        RTTR_Assert(!workplace);
        Destroy_noFigure();
    }

public:
    void Destroy() override { Destroy_nofBuildingWorker(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_nofBuildingWorker(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_nofBuildingWorker(sgd); }

    void Draw(DrawPoint drawPt) override;

    /// Wenn eine neue Ware kommt oder die Produktion wieder erlaubt wurde, wird das aufgerufen
    void GotWareOrProductionAllowed();
    /// Wenn wieder Platz an der Flagge ist und eine Ware wieder rausgetragen werden kann
    bool FreePlaceAtFlag();
    /// Wenn das Haus des Arbeiters abbrennt
    void LostWork();
    /// Wird aufgerufen, nachdem die Produktion in dem Gebäude, wo er arbeitet, verboten wurde
    void ProductionStopped();
};

#endif
