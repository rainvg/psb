#ifndef __src__broadcast__secure__secure__structs__hpp
#define __src__broadcast__secure__secure__structs__hpp

// Includes

#include "../secure.h"

namespace psb
{
    using namespace drop;

    // index

    template <typename type> bool secure <type> :: index ::operator == (const index & rho) const
    {
        return (this->feed == rho.feed) && (this->sequence == rho.sequence);
    }
};

#endif
