// Copyright (c) 2005 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ArchiveLoader.h"
#include "ResolvedFile.h"
#include "Timer.h"
#include "commonDefines.h"
#include "helpers/format.hpp"
#include "mygettext/mygettext.h"
#include "ogl/glArchivItem_Bob.h"
#include "libsiedler2/Archiv.h"
#include "libsiedler2/ArchivItem_Bob.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/ErrorCodes.h"
#include "libsiedler2/libsiedler2.h"
#include "s25util/Log.h"
#include "s25util/strAlgos.h"
#include <boost/filesystem.hpp>
#include <boost/pointer_cast.hpp>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace fs = boost::filesystem;

template<typename... T>
LoadError::LoadError(T&&... args) : std::runtime_error(helpers::format(std::forward<T>(args)...))
{}

namespace {
class NestedArchive : public libsiedler2::Archiv, public libsiedler2::ArchivItem
{
public:
    NestedArchive(libsiedler2::Archiv&& archive) : libsiedler2::Archiv(std::move(archive)) {}
    RTTR_CLONEABLE(NestedArchive)
};

bool isBobOverride(fs::path filePath)
{
    // For files we ignore the first extension as that is the type of the file (e.g. foo.bob.lst is a bob override file
    // packed as lst) Note that the next call to `extension()` can return an empty path if only 1 extension was present
    // Folders can be named anything so they must be named foo.bob directly rather than foo.bob.lst
    if(fs::is_regular_file(filePath))
        filePath.replace_extension();
    return s25util::toLower(filePath.extension().string()) == ".bob";
}

std::map<uint16_t, uint16_t> extractBobMapping(libsiedler2::Archiv& archive, const fs::path& filepath)
{
    std::unique_ptr<libsiedler2::ArchivItem_Text> txtItem;
    for(auto& entry : archive)
    {
        if(entry && entry->getBobType() == libsiedler2::BobType::Text)
        {
            if(txtItem)
                throw LoadError(_("Bob-like file contained multiple text entries: %s\n"), filepath);
            txtItem = boost::static_pointer_cast<libsiedler2::ArchivItem_Text>(std::move(entry));
        }
    }
    if(!txtItem)
        return {};
    std::istringstream s(txtItem->getText());
    return libsiedler2::ArchivItem_Bob::readLinks(s);
}
} // namespace

/// Load a single file into the archive
libsiedler2::Archiv ArchiveLoader::loadFile(const fs::path& filePath,
                                            const libsiedler2::ArchivItem_Palette* palette) const
{
    logger_.write(_("Loading %1%: ")) % filePath;

    libsiedler2::Archiv archive;
    if(int ec = libsiedler2::Load(filePath, archive, palette))
        throw LoadError(libsiedler2::getErrorString(ec));

    return archive;
}

libsiedler2::Archiv ArchiveLoader::loadDirectory(const fs::path& filePath,
                                                 const libsiedler2::ArchivItem_Palette* palette) const
{
    logger_.write(_("Loading directory %s\n")) % filePath;
    std::vector<libsiedler2::FileEntry> files = libsiedler2::ReadFolderInfo(filePath);
    logger_.write(_("  Loading %1% entries: ")) % files.size();

    libsiedler2::Archiv archive;

    if(int ec = libsiedler2::LoadFolder(std::move(files), archive, palette))
        throw LoadError(libsiedler2::getErrorString(ec));

    return archive;
}

libsiedler2::Archiv ArchiveLoader::loadFileOrDir(const fs::path& filePath,
                                                 const libsiedler2::ArchivItem_Palette* palette) const
{
    const auto fileStatus = status(filePath);
    if(!exists(fileStatus))
        throw LoadError(_("File or directory does not exist: %s\n"), filePath);
    if(!is_regular_file(fileStatus) && !is_directory(fileStatus))
        throw LoadError(_("Could not determine type of path %s\n"), filePath);

    try
    {
        const Timer timer(true);

        libsiedler2::Archiv result;
        if(is_directory(fileStatus))
            result = loadDirectory(filePath, palette);
        else
            result = loadFile(filePath, palette);

        using namespace std::chrono;
        // TODO: Change translations and use chronoIO
        logger_.write(_("done in %ums\n")) % duration_cast<milliseconds>(timer.getElapsed()).count();

        return result;
    } catch(const LoadError& e)
    {
        logger_.write(_("failed: %1%\n")) % e.what();
        throw LoadError();
    }
}

void ArchiveLoader::mergeArchives(libsiedler2::Archiv& targetArchiv, libsiedler2::Archiv& otherArchiv)
{
    if(targetArchiv.size() < otherArchiv.size())
        targetArchiv.alloc_inc(otherArchiv.size() - targetArchiv.size());
    for(unsigned i = 0; i < otherArchiv.size(); i++)
    {
        // Skip empty entries
        if(!otherArchiv[i])
            continue;
        // If target entry is empty, just move the new one
        if(!targetArchiv[i])
            targetArchiv.set(i, otherArchiv.release(i));
        else
        {
            auto* subArchiv = dynamic_cast<libsiedler2::Archiv*>(targetArchiv[i]);
            if(subArchiv)
            {
                // We have a sub-archiv -> Merge
                auto* otherSubArchiv = dynamic_cast<libsiedler2::Archiv*>(otherArchiv[i]);
                if(!otherSubArchiv)
                    throw LoadError(_("Failed to merge entry %1%. Archive expected!\n"), i);
                mergeArchives(*subArchiv, *otherSubArchiv);
            } else
                targetArchiv.set(i, otherArchiv.release(i)); // Just replace
        }
    }
}

libsiedler2::Archiv ArchiveLoader::load(const ResolvedFile& file, const libsiedler2::ArchivItem_Palette* palette) const
{
    libsiedler2::Archiv archive;
    for(const fs::path& curFilepath : file)
    {
        try
        {
            libsiedler2::Archiv newEntries = loadFileOrDir(curFilepath, palette);

            std::map<uint16_t, uint16_t> bobMapping;
            if(isBobOverride(curFilepath))
            {
                bobMapping = extractBobMapping(newEntries, curFilepath);
                // Emulate bob structure: Single file where first entry is the BOB archive
                libsiedler2::Archiv bobArchive;
                bobArchive.push(std::make_unique<NestedArchive>(std::move(newEntries)));
                using std::swap;
                swap(bobArchive, newEntries);
            }
            mergeArchives(archive, newEntries);
            if(!bobMapping.empty() && !archive.empty() && archive[0]->getBobType() == libsiedler2::BobType::Bob)
                checkedCast<glArchivItem_Bob*>(archive[0])->mergeLinks(bobMapping);
        } catch(const LoadError& e)
        {
            if(e.what() != std::string())
                logger_.write("Exception caught: %1%\n") % e.what();
            throw LoadError();
        }
    }
    return archive;
}
