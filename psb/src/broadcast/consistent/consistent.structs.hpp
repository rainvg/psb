#ifndef __src__broadcast__consistent__consistent__structs__hpp
#define __src__broadcast__consistent__consistent__structs__hpp

// Includes

#include "../consistent.h"

namespace psb
{
    using namespace drop;

    // index

    template <typename type> bool consistent <type> :: index ::operator == (const index & rho) const
    {
        return (this->feed == rho.feed) && (this->sequence == rho.sequence);
    }
};

#endif
