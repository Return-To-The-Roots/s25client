// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ResourceId.h"
#include "helpers/containerUtils.h"
#include "s25util/strAlgos.h"
#include <boost/filesystem/path.hpp>
#include <stdexcept>

ResourceId ResourceId::make(const std::string& name)
{
    if(name.length() > maxLength || !isValid(name.c_str(), name.length()))
        throw std::invalid_argument(name + " is not a valid resource id");

    return ResourceId(name);
}

ResourceId ResourceId::make(const boost::filesystem::path& filepath)
{
    auto name = filepath.stem();
    // remove all additional extensions
    while(name.has_extension())
        name.replace_extension();
    const std::string sName = s25util::toLower(name.string());
    return make(sName);
}

std::ostream& operator<<(std::ostream& os, const ResourceId& resId)
{
    return os.write(resId.name_, resId.length_);
}
