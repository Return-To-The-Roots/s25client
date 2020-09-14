#pragma once

#include "AddonBool.h"

class AddonToolOrdering : public AddonBool
{
public:
    AddonToolOrdering()
        : AddonBool(AddonId::TOOL_ORDERING, AddonGroup::GamePlay, _("Tool ordering"),
                    _("Allows to order a specific amount of tools for priority production."))
    {}
};
