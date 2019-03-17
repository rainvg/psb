#ifndef __src__sampler__sampler__hpp
#define __src__sampler__sampler__hpp

// Includes

#include "sampler.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename ctype> template <typename type, std :: enable_if_t <parameters :: in <type, typename sampler <ctype> :: variant> :: value> *> sampler <ctype> :: sampler(const type & sampler) : _sampler(sampler)
    {
    }
};

#endif
