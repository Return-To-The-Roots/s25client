// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include "nodeObjs/noMovable.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/RoadPathDirection.h"
#include <cstdint>

class ResourceId;
class RoadSegment;
class noRoadNode;
class glArchivItem_Bob;
enum class Job : uint8_t;
enum class GoodType : uint8_t;

enum class FigureState : uint8_t
{
    GotToGoal,
    GoHome,
    Wander,
    Job
};
constexpr auto maxEnumValue(FigureState)
{
    return FigureState::Job;
}

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

    // nur bei FigureState::Wander von Bedeutung:
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

    /*
       The armor variable should be a member of nofArmored. But this is not possible due to a circular dependency
       between objects during object deserialization. If a soldier is sent to a military building from a warehouse, the
       soldier is inserted into the ordered_troops list of the military building. This soldier is also in the leave
       queue of the warehouse after some time. When the game is stored in this state a circular dependency exists.

       What happens during deserialization?

       The warehouse is deserialized first. And therefore deserialization of the soldier in the leave queue will start.
       The deserialization is done by going up the class hierarchy. In the top base class GameObject the new obejct is
       inserted into the líst of already deserialized objects. But at this time the object is not yet fully constructed.
       In the base class noFigure the goal for the soldier is deserialized. The goal is a military building and holds
       the ordered_troops list. When this list is deserialized we try to deserialize the soldier, we are currently
       creating, again. Because the soldier is in the líst of already deserialized objects we get back a reference to
       this instance from the list instead of creating it new. The soldier is inserted into the ordered_troops list.
       Because this list is a sorted list a comparator is called to do the insertion. The comparator in this case is the
       ComparatorSoldiersByRank. This comparator uses the rank of the soldier, the objectId and the armor for
       comparison. But at this time the armor variable in the nofArmored subclass of the soldier is not yet deserialized
       and has a random value. This leads to wrong insertion into the sorted list. And later on to a wrong application
       state. Because after loading, when in the game the ordered soldier reaches the building the removing from the
       ordered_troops list will fail. This problem does not exist for the rank of the soldier. Because the rank of a
       soldier depends on the job id, which is deserialized in noFigure before the goal is deserialized.
    */
    bool armor;

    explicit noFigure(const noFigure&) = default;

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

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    void HandleEvent(unsigned id) override;

    /// Ziel setzen
    void SetGoalTonullptr() { goal_ = nullptr; }
    /// Ziel zurückgeben
    noRoadNode* GetGoal() const { return goal_; }

    /// Getter
    bool GetRoadDir() const { return rs_dir; }
    const RoadSegment* GetCurrentRoad() const { return cur_rs; }
    bool IsWandering() const { return fs == FigureState::Wander; }
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

    /// Draw the current walk animation frame from "carrier.bob"
    void DrawWalkingCarrier(DrawPoint drawPt, helpers::OptionalEnum<GoodType> ware, bool fat);
    /// Draw the current walk animation frame from "jobs.bob"
    void DrawWalkingBobJobs(DrawPoint drawPt, Job job);
    /// Draw the current walk animation frame using the start index into the given file
    void DrawWalking(DrawPoint drawPt, glArchivItem_Bob* file, unsigned id, bool fat);
    /// Draw the current walk animation frame using the start index into the given LST archive
    /// See @ref calcWalkFrameIndex
    void DrawWalking(DrawPoint drawPt, const ResourceId& file, unsigned id);
    /// Draw the current walk animation for this figure
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
    bool DoJobWorks() const { return fs == FigureState::Job; }

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
        if(fs == FigureState::Job)
            return (GetGOT() == GO_Type::NofCarrier);
        else if(fs == FigureState::GoHome || fs == FigureState::GotToGoal)
            return true;
        else
            return false;
    }

    /// Examines the route (maybe harbor, road destroyed?) before start shipping
    /// Returns (maybe new) destination harbor ((0,0) if he doesn't go by ship)
    /// and also the new direction it wants to travel which can be the (otherwise invalid) SHIP_DIR if the figure stays
    /// on board
    MapPoint ExamineRouteBeforeShipping(RoadPathDirection& newDir);

private:
    /// Calculate the index of the current frame for a figure walking in the given direction
    /// The sprites start at `imgSetIndex` and contain 8 images per direction, starting at EAST going clockwise.
    /// Some of the 48 indices might be empty entries in the archive if only specific directions can ever be drawn
    static unsigned calcWalkFrameIndex(unsigned imgSetIndex, Direction dir, unsigned animationStep);
};
