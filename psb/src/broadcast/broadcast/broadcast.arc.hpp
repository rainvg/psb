#ifndef __src__broadcast__broadcast__broadcast__arc__hpp
#define __src__broadcast__broadcast__broadcast__arc__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: arc :: arc(const sampler <channels> & sampler, const int & id) : _id(id), _handshakes{.fast = 0, .secure = 0}, _churn{.trigger = 0}, _sampler(sampler)
    {
    }
};

#endif
