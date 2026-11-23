// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofChurchWorker.h"
#include "buildings/nobChurch.h"

nofChurchWorker::nofChurchWorker(const MapPoint pos, const unsigned char player, nobUsual* const workplace)
    : nofWorkman(Job::Helper, pos, player, workplace)
{}

nofChurchWorker::nofChurchWorker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofChurchWorker::drawWorking(DrawPoint /*drawPt*/) {}

unsigned short nofChurchWorker::getCarryID() const
{
    return 0;
}

bool nofChurchWorker::areWaresAvailable() const
{
    return nofWorkman::areWaresAvailable() && !static_cast<const nobChurch*>(workplace)->isBlessingStorageFull();
}

helpers::OptionalEnum<GoodType> nofChurchWorker::produceWare()
{
    static_cast<nobChurch*>(workplace)->addBlessing();
    return std::nullopt;
}
