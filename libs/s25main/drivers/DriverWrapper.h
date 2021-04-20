// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <boost/dll/shared_library.hpp>
#include <boost/filesystem/path.hpp>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bfs = boost::filesystem;

namespace drivers {

enum class DriverType
{
    Video,
    Audio
};

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

    DriverWrapper();
    ~DriverWrapper();
    /// Läd einen Treiber in die Treiber DLL, versucht, "preference" zu nehmen
    bool Load(DriverType dt, std::string& preference);
    /// Gibt eine Treiber-Handle wieder frei
    void Unload();
    bool IsLoaded() const;
    template<typename T>
    T* GetFunction(const std::string& name)
    {
        return dll.get<T>(name);
    }

    /// Läd eine Liste von verfügbaren Treibern
    static std::vector<DriverItem> LoadDriverList(DriverType dt);

private:
    boost::dll::shared_library dll;
    /// Checks if the library is valid. Puts either the name or the error message into nameOrError
    static bool CheckLibrary(const bfs::path& path, DriverType dt, std::string& nameOrError);
};
} // namespace drivers
