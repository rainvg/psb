#ifndef __src__broadcast__consistent__hpp
#define __src__broadcast__consistent__hpp

// Includes

#include "consistent.h"

namespace psb
{
    using namespace drop;

    // consistent

    // Constructors

    template <typename type> consistent <type> :: consistent(const sampler <channels> & sampler) : _arc(std :: make_shared <arc> (sampler))
    {
    }

    // arc

    // Constructors

    template <typename type> consistent <type> :: arc :: arc(const sampler <channels> & sampler) : _sampler(sampler), _broadcast(sampler)
    {
    }
};

#endif
