#ifndef __src__broadcast__consistent__consistent__arc__hpp
#define __src__broadcast__consistent__consistent__arc__hpp

// Includes

#include "../consistent.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> consistent <type> :: arc :: arc(const sampler <channels> & sampler, const int & id) : _sampler(sampler), _broadcast(sampler, id)
    {
    }
}

#endif
