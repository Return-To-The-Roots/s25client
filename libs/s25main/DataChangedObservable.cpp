// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DataChangedObservable.h"
#include "IDataChangedListener.h"

DataChangedObservable::~DataChangedObservable()
{
    notifyListeners(0);
}

void DataChangedObservable::addListener(IDataChangedListener* listener)
{
    listeners.push_back(listener);
}

void DataChangedObservable::removeListener(IDataChangedListener* listener)
{
    listeners.remove(listener);
}

void DataChangedObservable::notifyListeners(unsigned changeId)
{
    for(auto* listener : listeners)
        listener->onChange(changeId);
}
