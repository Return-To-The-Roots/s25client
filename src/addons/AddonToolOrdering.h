#ifndef ADDONTOOLORDERING_H_INCLUDED
#define ADDONTOOLORDERING_H_INCLUDED

#include "Addons.h"

class AddonToolOrdering : public AddonBool
{
    public:
        AddonToolOrdering() : AddonBool(AddonId::TOOL_ORDERING,
                                            ADDONGROUP_GAMEPLAY,
                                            _("Tool ordering"),
                                            _("Allows to order a specific amount of tools for priority production."),
                                            0
                                           )
        {
        }
};


#endif
