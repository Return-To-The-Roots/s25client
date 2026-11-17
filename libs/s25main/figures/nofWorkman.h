// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include "nofBuildingWorker.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/Resource.h"
class SerializedGameData;
class nobBaseWarehouse;
class nobUsual;

/// Ein Handwerker ist jemand, der seine Waren nur im Gebäude herstellt und diese rausträgt
/// Allen gemeinsam ist, dass sie einen festen Arbeitsablauf haben:
/// Warten -- Arbeiten -- Warten -- Ware raustragen -- wieder reinkommen -- ...
class nofWorkman : public nofBuildingWorker
{
private:
    // Funktionen, die nur von der Basisklasse  aufgerufen werden, wenn...
    void walkedDerived() override {} // man gelaufen ist
    /// Gibt den Warentyp zurück, welche der Arbeiter erzeugen will
    virtual helpers::OptionalEnum<GoodType> produceWare() = 0;
    /// Abgeleitete Klasse informieren, wenn man fertig ist mit Arbeiten
    virtual void workFinished() {}

protected:
    /// Entsprechende Methoden für die Abwicklung der einzelnen Zustände
    /// Nach erstem Warten, sprich der Arbeiter muss versuchen, neu anfangen zu arbeiten
    void handleStateWaiting1();
    void handleStateWaiting2();
    void handleStateWork();
    /// Called when the work actually starts. The base implementation changes the state and reduces ware count.
    /// Subclasses can return false, if work has to be aborted for any reason in which case we enter the wait-for-wares
    /// state
    virtual bool startWorking();

    /// Looks for a point with a given resource on the node
    MapPoint findPointWithResource(ResourceType type) const;

public:
    nofWorkman(Job job, MapPoint pos, unsigned char player, nobUsual* workplace);
    nofWorkman(SerializedGameData& sgd, unsigned obj_id);

    void handleDerivedEvent(unsigned id) override;
};
