#ifndef ADDONSTOPMETALWORKSONZERO_H_INCLUDED
#define ADDONSTOPMETALWORKSONZERO_H_INCLUDED

#include "Addons.h"

class AddonMetalworksBehaviorOnZero : public AddonList
{
public:
	AddonMetalworksBehaviorOnZero() : AddonList(AddonId::METALWORKSBEHAVIORONZERO,
		ADDONGROUP_GAMEPLAY,
		_("Change metalworks behavior on zero"),
		_("Change the working behavior of metalworks if all sliders in the tools window are set to zero.\n"
		             "Produce random ware: S2-Default\n"
		             "Produce nothing: RttR-Default"),
		0
		)
	{
		addOption(_("Produce random ware"));
		addOption(_("Produce nothing"));
	}
};

#endif
