// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h"
#include "DataChangedObservable.h"
#include "IDataChangedListener.h"

// Include last!
#include "DebugNew.h"

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
    for(std::list<IDataChangedListener*>::iterator it = listeners.begin(); it != listeners.end(); ++it)
        (*it)->OnChange(changeId);
}