//
// Created by pavel on 10.06.25.
//

#include "WeightParams.h"
#include <boost/math/special_functions/sign.hpp>

#include <cmath>

double CALC::calcCount(unsigned x, BuildParams params)
{
    double log2_linear = fabs(params.logTwo.linear);
    double log2Val = std::max(0.0, std::log(params.logTwo.constant + log2_linear * x));
    return params.constant + params.linear * x + boost::math::sign(params.logTwo.linear) * log2Val;
}
