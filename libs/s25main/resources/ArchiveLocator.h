// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ResolvedFile.h"
#include <boost/filesystem/path.hpp>
#include <map>
#include <vector>

class Log;
class ResourceId;

/// Helper for locating archives.
/// An archive is either a file (e.g. *.lst) or a folder containing 1 or more resources.
/// Each archive is uniquely identified by its ResourceId (which can be constructed from a path).
/// The resource system is layered:
/// If an archive exists in multiple folders the archives will be loaded in order with entries present in earlier
/// archives being replaced by those in later ones.
/// This allows modifying existing resources and adding new ones.
class ArchiveLocator
{
    struct FolderData
    {
        /// Path to the folder
        boost::filesystem::path path;
        /// Filenames in the folder
        std::vector<boost::filesystem::path> files;
    };

public:
    explicit ArchiveLocator(Log&);
    /// Add a folder with assets. Each asset (identified via resource id must only exist in 1 of those folders)
    void addAssetFolder(const boost::filesystem::path& path);
    /// Add a folder to the list of folders containing overrides. Files in folders added last will override prior ones
    void addOverrideFolder(const boost::filesystem::path& path);
    /// Remove all entries
    void clear();
    /// Resolve the given file. That is get the files in order to be loaded
    ResolvedFile resolve(const boost::filesystem::path& filepath) const;
    /// Resolve the given asset. Must be in one of the asset folders
    ResolvedFile resolve(const ResourceId& resId) const;

    // TODO: Remove
    const auto& getOverrideFolders() const { return overrideFolders_; }

private:
    template<typename T>
    void gatherFiles(const boost::filesystem::path& path, T&& onValidResource) const;

    Log& logger_;
    std::map<ResourceId, boost::filesystem::path> assets_;
    std::vector<FolderData> overrideFolders_;
};
