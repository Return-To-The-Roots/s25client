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
// but WITHOUT ANY WARRANTY{} without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "s25util/MessageInterface.h"
#include "s25util/warningSuppression.h"
#include <boost/preprocessor.hpp>

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define __GENERATE_FWD_DECL_SINGLE(s, data, expression) class expression;
#define __GENERATE_FWD_DECL(...) BOOST_PP_SEQ_FOR_EACH(__GENERATE_FWD_DECL_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

#define __GENERATE_CALLBACK_SINGLE(s, data, expression) \
    virtual bool OnGameMessage(const expression& /*msg*/) { return false; }
#define __GENERATE_CALLBACKS(...) BOOST_PP_SEQ_FOR_EACH(__GENERATE_CALLBACK_SINGLE, 0, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

/// Generates the class "GameMessageInterface" with one virtual method "OnGameMessage" for each parameter
/// Also handles the required forward declaration of the classes
/// Example: GENERATE_GAME_MESSAGE_INTERFACE(GM_Foo, GM_Bar) -->
///     class GM_Foo; class GM_Bar;
///     class GameMessageInterface (...)
///         virtual bool OnGameMessage(const GM_Foo& /*msg*/){}
///         virtual bool OnGameMessage(const GM_Bar& /*msg*/){}
///     };
///
#define GENERATE_GAME_MESSAGE_INTERFACE(...)             \
    __GENERATE_FWD_DECL(__VA_ARGS__)                     \
    class GameMessageInterface : public MessageInterface \
    {                                                    \
    protected:                                           \
        ~GameMessageInterface() override {}              \
                                                         \
    public:                                              \
        __GENERATE_CALLBACKS(__VA_ARGS__)                \
    };

RTTR_IGNORE_OVERLOADED_VIRTUAL
GENERATE_GAME_MESSAGE_INTERFACE(GameMessage_Ping, GameMessage_Pong,

                                GameMessage_Server_Type, GameMessage_Server_TypeOK, GameMessage_Server_Password, GameMessage_Server_Name,
                                GameMessage_Server_Start, GameMessage_Chat, GameMessage_Server_Async, GameMessage_Countdown,
                                GameMessage_CancelCountdown,

                                GameMessage_Player_Id, GameMessage_Player_Name, GameMessage_Player_List, GameMessage_Player_State,
                                GameMessage_Player_Nation, GameMessage_Player_Team, GameMessage_Player_Color, GameMessage_Player_Kicked,
                                GameMessage_Player_Ping, GameMessage_Player_New, GameMessage_Player_Ready, GameMessage_Player_Swap,
                                GameMessage_Player_SwapConfirm,

                                GameMessage_Map_Info, GameMessage_MapRequest, GameMessage_Map_Data, GameMessage_Map_Checksum,
                                GameMessage_Map_ChecksumOK, GameMessage_GGSChange, GameMessage_RemoveLua, GameMessage_Pause,
                                GameMessage_SkipToGF, GameMessage_Server_NWFDone, GameMessage_GameCommand, GameMessage_Speed,

                                GameMessage_GetAsyncLog, GameMessage_AsyncLog)
RTTR_POP_DIAGNOSTIC
