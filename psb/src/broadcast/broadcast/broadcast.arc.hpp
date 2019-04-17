#ifndef __src__broadcast__broadcast__broadcast__arc__hpp
#define __src__broadcast__broadcast__broadcast__arc__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: arc :: arc() : _handshakes{.fast = 0, .secure = 0}
    {
    }
};

#endif
