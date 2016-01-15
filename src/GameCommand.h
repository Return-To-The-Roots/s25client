// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GameCommand_h__
#define GameCommand_h__

class Serializer;
class GameWorldGame;
class GameClientPlayer;
class AIInterface;
class GameClient;
template<typename T>
class GameCommandFactory;

// fwd decl
namespace gc{ class GameCommand; }
void intrusive_ptr_add_ref(gc::GameCommand* x);
void intrusive_ptr_release(gc::GameCommand* x);

#include <boost/intrusive_ptr.hpp>

// Macro used by all derived GameCommands to allow specified class access to non-public members (e.g. contructor)
// Only factory classes should be in here
#define GC_FRIEND_DECL friend class GameCommand; friend class ::GameCommandFactory<GameClient>; friend class ::GameCommandFactory<AIInterface>


namespace gc
{

    enum Type
    {
        NOTSEND = 0,
        NOTHING,
        SETFLAG,
        DESTROYFLAG,
        BUILDROAD,
        DESTROYROAD,
        CHANGEDISTRIBUTION,
        CHANGEBUILDORDER,
        SETBUILDINGSITE,
        DESTROYBUILDING,
        CHANGETRANSPORT,
        CHANGEMILITARY,
        CHANGETOOLS,
        CALLGEOLOGIST,
        CALLSCOUT,
        ATTACK,
        SWITCHPLAYER,
        TOGGLECOINS,
        TOGGLEPRODUCTION,
        CHANGEINVENTORYSETTING,
        CHANGEALLINVENTORYSETTINGS,
        CHANGERESERVE,
        SUGGESTPACT,
        ACCEPTPACT,
        CANCELPACT,
        TOGGLESHIPYARDMODE,
        STARTEXPEDITION,
        STARTATTACKINGEXPEDITION,
        EXPEDITION_COMMAND,
        SEAATTACK,
        STARTEXPLORATIONEXPEDITION,
        TRADEOVERLAND,
        SURRENDER,
        CHEAT_ARMAGEDDON,
        DESTROYALL,
        UPGRADEROAD,
        SENDSOLDIERSHOME,
        ORDERNEWSOLDIERS,
        NOTIFYALLIESOFLOCATION
    };

    class GameCommand
    {
        /// Type of this command
        Type gst;
        unsigned refCounter_;
        friend void ::intrusive_ptr_add_ref(GameCommand* x);
        friend void ::intrusive_ptr_release(GameCommand* x);
    public:
        GameCommand(const GameCommand& obj): gst(obj.gst), refCounter_(0) // Do not copy refCounter!
        {}
        virtual ~GameCommand(void) {}

        GameCommand& operator=(const GameCommand& obj)
        {
            if(this == &obj)
                return *this;
            gst = obj.gst;
            // Do not copy or reset refCounter!
            return *this;
        }

        /// Builds a GameCommand depending on Type
        static GameCommand* Deserialize(const Type gst, Serializer& ser);

        /// Returns the Type
        Type GetType() const { return gst; }
        /// Serializes this GameCommand
        virtual void Serialize(Serializer& ser) const = 0;

        /// Execute this GameCommand
        virtual void Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid) = 0;

    protected:
        GameCommand(const Type gst) : gst(gst), refCounter_(0) {}
    };

    // Use this for safely using Pointers to GameCommands
    typedef boost::intrusive_ptr<GameCommand> GameCommandPtr;

} // ns gc

inline void intrusive_ptr_add_ref(gc::GameCommand* x){
    ++x->refCounter_;
}

inline void intrusive_ptr_release(gc::GameCommand* x){
    RTTR_Assert(x->refCounter_);
    if(--x->refCounter_ == 0) 
        delete x;
}

#endif // GameCommand_h__
