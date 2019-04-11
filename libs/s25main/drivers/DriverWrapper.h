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

#include <boost/filesystem/path.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bfs = boost::filesystem;
namespace boost { namespace dll {
    class shared_library;
}} // namespace boost::dll

namespace drivers {

enum class DriverType
{
    Video,
    Audio
};

template<typename T, typename From>
constexpr T* FunctionPointerCast(From* from)
{
    static_assert(std::is_void<From>::value, "Function must be used only for casting from void pointers");
    static_assert(sizeof(From*) == sizeof(T*), "Pointer sizes don't match");
    return reinterpret_cast<T*>(from);
}

class DriverWrapper
{
public:
    class DriverItem
    {
    public:
        DriverItem(bfs::path file, std::string name) : file(std::move(file)), name(std::move(name)) {}
        const bfs::path& GetFile() const { return file; }
        const std::string& GetName() const { return name; }

    private:
        bfs::path file;
        std::string name;
    };

public:
    DriverWrapper();
    ~DriverWrapper();
    /// Läd einen Treiber in die Treiber DLL, versucht, "preference" zu nehmen
    bool Load(DriverType dt, std::string& preference);
    /// Gibt eine Treiber-Handle wieder frei
    void Unload();
    /// Gibt Adresse auf eine bestimmte Funktion zurück
    void* GetDLLFunction(const std::string& name);
    template<typename T>
    T* GetFunction(const std::string& name)
    {
        return FunctionPointerCast<T>(GetDLLFunction(name));
    }

    /// Läd eine Liste von verfügbaren Treibern
    static std::vector<DriverItem> LoadDriverList(DriverType dt);

private:
    std::unique_ptr<boost::dll::shared_library> dll;
    /// Checks if the library is valid. Puts either the name or the error message into nameOrError
    static bool CheckLibrary(const bfs::path& path, DriverType dt, std::string& nameOrError);
};
} // namespace drivers

#endif // DRIVERWRAPPER_H_INCLUDED
