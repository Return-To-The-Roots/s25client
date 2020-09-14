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
//

#pragma once

/// Fehler, die vom Client gemeldet werden
enum ClientError
{
    CE_INCOMPLETE_MESSAGE,
    CE_SERVER_FULL,
    CE_WRONG_PW,
    CE_CONNECTION_LOST,
    CE_INVALID_SERVERTYPE,
    CE_MAP_TRANSMISSION,
    CE_WRONG_VERSION,
    CE_INVALID_MAP
};

const char* ClientErrorToStr(ClientError error);
