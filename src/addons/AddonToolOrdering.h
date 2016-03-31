#ifndef ADDONTOOLORDERING_H_INCLUDED
#define ADDONTOOLORDERING_H_INCLUDED

#include "Addons.h"

class AddonToolOrdering : public AddonBool
{
    public:
        AddonToolOrdering() : AddonBool(ADDON_TOOL_ORDERING,
                                            ADDONGROUP_GAMEPLAY,
                                            gettext_noop("Tool ordering"),
                                            gettext_noop("Allows to order a specific amount of tools for priority production."),
                                            0
                                           )
        {
        }
};


#endif
