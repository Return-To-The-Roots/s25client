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
        AddonNoAlliedPush() : AddonBool(ADDON_NO_ALLIED_PUSH,
                                              ADDONGROUP_MILITARY,
                                              gettext_noop("Improved Alliance"),
                                              gettext_noop("Allied players can no longer push\n"
                                                      "your borders back with new buildings."),
                                              0
                                             )
        {
        }
};

#endif // !NOALLIEDPUSH_H_INCLUDED
