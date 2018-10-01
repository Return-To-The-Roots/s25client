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

#include "driverDefines.h" // IWYU pragma: keep
#include "DriverInterfaceVersion.h"

#ifdef _WIN32
#define DRIVERDLLAPI extern "C" __declspec(dllexport)
#else
#define DRIVERDLLAPI extern "C"
#endif // !_WIN32

/**
 *  API-Versions-Lieferfunktion
 *
 *  @return liefert die API-Version des Treibers
 */
DRIVERDLLAPI unsigned GetDriverAPIVersion()
{
    return DRIVERAPIVERSION;
}
