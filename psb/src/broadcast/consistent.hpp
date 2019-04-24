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

        for(size_t server = 0; server < settings :: sample :: size; server++)
        {
            [](std :: weak_ptr <arc> warc, psb :: sampler <channels> sampler) -> promise <void>
            {
                auto connection = co_await sampler.connect <psb :: echo> ();

                if(auto arc = warc.lock())
                {
                    std :: shared_ptr <struct server> server(new (struct server){.connection = connection});

                    arc->_guard([&]()
                    {
                        arc->_servers.push_back(server);
                    });

                    consistent consistent = arc;

                    consistent.serversend(server);
                    consistent.serverreceive(arc, server);
                }
            }(warc, sampler);
        }
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

    template <typename type> void consistent <type> :: spot(std :: weak_ptr <arc> warc, const hash & batch)
    {
        {/*std :: cout << "Spotted " << hash << std :: endl;*/}

        auto handlers = this->_arc->_guard([&]()
        {
            this->_arc->_quorums[batch] = {.echoes = 0};

            for(const auto & server : this->_arc->_servers)
                server->queries.post(batch);

            return this->_arc->_handlers.spot;
        });

        for(const auto & handler : handlers)
            handler(batch);
    }

    template <typename type> promise <void> consistent <type> :: dispatch(std :: weak_ptr <arc> warc, typename broadcast <type> :: batch batch)
    {
        {/*std :: cout << "Dispatching batch " << batch.info.hash << std :: endl;*/}

        auto tampered = std :: vector <uint32_t> (); // co_await verifier :: system.get().verify(batch);

        {/*std :: cout << "Signatures verified." << std :: endl;*/}

        if(auto arc = warc.lock())
        {
            std :: vector <std :: weak_ptr <client>> subscriptions;
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

                {/*std :: cout << collisions.size() << " collisions found for " << batch.info.hash << std :: endl;*/}
                arc->_batches[batch.info.hash] = batch;
                {/*std :: cout << "Added batch " << batch.info.hash << std :: endl;*/}

                arc->_collisions[batch.info.hash] = collisions;

                if(arc->_subscriptions.find(batch.info.hash) != arc->_subscriptions.end())
                {
                    subscriptions.swap(arc->_subscriptions[batch.info.hash]);
                    arc->_subscriptions.erase(batch.info.hash);
                }
            });

            {/*std :: cout << "There are " << subscriptions.size() << " subscriptions." << std :: endl;*/}

            for(const auto & subscription : subscriptions)
            {
                if(auto client = subscription.lock())
                {
                    optional <echo> response = (struct echo){.batch = batch.info.hash, .collisions = collisions};
                    client->responses.post(response);
                }
            }

            consistent consistent = arc;
            consistent.check(batch.info.hash);
        }

        return promise <void> ();
    }

    template <typename type> void consistent <type> :: check(const hash & hash)
    {
        auto batch = this->_arc->_guard([&]() -> optional <typename broadcast <type> :: batch>
        {
            {/*std :: cout << "Checking " << hash << " : " << (this->_arc->_quorums.find(hash) != this->_arc->_quorums.end()) << " " << ((this->_arc->_quorums.find(hash) != this->_arc->_quorums.end()) && (this->_arc->_quorums[hash].echoes >= settings :: sample :: threshold)) << " " << (this->_arc->_batches.find(hash) != this->_arc->_batches.end()) << std :: endl;*/}
            if((this->_arc->_quorums.find(hash) != this->_arc->_quorums.end()) &&
               (this->_arc->_quorums[hash].echoes >= settings :: sample :: threshold) &&
               (this->_arc->_batches.find(hash) != this->_arc->_batches.end()))
            {
               this->_arc->_quorums.erase(hash);
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
                {/*std :: cout << "Connection incoming." << std :: endl;*/}

                std :: shared_ptr <struct client> client(new (struct client){.connection = connection});

                if(auto arc = warc.lock())
                {
                    arc->_guard([&]()
                    {
                        arc->_clients.push_back(client);
                    });

                    consistent consistent = arc;

                    consistent.clientsend(client);
                    consistent.clientreceive(arc, client);
                }
                else
                    break;
            }
            catch(...)
            {
                {/*std :: cout << "EXCEPTION: accept" << std :: endl;*/}
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
                    for(const auto & client : arc->_clients)
                        client->responses.post(optional <echo> ());
                });
            }
            else
                break;
        }
    }

    template <typename type> promise <void> consistent <type> :: clientsend(std :: weak_ptr <client> wclient)
    {
        try
        {
            while(true)
            {
                if(auto client = wclient.lock())
                {
                    optional <echo> response = co_await client->responses.wait();

                    if(response)
                        {/*std :: cout << "Sending response for " << (*response).batch << std :: endl;*/}

                    co_await client->connection.send(response);
                }
                else
                    break;
            }
        }
        catch(...)
        {
            {/*std :: cout << "EXCEPTION: clientsend" << std :: endl;*/}
        }
    }

    template <typename type> promise <void> consistent <type> :: clientreceive(std :: weak_ptr <arc> warc, std :: weak_ptr <client> wclient)
    {
        try
        {
            while(true)
            {
                if(auto client = wclient.lock())
                {
                    hash query = co_await client->connection.template receive <hash> ();

                    {/*std :: cout << "Received query for " << query << std :: endl;*/}

                    if(auto arc = warc.lock())
                    {
                        auto response = arc->_guard([&]() -> optional <echo>
                        {
                            if(arc->_collisions.find(query) != arc->_collisions.end())
                                return (struct echo){.batch = query, .collisions = arc->_collisions[query]};
                            else
                            {
                                if(arc->_subscriptions.find(query) == arc->_subscriptions.end())
                                    arc->_subscriptions[query] = std :: vector <std :: weak_ptr <class client>> ();

                                arc->_subscriptions[query].push_back(client);

                                return optional <echo> ();
                            }
                        });

                        if(response)
                            client->responses.post(response);
                    }
                }
                else
                    break;
            }
        }
        catch(...)
        {
            {/*std :: cout << "EXCEPTION: clientreceive" << std :: endl;*/}
        }
    }

    template <typename type> promise <void> consistent <type> :: serversend(std :: weak_ptr <server> wserver)
    {
        try
        {
            while(true)
            {
                if(auto server = wserver.lock())
                {
                    hash query = co_await server->queries.wait();

                    {/*std :: cout << "Sending query for " << query << std :: endl;*/}
                    co_await server->connection.send(query);
                }
                else
                    break;
            }
        }
        catch(...)
        {
            {/*std :: cout << "EXCEPTION: serversend" << std :: endl;*/}
        }
    }

    template <typename type> promise <void> consistent <type> :: serverreceive(std :: weak_ptr <arc> warc, std :: weak_ptr <server> wserver)
    {
        try
        {
            while(true)
            {
                if(auto server = wserver.lock())
                {
                    optional <echo> response = co_await server->connection.template receive <optional <echo>> ();


                    if(!response)
                        continue;

                    {/*std :: cout << "Received response for " << (*response).batch << std :: endl;*/}

                    if(!(*response).collisions.size())
                    {
                        if(auto arc = warc.lock())
                        {
                            bool check = arc->_guard([&]()
                            {
                                if(arc->_quorums.find((*response).batch) != arc->_quorums.end())
                                {
                                    arc->_quorums[(*response).batch].echoes++;
                                    {/*std :: cout << "Received " << arc->_quorums[(*response).batch].echoes << " responses for batch " << (*response).batch << std :: endl;*/}
                                    return true;
                                }
                                else
                                    return false;
                            });

                            if(check)
                            {
                                consistent consistent = arc;
                                consistent.check((*response).batch);
                            }
                        }
                        else
                            break;
                    }
                    else
                    {
                        std :: cerr << "Unimplemented: non-empty collision set." << std :: endl;
                        exit(1);
                    }
                }
                else
                    break;
            }
        }
        catch(...)
        {
            {/*std :: cout << "EXCEPTION: serverreceive" << std :: endl;*/}
        }
    }

};

#endif
