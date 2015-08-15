#ifndef ADDONSTOPMETALWORKSONZERO_H_INCLUDED
#define ADDONSTOPMETALWORKSONZERO_H_INCLUDED

#include "Addons.h"

class AddonMetalworksBehaviorOnZero : public AddonList
{
public:
	AddonMetalworksBehaviorOnZero() : AddonList(ADDON_METALWORKSBEHAVIORONZERO,
		ADDONGROUP_GAMEPLAY,
		gettext_noop("Change metalworks behavior on zero"),
		gettext_noop("Change the working behavior of metalworks if all sliders in the tools window are set to zero.\n"
		             "Produce random ware: S2-Default\n"
		             "Produce nothing: RttR-Default"),
		1
		)
	{
		addOption(gettext_noop("Produce random ware"));
		addOption(gettext_noop("Produce nothing"));
	}
};

#endif
