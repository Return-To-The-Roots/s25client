#ifndef ADDONSHOWRESOURCES_H_INCLUDED
#define ADDONSHOWRESOURCES_H_INCLUDED

#pragma once

#include "AddonBool.h"
#include "mygettext/mygettext.h"

/**
 *  addon shows statstic about remaining resources of mines and wells.
 */
class AddonShowResources : public AddonBool
{
public:
	AddonShowResources()
        : AddonBool(AddonId::SHOWRESOURCES, ADDONGROUP_ECONOMY, _("Show resources"),
                    _("Show the amount of resources which a mine or a well has left."), 0)
    {}
};

#endif
