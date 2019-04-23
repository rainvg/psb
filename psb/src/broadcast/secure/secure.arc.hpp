#ifndef __src__broadcast__secure__secure__arc__hpp
#define __src__broadcast__secure__secure__arc__hpp

// Includes

#include "../secure.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> secure <type> :: arc :: arc(const sampler <channels> & sampler, const int & id) : _sampler(sampler), _consistent(sampler, id)
    {
    }
}

#endif
