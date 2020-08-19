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

#include "ArchiveLocator.h"
#include "files.h"
#include "helpers/containerUtils.h"
#include "mygettext/mygettext.h"
#include "resources/ResourceId.h"
#include "s25util/Log.h"
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace fs = boost::filesystem;

ArchiveLocator::ArchiveLocator(Log& logger) : logger_(logger) {}

static boost::optional<ResourceId> makeResourceId(const fs::path& filename)
{
    try
    {
        return ResourceId::make(filename);
    } catch(const std::invalid_argument&)
    {
        return boost::none;
    }
}

template<typename T>
void ArchiveLocator::gatherFiles(const fs::path& path, T&& onValidResource) const
{
    if(!fs::exists(path))
        throw std::runtime_error(std::string("Path ") + path.string() + " does not exist");
    for(const auto& it : fs::directory_iterator(path))
    {
        const auto filename = it.path().filename();
        const auto resId = makeResourceId(filename);
        if(resId)
            onValidResource(*resId, it.path());
        else
        {
            logger_.write(_("Ignoring resource %1% in %2% because the name contains unsupported characters\n"))
              % filename % it.path().parent_path();
        }
    }
}

void ArchiveLocator::addAssetFolder(const boost::filesystem::path& path)
{
    gatherFiles(path, [this](const ResourceId& resId, const fs::path& filepath) {
        auto it = assets_.find(resId);
        if(it == assets_.end())
            assets_[resId] = filepath;
        else
        {
            logger_.write(_("Ignoring resource %1% in %2% because it is already found in %3%\n")) % resId
              % filepath.parent_path() % it->second.parent_path();
        }
    });
}

void ArchiveLocator::addOverrideFolder(const boost::filesystem::path& path)
{
    FolderData folder;
    folder.path = path;
    gatherFiles(path,
                [&folder](auto&& /*resId*/, const fs::path& filepath) { folder.files.push_back(filepath.filename()); });
    // Don't add folders twice
    if(helpers::contains_if(overrideFolders_,
                            [&path](const auto& curOverride) { return fs::equivalent(curOverride.path, path); }))
        throw std::runtime_error(std::string("Path ") + path.string() + " already added");
    overrideFolders_.push_back(std::move(folder));
}

void ArchiveLocator::clear()
{
    assets_.clear();
    overrideFolders_.clear();
}

ResolvedFile ArchiveLocator::resolve(const ResourceId& resId) const
{
    auto it = assets_.find(resId);
    return (it != assets_.end()) ? resolve(it->second) : ResolvedFile();
}

ResolvedFile ArchiveLocator::resolve(const boost::filesystem::path& filepath) const
{
    ResolvedFile result;
    result.push_back(filepath);
    const ResourceId resId = ResourceId::make(filepath);
    for(const FolderData& overrideFolder : overrideFolders_)
    {
        auto itFile =
          helpers::find_if(overrideFolder.files, [resId](const auto& file) { return ResourceId::make(file) == resId; });
        if(itFile != overrideFolder.files.end())
        {
            const auto fullFilePath = overrideFolder.path / *itFile;
            if(!fs::exists(fullFilePath))
                logger_.write(_("Skipping removed file %1% when checking for files to load for %2%\n")) % fullFilePath
                  % resId;
            else if(helpers::contains(result, fullFilePath))
                logger_.write(_("Skipping duplicate override file %1% for %2%\n")) % fullFilePath % resId;
            else
                result.push_back(fullFilePath);
        }
    }
    return result;
}
