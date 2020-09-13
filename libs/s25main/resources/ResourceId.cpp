// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
    return os << resId.name_;
}
