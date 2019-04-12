#ifndef __src__broadcast__broadcast__broadcast__structs__hpp
#define __src__broadcast__broadcast__broadcast__structs__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // batchinfo

    // Operators

    template <typename type> bool broadcast <type> :: batchinfo :: operator == (const batchinfo & rho) const
    {
        return (this->hash == rho.hash);
    }
};

#endif
