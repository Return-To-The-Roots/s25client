#ifndef NOALLIEDPUSH_H_INCLUDED
#define NOALLIEDPUSH_H_INCLUDED

#pragma once

#include "Addons.h"

///////////////////////////////////////////////////////////////////////////////
/**
 *  Addon stops allied players from pushing your borders back with new military buildings
 *
 *  @author PoC
 */
class AddonNoAlliedPush : public AddonBool
{
    public:
        AddonNoAlliedPush() : AddonBool(AddonId::NO_ALLIED_PUSH,
                                              ADDONGROUP_MILITARY,
                                              _("Improved Alliance"),
                                              _("Allied players can no longer push your borders back with new buildings."),
                                              0
                                             )
        {
        }
};

#endif // !NOALLIEDPUSH_H_INCLUDED
