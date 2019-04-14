#ifndef __src__broadcast__broadcast__hpp
#define __src__broadcast__broadcast__hpp

// Includes

#include "broadcast.h"
#include "broadcast/broadcast.structs.hpp"
#include "broadcast/broadcast.block.hpp"
#include "broadcast/broadcast.sponge.hpp"
#include "broadcast/broadcast.batchset.hpp"
#include "broadcast/broadcast.blockmask.hpp"
#include "broadcast/broadcast.link.hpp"

namespace psb
{
    using namespace drop;

    // Configuration

    template <typename type> size_t broadcast <type> :: configuration :: sponge :: capacity = 256;
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

    template <typename type> void broadcast <type> :: link(const connection & connection)
    {
        auto link = std :: make_shared <class link> (connection);

        this->_arc->_guard([&]()
        {
            this->_arc->_links.insert(link);
        });

        link->start();
    }

    template <typename type> void broadcast <type> :: release(const std :: vector <block> & blocks)
    {
        std :: cout << "Releasing batch at " << now() << ":" << std :: endl;
        for(const auto & block : blocks)
        {
            for(size_t i = 0; i < block.size(); i++)
                std :: cout << "(" << block[i].feed << "/" << block[i].sequence << "): " << block[i].payload << std :: endl;
            std :: cout << std :: endl;
        }
    }
};

#endif
