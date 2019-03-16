#ifndef __src__sampler__directory__hpp
#define __src__sampler__directory__hpp

// Includes

#include "directory.h"
#include "sampler.hpp"
#include "directory/directory.sampler.hpp"

namespace psb
{
    using namespace drop;

    // Static methods

    template <typename ctype> sampler <ctype> directory :: sample(const address & directory)
    {
        return psb :: sampler <ctype> :: template construct <sampler <ctype>> (directory);
    }
};

#endif
