// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FindWhConditions.h"
#include "buildings/nobBaseWarehouse.h"
#include "gameData/ShieldConsts.h"

namespace FW {

bool HasMinWares::operator()(const nobBaseWarehouse& wh) const
{
    return wh.getNumRealWares(type) >= count;
}

bool HasFigure::operator()(const nobBaseWarehouse& wh) const
{
    if(wh.getNumRealFigures(type) > 0)
        return true;
    else if(recruitingAllowed && type != Job::PackDonkey)
        return wh.canRecruit(type);
    else
        return false;
}

bool HasWareAndFigure::operator()(const nobBaseWarehouse& wh) const
{
    return HasMinWares::operator()(wh) && HasFigure::operator()(wh);
}

bool HasAnyMatchingSoldier::operator()(const nobBaseWarehouse& wh) const
{
    for(unsigned i = 0; i < SOLDIER_JOBS.size(); i++)
    {
        if(ranks[i] && wh.getNumRealFigures(SOLDIER_JOBS[i]) > 0)
            return true;
    }
    return false;
}

bool AcceptsWare::operator()(const nobBaseWarehouse& wh) const
{
    // Einlagern darf nicht verboten sein
    // Schilder beachten!
    const GoodType good = ConvertShields(type);
    return !wh.getInventorySetting(good).isSet(EInventorySetting::Stop);
}

bool AcceptsFigure::operator()(const nobBaseWarehouse& wh) const
{
    // Boat carriers are normal figures in the wh
    Job job = type;
    if(job == Job::BoatCarrier)
        job = Job::Helper;

    return !wh.getInventorySetting(job).isSet(EInventorySetting::Stop);
}

bool CollectsWare::operator()(const nobBaseWarehouse& wh) const
{
    // Einlagern muss gewollt sein
    // Schilder beachten!
    GoodType gt = ConvertShields(type);
    return (wh.getInventorySetting(gt).isSet(EInventorySetting::Collect));
}

bool CollectsFigure::operator()(const nobBaseWarehouse& wh) const
{
    // Einlagern muss gewollt sein
    Job job = type;
    if(job == Job::BoatCarrier)
        job = Job::Helper;
    return (wh.getInventorySetting(job).isSet(EInventorySetting::Collect));
}

bool HasWareButNoCollect::operator()(const nobBaseWarehouse& wh) const
{
    return HasMinWares::operator()(wh) && !CollectsWare::operator()(wh);
}

bool HasFigureButNoCollect::operator()(const nobBaseWarehouse& wh) const
{
    return HasFigure::operator()(wh) && !CollectsFigure::operator()(wh);
}

bool AcceptsWareButNoSend::operator()(const nobBaseWarehouse& wh) const
{
    const GoodType good = ConvertShields(type);
    return AcceptsWare::operator()(wh) && !wh.getInventorySetting(good).isSet(EInventorySetting::Send);
}

bool AcceptsFigureButNoSend::operator()(const nobBaseWarehouse& wh) const
{
    Job job = type;
    if(job == Job::BoatCarrier)
        job = Job::Helper;

    return AcceptsFigure::operator()(wh) && !wh.getInventorySetting(job).isSet(EInventorySetting::Send);
}

} // namespace FW
