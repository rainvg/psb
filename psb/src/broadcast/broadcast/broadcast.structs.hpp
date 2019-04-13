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

    // blockid

    // Operators

    template <typename type> bool broadcast <type> :: blockid :: operator < (const blockid & rho) const
    {
        return (this->hash < rho.hash) || ((this->hash == rho.hash) && (this->sequence < rho.sequence));
    }

    template <typename type> bool broadcast <type> :: blockid :: operator == (const blockid & rho) const
    {
        return (this->hash == rho.hash) && (this->sequence == rho.sequence);
    }
};

#endif
