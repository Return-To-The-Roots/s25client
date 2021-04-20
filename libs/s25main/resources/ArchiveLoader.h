// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
