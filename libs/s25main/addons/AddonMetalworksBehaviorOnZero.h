#ifndef ADDONSTOPMETALWORKSONZERO_H_INCLUDED
#define ADDONSTOPMETALWORKSONZERO_H_INCLUDED

#include "AddonList.h"

class AddonMetalworksBehaviorOnZero : public AddonList
{
public:
    AddonMetalworksBehaviorOnZero()
        : AddonList(AddonId::METALWORKSBEHAVIORONZERO, AddonGroup::GamePlay | AddonGroup::Economy, _("Change metalworks behavior on zero"),
                    _("Change the working behavior of metalworks if all sliders in the tools window are set to zero.\n"
                      "Produce random ware: S2-Default\n"
                      "Produce nothing: RttR-Default"),
                    {
                      _("Produce random ware"),
                      _("Produce nothing"),
                    })
    {}
};

#endif
