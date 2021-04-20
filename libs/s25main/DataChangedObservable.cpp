// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DataChangedObservable.h"
#include "IDataChangedListener.h"

DataChangedObservable::~DataChangedObservable()
{
    NotifyListeners(0);
}

void DataChangedObservable::AddListener(IDataChangedListener* listener)
{
    listeners.push_back(listener);
}

void DataChangedObservable::RemoveListener(IDataChangedListener* listener)
{
    listeners.remove(listener);
}

void DataChangedObservable::NotifyListeners(unsigned changeId)
{
    for(auto* listener : listeners)
        listener->OnChange(changeId);
}
