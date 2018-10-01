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

#ifndef IDataChangedListener_h__
#define IDataChangedListener_h__

/// Interface for a listener that gets notifed on specific changes
class IDataChangedListener
{
protected:
    ~IDataChangedListener() {}

public:
    /// Handles a change, changeId is the type of the change (object dependent)
    virtual void OnChange(unsigned changeId) = 0;
};

#endif // IDataChangedListener_h__
