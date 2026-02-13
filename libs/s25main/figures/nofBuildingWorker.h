// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"
#include "gameTypes/GoodTypes.h"

class nobUsual;
class nobBaseWarehouse;
class SerializedGameData;

/// Represents a worker in a building
class nofBuildingWorker : public noFigure
{
public:
    /// Current activity/state
    enum class State : uint8_t
    {
        FigureWork,                         /// noFigure work (walking to workplace, wandering, etc.)
        EnterBuilding,                      /// Entering the building
        Waiting1,                           /// Waiting to start producing
        Waiting2,                           /// Waiting after producing before carrying out the ware (craftsmen only)
        CarryoutWare,                       /// Carrying out the ware
        Work,                               /// Working
        WaitingForWaresOrProductionStopped, /// Waiting for wares or because production was stopped
        WalkToWorkpoint,                    /// Walking to the "work point" (farmers only)
        WalkingHome,                        /// Walking back home from the work point (farmers only)
        WaitForWareSpace,                   /// Waiting for a free slot at the flag in front of the building
        HunterChasing,                      /// Hunter: chasing the animal up to a certain distance
        HunterFindingShootingpoint,  /// Hunter: finds a point around the animal to shoot from
        HunterShooting,              /// Hunter: shooting the animal
        HunterWalkingToCadaver,      /// Hunter: walking to the cadaver
        HunterEviscerating,          /// Hunter: eviscerating the animal
        CatapultTargetBuilding,      /// Catapult: turns to the target building and shoots
        CatapultBackoff,             /// Catapult: stops shooting and turns back to the start position
        HunterWaitingForAnimalReady, /// Hunter: Arrived at shooting pos and waiting for animal to be ready to
                                     /// be shot
    };
    friend constexpr auto maxEnumValue(State) { return State::HunterWaitingForAnimalReady; }

protected:
    State state;

    /// Workplace (house he works in)
    nobUsual* workplace;

    // Ware currently being carried (if any)
    helpers::OptionalEnum<GoodType> ware;

    /// Whether the worker produced sounds while working (optimization)
    bool was_sounding;

    /// Called by derived classes when they want to drop the ware at the flag (or not), i.e. finished working
    void WorkingReady();
    /// If the worker should "quit" the workplace, stop walking to the target (for any reason)
    void AbrogateWorkplace() override;
    /// Tries to start working.
    /// Checks preconditions (production enabled, wares available...) and starts the pre-Work-Waiting period if ok
    void TryToWork();
    /// Returns true, when there are enough wares available for working.
    /// Note: On false, we will wait for the next ware or production change till checking again
    virtual bool AreWaresAvailable() const;

private:
    /// Called by noFigure
    void Walked() override;      // after walking
    void GoalReached() override; // when the goal is reached

protected:
    /// Draws the worker while working
    virtual void DrawWorking(DrawPoint drawPt) = 0;
    static constexpr unsigned short CARRY_ID_CARRIER_OFFSET = 100;
    /// Ask derived class for an ID into JOBS.BOB when the figure is carrying a ware
    /// Use GD_* + CARRY_ID_CARRIER_OFFSET for using carrier graphics
    virtual unsigned short GetCarryID() const = 0;
    /// Forward walking to derived classes
    virtual void WalkedDerived() = 0;
    /// Work had to be aborted because the workplace was lost
    virtual void WorkAborted();
    /// Workplace was reached
    virtual void WorkplaceReached();

    /// Draws the figure while returning home / entering the building (often carrying wares)
    virtual void DrawWalkingWithWare(DrawPoint drawPt);
    /// Draw the figure in other work states
    virtual void DrawOtherStates(DrawPoint drawPt);

public:
    State GetState() const { return state; }

    nofBuildingWorker(Job job, MapPoint pos, unsigned char player, nobUsual* workplace);
    nofBuildingWorker(Job job, MapPoint pos, unsigned char player, nobBaseWarehouse* goalWh);
    nofBuildingWorker(SerializedGameData& sgd, unsigned obj_id);
    nofBuildingWorker(const nofBuildingWorker&) = delete;

    void Destroy() override
    {
        RTTR_Assert(!workplace);
        noFigure::Destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    void Draw(DrawPoint drawPt) override;

    /// Called when a new ware arrives or production is allowed again
    void GotWareOrProductionAllowed();
    /// Called when there is space at the flag so a ware can be carried out again
    bool FreePlaceAtFlag();
    /// Called when the worker's house burns down
    void LostWork();
    /// Called after production is forbidden in the building where he works
    void ProductionStopped();
};
