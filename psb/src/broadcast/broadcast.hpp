#ifndef __src__broadcast__broadcast__hpp
#define __src__broadcast__broadcast__hpp

// Includes

#include "broadcast.h"
#include "broadcast/broadcast.sponge.hpp"

namespace psb
{
    using namespace drop;

    // Configuration

    template <typename type> size_t broadcast <type> :: configuration :: sponge :: capacity = 262144;
    template <typename type> interval broadcast <type> :: configuration :: sponge :: timeout = 5_s;

    // Constructors

    template <typename type> broadcast <type> :: broadcast() : _arc(std :: make_shared <arc> ())
    {
    }

    // Private constructors

    template <typename type> broadcast <type> :: broadcast(const std :: shared_ptr <arc> & arc) : _arc(arc)
    {
    }

    // Methods

    template <typename type> void broadcast <type> :: publish(const class signer :: publickey & feed, const uint32_t & sequence, const type & payload, const signature & signature)
    {
        this->_arc->_sponge.push(this->_arc, {.feed = feed, .sequence = sequence, .payload = payload, .signature = signature});
    }

    // Private methods

    template <typename type> void broadcast <type> :: release(std :: vector <message> & messages)
    {
        std :: cout << "Releasing batch at " << now() << ":" << std :: endl;
        for(const auto & message : messages)
            std :: cout << "(" << message.feed << "/" << message.sequence << "): " << message.payload << std :: endl;
    }
};

#endif
