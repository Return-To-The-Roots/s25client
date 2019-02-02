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
#ifndef DRIVERWRAPPER_H_INCLUDED
#define DRIVERWRAPPER_H_INCLUDED

#ifdef DriverType
#undef DriverType
#endif

#include <boost/filesystem/path.hpp>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#ifndef HINSTANCE
#define HINSTANCE void*
#endif
#endif

#include <string>
#include <vector>

namespace bfs = boost::filesystem;

class DriverWrapper
{
public:
    enum DriverType
    {
        DT_VIDEO = 0,
        DT_AUDIO
    };

    class DriverItem
    {
    public:
        DriverItem(const bfs::path& file, const std::string& name) : file(file), name(name) {}
        const bfs::path& GetFile() { return file; }
        const std::string& GetName() { return name; }

    private:
        bfs::path file;
        std::string name;
    };

public:
    DriverWrapper();
    ~DriverWrapper();

    /// Läd einen Treiber in die Treiber DLL, versucht, "preference" zu nehmen
    bool Load(const DriverType dt, std::string& preference);
    /// Gibt eine Treiber-Handle wieder frei
    void Unload();
    /// Gibt Adresse auf eine bestimmte Funktion zurück
    void* GetDLLFunction(const std::string& name);

    /// Läd eine Liste von verfügbaren Treibern
    static std::vector<DriverItem> LoadDriverList(const DriverType dt);

private:
    /// Handle auf die DLL
    HINSTANCE dll;
    /// Checks if the library is valid. Puts either the name or the error message into nameOrError
    static bool CheckLibrary(const bfs::path& path, DriverType dt, std::string& nameOrError);
};

#endif // DRIVERWRAPPER_H_INCLUDED
