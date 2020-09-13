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

#include "helpers/MaxEnumValue.h"
#include "helpers/OptionalEnum.h"
#include "nodeObjs/noMovable.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadPathDirection.h"
#include <cstdint>

class ResourceId;
class RoadSegment;
class noRoadNode;
class glArchivItem_Bob;
enum Job : unsigned char;
enum GoodType : unsigned char;

enum FigureState : uint8_t
{
    FS_GOTOGOAL = 0,
    FS_GOHOME,
    FS_WANDER,
    FS_JOB
};
DEFINE_MAX_ENUM_VALUE(FigureState, FigureState::FS_JOB)

class SerializedGameData;

// Stellt einen Menschen dar
class noFigure : public noMovable
{
protected:
    FigureState fs; // aktueller Status
    Job job_;       // Beruf(sart)
    unsigned char player;

    // Straßenlaufzeug: (nur genutzt beim Laufen im Straßennetz!)

    const RoadSegment* cur_rs; /// Straße, auf der er gerade läuft
    unsigned short rs_pos;     /// Position auf der aktuellen Straße (Wegstück)
    bool rs_dir;               /// von welcher Seite er auf die Straße läuft

    // Befindet sich die Figur gerade auf einem Schiff?
    bool on_ship;

    /// zu welcher Flagge er laufen soll
    noRoadNode* goal_;
    /// ist der Platz davor besetzt (z.B. durch Kampf), muss er warten (Stehen bleiben), sowohl genau auf einem
    /// Wegpunkt, also auch beliebig dazwischen!
    bool waiting_for_free_node;

    // nur bei FS_WANDER von Bedeutung:
    /// Restlicher Weg für das Rumirren (0xFFFF wenn schon auf dem Weg zu einer Flagge!)
    unsigned short wander_way;
    /// Wieviel (erfolglose) Rumirr-Flaggensuch-Versuche hat es schon gegeben (nach bestimmter Zahl Figur sterben
    /// lassen)
    unsigned short wander_tryings;
    /// Falls eine Flagge gefunden wurde, Zielpunkt, der Flagge
    MapPoint flagPos_;
    /// Obj-ID der (damaligen) Flagge, (evtl wurde sie zwischendurch abgerissen)
    unsigned flag_obj_id;
    /// Wenn der Typ aus einem Lagerhaus geflohen ist, Obj-ID des abbrennenden Lagerhauses zur
    /// Kommunikation mit anderen Kollegen, die ebenfalls flüchten --> "Kollektivwegfindung", ansonsten ist das
    /// 0xFFFFFFFF
    unsigned burned_wh_id;

    static const RoadSegment emulated_wanderroad;

    /// Speichert letzten Animationsframes (zum Abspielen von Sounds)
    unsigned last_id;

private:
    /// abgeleitete Klassen informieren, wenn ...
    virtual void GoalReached() = 0; // das Ziel erreicht wurde
    virtual void Walked() = 0;      // man gelaufen ist

    /// Für alle restlichen Events, die nicht von noFigure behandelt werden
    virtual void HandleDerivedEvent(unsigned id) = 0;

    /// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
    virtual unsigned GetVisualRange() const;

    /// Unterfunktion von Wander --> zur Flagge irren
    void WanderToFlag();
    /// Unterfunktion von Wander --> zur Flagge irren
    // void WanderToFlagFailedTrade();

    /// Sichtbarkeiten berechnen für Figuren mit Sichtradius (Soldaten, Erkunder) vor dem Laufen
    void CalcVisibilities(MapPoint pt);

protected:
    /// In aktueller Richtung ein Stück zurcklegen
    void WalkFigure();
    /// Schatten der Figur malen
    void DrawShadow(DrawPoint drawPt, unsigned char anistep, Direction dir);

    /// Herumirren
    void Wander();

    /// Herumirren after failed traderoute
    void WanderFailedTrade();

    virtual void AbrogateWorkplace() = 0;

public:
    /// Konstruktor für Figuren, die auf dem Wegenetz starten
    noFigure(Job job, MapPoint pos, unsigned char player, noRoadNode* goal);
    /// Konstruktor für Figuren, die im Job-Modus starten
    noFigure(Job job, MapPoint pos, unsigned char player);

    noFigure(SerializedGameData& sgd, unsigned obj_id);

    /// Aufräummethoden
protected:
    void Destroy_noFigure();

public:
    void Destroy() override { Destroy_noFigure(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_noFigure(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noFigure(sgd); }

    void HandleEvent(unsigned id) override;

    /// Ziel setzen
    void SetGoalTonullptr() { goal_ = nullptr; }
    /// Ziel zurückgeben
    noRoadNode* GetGoal() const { return goal_; }

    /// Getter
    bool GetRoadDir() const { return rs_dir; }
    const RoadSegment* GetCurrentRoad() const { return cur_rs; }
    bool IsWandering() const { return fs == FS_WANDER; }
    /// Tut was, nachdem er rausgehen soll
    void ActAtFirst();
    /// Legt die Anfangsdaten für das Laufen auf Wegen fest
    void InitializeRoadWalking(const RoadSegment* road, unsigned short rs_pos, bool rs_dir);
    /// Gibt Job-Typ zurück
    Job GetJobType() const { return job_; }
    /// Returns true if this is a soldier (they get some special handling at some points)
    bool IsSoldier() const;

    unsigned CalcWalkAnimationFrame() const;
    DrawPoint InterpolateWalkDrawPos(DrawPoint drawPt) const;

    /// Zeichnet eine Figur aus "carrier.bob" beim Laufen.
    void DrawWalkingCarrier(DrawPoint drawPt, helpers::OptionalEnum<GoodType> ware, bool fat);
    /// Zeichnet eine Figur aus "jobs.bob", wenn sie läuft.
    void DrawWalkingBobJobs(DrawPoint drawPt, Job job);
    /// Zeichnet standardmäßig die Figur, wenn sie läuft
    void DrawWalking(DrawPoint drawPt, glArchivItem_Bob* file, unsigned id, bool fat);
    /// Zeichnet standardmäßig die Figur, wenn sie läuft aus einem bestimmten normalen LST Archiv
    void DrawWalking(DrawPoint drawPt, const ResourceId& file, unsigned id);
    /// Zeichnet standardmäßig die Figur, wenn sie läuft, nimmt automatisch richtige Job-ID/Datei
    void DrawWalking(DrawPoint drawPt);
    /// Interpoliert die Positon zwischen zwei Knotenpunkten
    DrawPoint CalcFigurRelative() const;
    /// Anfangen zu laufen (Event anmelden, Tür aufmachen ggf)
    void StartWalking(Direction dir);
    /// Starts walking in a random dir and returns whether this was possible
    bool WalkInRandomDir();
    /// Umherirren starten (frei rumlaufen)
    void StartWandering(unsigned burned_wh_id = 0xFFFFFFFF);
    /// Auf Straßen(!) nach Hause laufen
    void GoHome(noRoadNode* goal = nullptr);
    /// Aktuellen Weg, auf dem er läuft, fr ungültig erklären
    void CutCurrentRoad() { cur_rs = nullptr; }
    /// Auf Straßen zur Zielflagge laufen
    void WalkToGoal();
    /// Gibt die Straße zurück, auf der man gerade läuft
    const RoadSegment* GetCurrentRoad() { return cur_rs; }
    /// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde, setzt vorraus, dass die Figur auf der geteilten
    /// Straße läuft!
    void CorrectSplitData(const RoadSegment* rs2);
    /// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde (für abgeleitete Klassen)
    virtual void CorrectSplitData_Derived();
    /// Lässt die Figur sterben (löst sich auf und hinterlässt ggf. Leiche)
    void Die();
    /// Removes the figure from the players inventory (e.g. when it does not exist anymore)
    void RemoveFromInventory();

    /// Lässt die Figur sterben (löst sich auf und hinterlässt ggf. Leiche) - does not reduce good counts because for
    /// trade routes they have been reduced already.
    void DieFailedTrade();
    /// Mitglied von nem Lagerhaus(Lagerhausarbeiter, die die Träger-Bestände nicht beeinflussen?)
    /// Kann außer bei WarehouseWorker die Default-Definition gelassen werden
    virtual bool MemberOfWarehouse() const { return false; }

    /// Ein Punkt neben der Figur wurde freigegeben --> wenn sie deswegen angehalten ist, kann sie weiterlaufen
    void NodeFreed(MapPoint pt);

    /// Wartet sie auf einen freien Platz?
    bool IsWaitingForFreeNode() const { return waiting_for_free_node; }
    /// Stoppt, wenn er auf diesen Punkt zuläuft
    void StopIfNecessary(MapPoint pt);

    unsigned char GetPlayer() const { return player; }

    /// Macht die Figur Job-Arbeiten?
    bool DoJobWorks() const { return (fs == FS_JOB); }

    void Abrogate(); // beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)

    /// Informiert die Figur, dass für sie eine Schiffsreise beginnt
    void StartShipJourney();
    /// Tells the figure it arrived at a harbor at the given position
    void ArrivedByShip(MapPoint harborPos);
    /// Gibt zurück, ob die Figur kein Ziel mehr hat und damit nach einer Schifffahrt im
    /// Lagerhaus interniert werden muss
    bool HasNoGoal() const { return (goal_ == nullptr); }
    /// Gibt zurück, ob die Figur auf Straßen läuft zu ihrem Arbeitsplatz o.Ä.
    bool IsWalkingOnRoad() const
    {
        // Nur Träger arbeiten richtig auf Straßen
        if(fs == FS_JOB)
            return (GetGOT() == GOT_NOF_CARRIER);
        else if(fs == FS_GOHOME || fs == FS_GOTOGOAL)
            return true;
        else
            return false;
    }

    /// Examines the route (maybe harbor, road destroyed?) before start shipping
    /// Returns (maybe new) destination harbor ((0,0) if he doesn't go by ship)
    /// and also the new direction it wants to travel which can be the (otherwise invalid) SHIP_DIR if the figure stays
    /// on board
    MapPoint ExamineRouteBeforeShipping(RoadPathDirection& newDir);
};
