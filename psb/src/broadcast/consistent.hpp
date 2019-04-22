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

        this->_arc->_broadcast.template on <class spot> ([=](const auto & hash)
        {
            if(auto arc = warc.lock())
            {
                consistent consistent = arc;
                consistent.spot(warc, hash);
            }
        });

        this->_arc->_broadcast.template on <class deliver> ([=](const auto & batch)
        {
            if(auto arc = warc.lock())
            {
                consistent consistent = arc;
                consistent.dispatch(warc, batch);
            }
        });

        this->accept(this->_arc, sampler);
        this->keepalive(this->_arc);
    }

    // Private constructors

    template <typename type> consistent <type> :: consistent(const std :: shared_ptr <arc> & arc) : _arc(arc)
    {
    }

    // Methods

    template <typename type> template <typename etype, std :: enable_if_t <std :: is_same <etype, spot> :: value> *> void consistent <type> :: on(const std :: function <void (const hash &)> & handler)
    {
        this->_arc->_guard([&]()
        {
            this->_arc->_handlers.spot.push_back(handler);
        });
    }

    template <typename type> template <typename etype, std :: enable_if_t <std :: is_same <etype, deliver> :: value> *> void consistent <type> :: on(const std :: function <void (const typename broadcast <type> :: batch &)> & handler)
    {
        this->_arc->_guard([&]()
        {
            this->_arc->_handlers.deliver.push_back(handler);
        });
    }

    template <typename type> void consistent <type> :: publish(const class signer :: publickey & feed, const uint32_t & sequence, const type & payload, const signature & signature)
    {
        this->_arc->_broadcast.publish(feed, sequence, payload, signature);
    }

    // Private methods

    template <typename type> void consistent <type> :: spot(std :: weak_ptr <arc> warc, const hash & hash)
    {
        std :: cout << "Spotted " << hash << std :: endl;

        this->_arc->_guard([&]()
        {
            this->_arc->_echoes[hash] = {.echoes = 0};
        });

        for(size_t peer = 0; peer < settings :: sample :: size; peer++)
        {
            [&](sampler <channels> sampler) -> promise <void>
            {
                struct
                {
                    std :: weak_ptr <arc> warc;
                    class hash hash;
                } local {.warc = warc, .hash = hash};

                try
                {
                    std :: cout << "Establishing connection." << std :: endl;

                    auto connection = co_await sampler.connect <psb :: echo> ();

                    std :: cout << "Connection established." << std :: endl;

                    co_await connection.send(local.hash);

                    std :: cout << "Hash sent." << std :: endl;

                    while(true)
                    {
                        auto collisions = co_await connection.template receive <optional <offlist>> ();

                        if(collisions)
                        {
                            std :: cout << "Response obtained." << std :: endl;

                            if(!(*collisions).size())
                            {
                                if(auto arc = local.warc.lock())
                                {
                                    bool check = arc->_guard([&]()
                                    {
                                        if(arc->_echoes.find(local.hash) != arc->_echoes.end())
                                        {
                                            arc->_echoes[local.hash].echoes++;
                                            return true;
                                        }
                                        else
                                            return false;
                                    });

                                    if(check)
                                    {
                                        consistent consistent = arc;
                                        consistent.check(local.hash);
                                    }
                                }

                                break;
                            }
                            else
                            {
                                std :: cerr << "Unimplemented: non-empty collision set." << std :: endl;
                                exit(1);
                            }
                        }
                    }
                }
                catch(const exception <> & e)
                {
                    std :: cout << "Exception: " << e.what() << std :: endl;

                    try
                    {
                        e.details();
                    }
                    catch(const int & err)
                    {
                        std :: cout << "Errno: " << err << std :: endl;
                    }
                }
            }(this->_arc->_sampler);
        }
    }

    template <typename type> promise <void> consistent <type> :: dispatch(std :: weak_ptr <arc> warc, typename broadcast <type> :: batch batch)
    {
        std :: cout << "Dispatching batch " << batch.info.hash << std :: endl;

        auto tampered = co_await verifier :: system.get().verify(batch);

        std :: cout << "Signatures verified." << std :: endl;

        if(auto arc = warc.lock())
        {
            std :: vector <subscriber> subscribers;
            offlist collisions;

            arc->_guard([&]()
            {
                size_t sequence = 0;
                size_t cursor = 0;

                for(const auto & block : batch.blocks)
                    for(const auto & message : block)
                    {
                        if(cursor >= tampered.size() || tampered[cursor] != sequence)
                        {
                            index index{.feed = message.feed, .sequence = message.sequence};
                            if((arc->_messages.find(index) != arc->_messages.end()) && arc->_messages[index] != message.payload)
                                collisions.add(sequence);
                            else
                                arc->_messages[index] = message.payload;
                        }
                        else
                            cursor++;

                        sequence++;
                    }

                std :: cout << collisions.size() << " collisions found." << std :: endl;

                arc->_batches[batch.info.hash] = batch;
                arc->_collisions[batch.info.hash] = collisions;

                if(arc->_subscribers.find(batch.info.hash) != arc->_subscribers.end())
                {
                    subscribers.swap(arc->_subscribers[batch.info.hash]);
                    arc->_subscribers.erase(batch.info.hash);
                }
            });

            std :: cout << "There are " << subscribers.size() << " subscribers." << std :: endl;

            for(const auto & subscriber : subscribers)
            {
                [&]() -> promise <void>
                {
                    auto connection = subscriber.connection;
                    auto keepalive = subscriber.keepalive;
                    auto response = collisions;

                    try
                    {
                        if(keepalive)
                            co_await *keepalive;

                        std :: cout << "Sending collisions." << std :: endl;

                        co_await connection.template send <optional <offlist>> (response);
                        co_await wait(1_s);
                    }
                    catch(...)
                    {
                    }
                }();
            }

            consistent consistent = arc;
            consistent.check(batch.info.hash);
        }
    }

    template <typename type> promise <void> consistent <type> :: serve(std :: weak_ptr <arc> warc, connection connection)
    {
        try
        {
            hash hash = co_await connection.receive <class hash> ();

            std :: cout << "Received subscribe for " << hash << std :: endl;

            if(auto arc = warc.lock())
            {
                auto response = arc->_guard([&]() -> optional <offlist>
                {
                    if(arc->_collisions.find(hash) != arc->_collisions.end())
                    {
                        std :: cout << "Collisions ready." << std :: endl;
                        return arc->_collisions[hash];
                    }

                    if(arc->_subscribers.find(hash) == arc->_subscribers.end())
                        arc->_subscribers[hash] = std :: vector <subscriber> ();

                    std :: cout << "Pushing subscriber." << std :: endl;

                    arc->_subscribers[hash].push_back({.connection = connection});

                    return optional <offlist> ();
                });

                if(response)
                {
                    std :: cout << "Sending response." << std :: endl;
                    co_await connection.send(response);
                    co_await wait(1_s); // TODO: check if really necessary
                }
            }
        }
        catch(const exception <> & e)
        {
            std :: cout << "Exception: " << e.what() << std :: endl;

            try
            {
                e.details();
            }
            catch(const int & err)
            {
                std :: cout << "Errno: " << err << std :: endl;
            }
        }
    }

    template <typename type> void consistent <type> :: check(const hash & hash)
    {
        auto batch = this->_arc->_guard([&]() -> optional <typename broadcast <type> :: batch>
        {
            if((this->_arc->_echoes.find(hash) != this->_arc->_echoes.end()) &&
               (this->_arc->_echoes[hash].echoes >= settings :: sample :: threshold) &&
               (this->_arc->_batches.find(hash) != this->_arc->_batches.end()))
            {
               this->_arc->_echoes.erase(hash);
               return this->_arc->_batches[hash];
            }
            else
                return optional <typename broadcast <type> :: batch> ();
        });

        if(batch)
            this->deliver(*batch);
    }

    template <typename type> void consistent <type> :: deliver(const typename broadcast <type> :: batch & batch)
    {
        auto handlers = this->_arc->_guard([&]()
        {
            return this->_arc->_handlers.deliver;
        });

        for(const auto & handler : handlers)
            handler(batch);
    }

    // Services

    template <typename type> promise <void> consistent <type> :: accept(std :: weak_ptr <arc> warc, sampler <channels> sampler)
    {
        while(true)
        {
            try
            {
                auto connection = co_await sampler.accept <psb :: echo> ();

                std :: cout << "Connection incoming." << std :: endl;

                if(auto arc = warc.lock())
                {
                    consistent consistent = arc;
                    consistent.serve(warc, connection);
                }
                else
                    break;
            }
            catch(...)
            {
            }
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
                    for(auto & [hash, subscribers] : arc->_subscribers)
                        for(auto & subscriber : subscribers)
                        {
                            try
                            {
                                subscriber.keepalive = subscriber.connection.send(optional <offlist> ());
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
