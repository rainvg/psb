#ifndef __src__sampler__sampler__hpp
#define __src__sampler__sampler__hpp

// Includes

#include "sampler.h"

namespace psb
{
    using namespace drop;

    // Constraints

    template <typename ctype> template <typename type, typename... atypes> constexpr bool sampler <ctype> :: constraints :: construct()
    {
        return parameters :: in <type, variant> :: value && std :: is_constructible <type, atypes...> :: value;
    }

    // Private constructors

    template <typename ctype> sampler <ctype> :: sampler(const std :: shared_ptr <variant> & arc) : _arc(arc)
    {
    }

    // Static methods

    template <typename ctype> template <typename type, typename... atypes, std :: enable_if_t <sampler <ctype> :: constraints :: template construct <type, atypes...> ()> *> sampler <ctype> sampler <ctype> :: construct(atypes && ... args)
    {
        std :: shared_ptr <variant> arc = std :: make_shared <variant> ();
        arc->template emplace <type> (std :: forward <atypes> (args)...);

        return sampler(arc);
    }
};

#endif
