#ifndef __src__broadcast__consistent__hpp
#define __src__broadcast__consistent__hpp

// Includes

#include "consistent.h"
#include "consistent/consistent.arc.hpp"
#include "consistent/consistent.verifier.hpp"
#include "consistent/consistent.structs.hpp"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> consistent <type> :: consistent(const sampler <channels> & sampler, const int & id) : _arc(std :: make_shared <arc> (sampler, id))
    {
        std :: weak_ptr <arc> warc = this->_arc;

        this->_arc->_broadcast.template on <deliver> ([=](const auto & batch)
        {
            if(auto arc = warc.lock())
            {
                consistent consistent = arc;
                consistent.dispatch(warc, batch);
            }
        });
    }

    // Private constructors

    template <typename type> consistent <type> :: consistent(const std :: shared_ptr <arc> & arc) : _arc(arc)
    {
    }

    // Private methods

    template <typename type> promise <void> consistent <type> :: dispatch(std :: weak_ptr <arc> warc, const typename broadcast <type> :: batch & batch)
    {
        offlist collisions;
        size_t sequence = 0;

        size_t cursor = 0;

        auto tampered = co_await verifier :: system.get().verify(batch);

        if(auto arc = warc.lock())
        {
            arc->_guard([&]()
            {
                class offlist offlist;

                for(const auto & block : batch.blocks)
                    for(const auto & message : block)
                    {
                        if(cursor >= tampered.size() || tampered[cursor] != sequence)
                        {
                            index index{.feed = message.feed, .sequence = message.sequence};
                            if((arc->_messages.find(index) != arc->_messages.end()) && arc->_messages[index] != message.payload)
                                offlist.add(sequence);
                            else
                                arc->_messages[index] = message.payload;
                        }
                        else
                            cursor++;

                        sequence++;
                    }

                this->_arc->_collisions[batch.info.hash] = offlist;
            });
        }
    }

    template <typename type> promise <void> consistent <type> :: serve(std :: weak_ptr <arc> warc, connection connection)
    {
        hash hash = co_await connection.receive <class hash> ();

        if(auto arc = warc.lock())
        {
            auto response = arc->_guard([&]() -> optional <offlist>
            {
                if(arc->_collisions.find(hash) != arc->_collisions.end())
                    return arc->_collisions[hash];

                if(arc->_subscribers.find(hash) == arc->_subscribers.end())
                    arc->_subscribers[hash] = std :: vector <subscriber> ();

                arc->_subscribers[hash].push_back({.connection = connection});

                return optional <offlist> ();
            });

            if(response)
                co_await connection.send(response);
        }
    }

    // Services

    template <typename type> promise <void> consistent <type> :: accept(std :: weak_ptr <arc> warc, sampler <channels> sampler)
    {
        while(true)
        {
            auto connection = co_await sampler.accept <echo> ();

            if(auto arc = warc.lock())
            {
                consistent consistent = arc;
                consistent.serve(warc, connection);
            }
            else
                break;
        }
    }

    template <typename type> promise <void> consistent <type> :: keepalive(std :: weak_ptr <arc> warc)
    {
        while(true)
        {
            co_await wait(settings :: keepalive :: interval);

            if(auto arc = warc.lock())
            {
                arc->_guard([&]()
                {
                    for(const auto & [hash, subscribers] : arc->_subscribers)
                        for(const auto & subscriber : subscribers)
                        {
                            try
                            {
                                subscriber.keepalive = subscriber.send(optional <offlist> ());
                            }
                            catch(...)
                            {
                            }
                        }
                });
            }
            else
                break;
        }
    }
};

#endif
