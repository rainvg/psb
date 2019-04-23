#ifndef __src__broadcast__secure__hpp
#define __src__broadcast__secure__hpp

// Includes

#include "secure.h"
#include "secure/secure.arc.hpp"
#include "secure/secure.structs.hpp"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> secure <type> :: secure(const sampler <channels> & sampler, const int & id) : _arc(std :: make_shared <arc> (sampler, id))
    {
        std :: weak_ptr <arc> warc = this->_arc;

        this->_arc->_consistent.template on <class spot> ([=](const auto & hash)
        {
            if(auto arc = warc.lock())
            {
                secure secure = arc;
                secure.spot(warc, hash);
            }
        });

        this->_arc->_consistent.template on <class deliver> ([=](const auto & batch)
        {
            if(auto arc = warc.lock())
            {
                secure secure = arc;
                secure.dispatch(warc, batch);
            }
        });

        this->accept(this->_arc, sampler);
        this->keepalive(this->_arc);

        for(size_t server = 0; server < settings :: sample :: size; server++)
        {
            [](std :: weak_ptr <arc> warc, psb :: sampler <channels> sampler) -> promise <void>
            {
                auto connection = co_await sampler.connect <psb :: ready> ();

                if(auto arc = warc.lock())
                {
                    std :: shared_ptr <struct server> server(new (struct server){.connection = connection});

                    arc->_guard([&]()
                    {
                        arc->_servers.push_back(server);
                    });

                    secure secure = arc;

                    {/*std :: cout << "Starting services" << std :: endl;*/}

                    secure.serversend(server);
                    secure.serverreceive(arc, server);
                }
            }(warc, sampler);
        }
    }

    // Private constructors

    template <typename type> secure <type> :: secure(const std :: shared_ptr <arc> & arc) : _arc(arc)
    {
    }

    // Methods

    template <typename type> template <typename etype, std :: enable_if_t <std :: is_same <etype, spot> :: value> *> void secure <type> :: on(const std :: function <void (const hash &)> & handler)
    {
        this->_arc->_guard([&]()
        {
            this->_arc->_handlers.spot.push_back(handler);
        });
    }

    template <typename type> template <typename etype, std :: enable_if_t <std :: is_same <etype, deliver> :: value> *> void secure <type> :: on(const std :: function <void (const typename broadcast <type> :: batch &)> & handler)
    {
        this->_arc->_guard([&]()
        {
            this->_arc->_handlers.deliver.push_back(handler);
        });
    }

    template <typename type> void secure <type> :: publish(const class signer :: publickey & feed, const uint32_t & sequence, const type & payload, const signature & signature)
    {
        this->_arc->_consistent.publish(feed, sequence, payload, signature);
    }

    // Private methods

    template <typename type> void secure <type> :: spot(std :: weak_ptr <arc> warc, const hash & batch)
    {
        {/*std :: cout << "Spotted " << batch << std :: endl;*/}

        auto handlers = this->_arc->_guard([&]()
        {
            this->_arc->_quorums[batch] = 0;

            for(const auto & server : this->_arc->_servers)
                server->queries.post(batch);

            return this->_arc->_handlers.spot;
        });

        for(const auto & handler : handlers)
            handler(batch);
    }

    template <typename type> void secure <type> :: dispatch(std :: weak_ptr <arc> warc, typename broadcast <type> :: batch batch)
    {
        {/*std :: cout << "Dispatching batch " << batch.info.hash << std :: endl;*/}

        if(auto arc = warc.lock())
        {
            std :: vector <std :: weak_ptr <client>> subscriptions;

            arc->_guard([&]()
            {
                arc->_batches[batch.info.hash] = batch;
                {/*std :: cout << "Added batch " << batch.info.hash << std :: endl;*/}

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
                    optional <hash> response = batch.info.hash;
                    client->responses.post(response);
                }
            }

            secure secure = arc;
            secure.check(batch.info.hash);
        }
    }

    template <typename type> void secure <type> :: check(const hash & hash)
    {
        auto batch = this->_arc->_guard([&]() -> optional <typename broadcast <type> :: batch>
        {
            if((this->_arc->_quorums.find(hash) != this->_arc->_quorums.end()) &&
               (this->_arc->_quorums[hash] >= settings :: sample :: threshold) &&
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

    template <typename type> void secure <type> :: deliver(const typename broadcast <type> :: batch & batch)
    {
        auto handlers = this->_arc->_guard([&]()
        {
            return this->_arc->_handlers.deliver;
        });

        for(const auto & handler : handlers)
            handler(batch);
    }

    // Services

    template <typename type> promise <void> secure <type> :: accept(std :: weak_ptr <arc> warc, sampler <channels> sampler)
    {
        while(true)
        {
            try
            {
                auto connection = co_await sampler.accept <psb :: ready> ();
                {/*std :: cout << "Connection incoming." << std :: endl;*/}

                std :: shared_ptr <struct client> client(new (struct client){.connection = connection});

                if(auto arc = warc.lock())
                {
                    arc->_guard([&]()
                    {
                        arc->_clients.push_back(client);
                    });

                    secure secure = arc;

                    secure.clientsend(client);
                    secure.clientreceive(arc, client);
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

    template <typename type> promise <void> secure <type> :: keepalive(std :: weak_ptr <arc> warc)
    {
        while(true)
        {
            co_await wait(settings :: keepalive :: interval);

            if(auto arc = warc.lock())
            {
                arc->_guard([&]()
                {
                    for(const auto & client : arc->_clients)
                        client->responses.post(optional <hash> ());
                });
            }
            else
                break;
        }
    }

    template <typename type> promise <void> secure <type> :: clientsend(std :: weak_ptr <client> wclient)
    {
        try
        {
            while(true)
            {
                if(auto client = wclient.lock())
                {
                    {/*std :: cout << "Inside clientsend: waiting pipe" << std :: endl;*/}
                    optional <hash> response = co_await client->responses.wait();

                    if(response)
                        {/*std :: cout << "Sending response for " << (*response) << std :: endl;*/}

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

    template <typename type> promise <void> secure <type> :: clientreceive(std :: weak_ptr <arc> warc, std :: weak_ptr <client> wclient)
    {
        try
        {
            while(true)
            {
                if(auto client = wclient.lock())
                {
                    {/*std :: cout << "Inside clientreceive: receiving" << std :: endl;*/}
                    hash query = co_await client->connection.template receive <hash> ();

                    {/*std :: cout << "Received query for " << query << std :: endl;*/}

                    if(auto arc = warc.lock())
                    {
                        auto response = arc->_guard([&]()
                        {
                            if(arc->_batches.find(query) != arc->_batches.end())
                                return true;
                            else
                            {
                                if(arc->_subscriptions.find(query) == arc->_subscriptions.end())
                                    arc->_subscriptions[query] = std :: vector <std :: weak_ptr <class client>> ();

                                arc->_subscriptions[query].push_back(client);

                                return false;
                            }
                        });

                        if(response)
                            client->responses.post(optional <hash> (query));
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

    template <typename type> promise <void> secure <type> :: serversend(std :: weak_ptr <server> wserver)
    {
        try
        {
            while(true)
            {
                if(auto server = wserver.lock())
                {
                    {/*std :: cout << "Inside serversend: waiting pipe" << std :: endl;*/}
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

    template <typename type> promise <void> secure <type> :: serverreceive(std :: weak_ptr <arc> warc, std :: weak_ptr <server> wserver)
    {
        try
        {
            while(true)
            {
                if(auto server = wserver.lock())
                {
                    {/*std :: cout << "Inside serverreceive: receiving" << std :: endl;*/}
                    optional <hash> response = co_await server->connection.template receive <optional <hash>> ();

                    if(!response)
                        continue;

                    {/*std :: cout << "Received response for " << *response << std :: endl;*/}

                    if(auto arc = warc.lock())
                    {
                        bool check = arc->_guard([&]()
                        {
                            if(arc->_quorums.find(*response) != arc->_quorums.end())
                            {
                                arc->_quorums[*response]++;
                                {/*std :: cout << "Received " << arc->_quorums[*response] << " responses for batch " << *response << std :: endl;*/}
                                return true;
                            }
                            else
                                return false;
                        });

                        if(check)
                        {
                            secure secure = arc;
                            secure.check(*response);
                        }
                    }
                    else
                        break;
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
