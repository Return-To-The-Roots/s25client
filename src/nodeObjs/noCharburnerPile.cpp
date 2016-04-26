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

#include "defines.h" // IWYU pragma: keep
#include "noCharburnerPile.h"

#include "Loader.h"
#include "GameClient.h"
#include "SerializedGameData.h"
#include "EventManager.h"
#include "world/GameWorldGame.h"
#include "noEnvObject.h"
#include "noFire.h"
#include "ogl/glArchivItem_Bitmap.h"


/// Length of the smoldering
const unsigned SMOLDERING_LENGTH = 3000;
/// if nothing happens in this amount of GF the pile will catch fire and burn down (removes inactive piles)
/// it takes about ~4000 gf to remove the cover & harvest the coal from a priority pile - so after this delay there are ~8k gf remaining for the charburner to start working on the started pile again
/// -> this should only run out if there is either a very long time without resources or the building was destroyed/stopped
const unsigned SELFDESTRUCT_DELAY = 12000;

/// Work steps for the construction of the wood pile and the cover
const unsigned short CONSTRUCTION_WORKING_STEPS[2] = {6, 6};
/// Work steps for one graphical step during the remove of the cover
const unsigned short REMOVECOVER_WORK_STEPS = 1;
/// Work steps for one graphical step during the "harvest"
const unsigned short HARVEST_WORK_STEPS = 1;

noCharburnerPile::noCharburnerPile(const MapPoint pos) : noCoordBase(NOP_CHARBURNERPILE, pos),
    state(STATE_WOOD), step(0), sub_step(1), event(NULL)
{
}

noCharburnerPile::~noCharburnerPile()
{

}

void noCharburnerPile::Destroy_noCharburnerPile()
{
    em->RemoveEvent(event);

    // Bauplätze drumrum neu berechnen
    gwg->RecalcBQAroundPointBig(pos);

    Destroy_noCoordBase();
}

void noCharburnerPile::Serialize_noCharburnerPile(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(state));
    sgd.PushUnsignedShort(step);
    sgd.PushUnsignedShort(sub_step);
    sgd.PushObject(event, true);
}

noCharburnerPile::noCharburnerPile(SerializedGameData& sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    state(State(sgd.PopUnsignedChar())),
    step(sgd.PopUnsignedShort()),
    sub_step(sgd.PopUnsignedShort()),
    event(sgd.PopEvent())
{
}

void noCharburnerPile::Draw( int x, int y)
{
    switch(state)
    {
        case STATE_WOOD:
        {
            // Draw sand on which the wood stack is constructed
            LOADER.GetImageN("charburner_bobs", 25)->Draw(x, y);

            glArchivItem_Bitmap* image;
            if(step == 0)
                image = LOADER.GetImageN("charburner_bobs", 26);
            else
            {
                image = LOADER.GetImageN("charburner_bobs", 28);

                // Draw wood pile beneath the cover
                LOADER.GetImageN("charburner_bobs", 26)->Draw(x, y);
            }

            unsigned short progress = sub_step * image->getHeight() / CONSTRUCTION_WORKING_STEPS[step];
            unsigned short height = image->getHeight() - progress;
            if(progress != 0)
                image->Draw(x, y + height, 0, 0, 0, height, 0, progress);
        } return;
        case STATE_SMOLDERING:
        {
            LOADER.GetImageN("charburner_bobs", 27 + GAMECLIENT.GetGlobalAnimation(2, 10, 1, GetObjId() + this->pos.x * 10 + this->pos.y * 10))->Draw(x, y);

            // Dann Qualm zeichnen
            unsigned int globalAnimation = GAMECLIENT.GetGlobalAnimation(8, 5, 2, (this->pos.x + this->pos.y) * 100);
            LOADER.GetMapImageN(692 + 1 * 8 + globalAnimation)->Draw(x + 21, y - 11, 0, 0, 0, 0, 0, 0, 0x99EEEEEE); //-V525
            LOADER.GetMapImageN(692 + 2 * 8 + globalAnimation)->Draw(x - 02, y - 06, 0, 0, 0, 0, 0, 0, 0x99EEEEEE);
            LOADER.GetMapImageN(692 + 1 * 8 + globalAnimation)->Draw(x - 25, y - 11, 0, 0, 0, 0, 0, 0, 0x99EEEEEE);
            LOADER.GetMapImageN(692 + 3 * 8 + globalAnimation)->Draw(x - 02, y - 35, 0, 0, 0, 0, 0, 0, 0x99EEEEEE);
        } return;
        case STATE_REMOVECOVER:
        {
            LOADER.GetImageN("charburner_bobs", 28 + step)->Draw(x, y);
        } return;
        case STATE_HARVEST:
        {
            LOADER.GetImageN("charburner_bobs", 34 + step)->Draw(x, y);

        } return;
        default: return;
    }
}

void noCharburnerPile::HandleEvent(const unsigned int  /*id*/)
{
    // Smoldering is over
    // Pile is ready for the remove of the cover
	if(state==STATE_SMOLDERING)
	{
		state = STATE_REMOVECOVER;
		//start a selfdestruct timer
		event = em->AddEvent(this, SELFDESTRUCT_DELAY,0);
	}
	else
	{
		//selfdestruct!
		event = NULL;
		gwg->SetNO(pos, new noFire(pos, 0), true);
		gwg->RecalcBQAroundPoint(pos);
		em->AddToKillList(this);
	}
}


/// Charburner has worked on it --> Goto next step
void noCharburnerPile::NextStep()
{
	//reset selfdestruct timer
	if(event)
		em->RemoveEvent(event);
	event = em->AddEvent(this,SELFDESTRUCT_DELAY,0);

	//execute step
    switch(state)
    {
        default: return;
        case STATE_WOOD:
        {
            ++sub_step;
            if(sub_step == CONSTRUCTION_WORKING_STEPS[step])
            {
                ++step;
                sub_step = 0;

                // Reached new state?
                if(step == 2)
                {
                    step = 0;
                    state = STATE_SMOLDERING;
					em->RemoveEvent(event);
                    event = em->AddEvent(this, SMOLDERING_LENGTH, 0);
                }
            }

        } return;
        case STATE_REMOVECOVER:
        {
            ++sub_step;
            if(sub_step == REMOVECOVER_WORK_STEPS)
            {
                ++step;
                sub_step = 0;

                // Reached new state?
                if(step == 6)
                {
                    state = STATE_HARVEST;
                    step = 0;
                }
            }

        } return;
        case STATE_HARVEST:
        {
            ++sub_step;
            if(sub_step == HARVEST_WORK_STEPS)
            {
                ++step;
                sub_step = 0;

                // Reached new state?
                if(step == 6)
                {
                    // Add an empty pile as environmental object
                    gwg->SetNO(pos, new noEnvObject(pos, 40, 6), true);
                    em->AddToKillList(this);

                    // BQ drumrum neu berechnen
                    gwg->RecalcBQAroundPoint(pos);
                }
            }
        } return;
    }
}


noCharburnerPile::WareType noCharburnerPile::GetNeededWareType() const
{
    if(sub_step % 2 == 0)
        return WT_WOOD;
    else
        return WT_GRAIN;
}
