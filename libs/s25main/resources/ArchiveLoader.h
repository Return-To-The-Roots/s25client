// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include <boost/filesystem/path.hpp>
#include <stdexcept>

class Log;
class ResolvedFile;

namespace libsiedler2 {
class Archiv;
class ArchivItem_Palette;
} // namespace libsiedler2

/// Exception thrown when loading failed
class LoadError : public std::runtime_error
{
public:
    LoadError() : std::runtime_error("") {}
    template<typename... T>
    explicit LoadError(T&&... args);
};

class ArchiveLoader
{
public:
    explicit ArchiveLoader(Log& logger) : logger_(logger) {}
    /// Load a resolved file. Throws a LoadError on error.
    libsiedler2::Archiv load(const ResolvedFile& file, const libsiedler2::ArchivItem_Palette* palette = nullptr) const;
    /// Load a file or directory. Throws a LoadError on error.
    libsiedler2::Archiv loadFileOrDir(const boost::filesystem::path& filePath,
                                      const libsiedler2::ArchivItem_Palette* palette = nullptr) const;
    /// Recursively merge 2 archives.
    static void mergeArchives(libsiedler2::Archiv& targetArchiv, libsiedler2::Archiv& otherArchiv);

private:
    /// Load a single file, logs a message without trailing newline on start and throws a LoadError on error.
    libsiedler2::Archiv loadFile(const boost::filesystem::path& filePath,
                                 const libsiedler2::ArchivItem_Palette* palette) const;
    /// Load a single file, logs a message without trailing newline on start and throws a LoadError on error.
    libsiedler2::Archiv loadDirectory(const boost::filesystem::path& filePath,
                                      const libsiedler2::ArchivItem_Palette* palette) const;

    Log& logger_;
};
