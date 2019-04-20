#ifndef __src__broadcast__broadcast__broadcast__link__hpp
#define __src__broadcast__broadcast__broadcast__link__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: link :: link(const connection & connection, const int & id) : _id(id), _chrono{.latency = 0, .keepalive = now()}, _connection(connection), _alive(true)
    {
    }

    // Getters

    template <typename type> int broadcast <type> :: link :: id() const
    {
        return this->_id;
    }

    template <typename type> size_t broadcast <type> :: link :: requests()
    {
        return this->_guard([&]()
        {
            return this->_requests.pending.size() + this->_requests.local.size();
        });
    }

    template <typename type> interval broadcast <type> :: link :: latency()
    {
        return this->_guard([&]()
        {
            return this->_chrono.latency;
        });
    }

    // Methods

    template <typename type> void broadcast <type> :: link :: announce(const announcement & announcement)
    {
        bool post = this->_guard([&]()
        {
            if(this->_alive)
                this->_announcements.push_back(announcement);

            return this->_alive;
        });

        if(post)
            this->_pipe.post();
    }

    template <typename type> void broadcast <type> :: link :: advertise(const blockid & block)
    {
        bool post = this->_guard([&]()
        {
            if(this->_alive)
                this->_advertisements.insert(block);

            return this->_alive;
        });

        if(post)
            this->_pipe.post();
    }

    template <typename type> void broadcast <type> :: link :: request(const blockid & block)
    {
        this->_guard([&]()
        {
            if(!(this->_alive))
                exception <dead_link> :: raise(this);

            this->_requests.pending.push_back(block);
        });

        this->_pipe.post();
    }

    template <typename type> promise <std :: vector <typename broadcast <type> :: batchinfo>> broadcast <type> :: link :: sync(std :: weak_ptr <arc> warc, std :: shared_ptr <link> link)
    {
        if(this->_connection.tiebreak())
        {
            optional <typename syncset <batchinfo> :: round> round;

            if(auto arc = warc.lock())
            {
                broadcast broadcast = arc;
                round = broadcast.delivered().sync();
            }

            if(!round)
                exception <arc_expired> :: raise(this);

            co_await this->_connection.send((*round).view);
        }

        std :: vector <batchinfo> add;

        while(true)
        {
            auto view = co_await this->_connection.template receive <typename syncset <batchinfo> :: view> ();

            if(view.size() == 0)
                break;

            optional <typename syncset <batchinfo> :: round> round;

            if(auto arc = warc.lock())
            {
                broadcast broadcast = arc;
                round = broadcast.delivered().sync(view);
            }

            if(!round)
                exception <arc_expired> :: raise(this);

            add.insert(add.end(), (*round).add.begin(), (*round).add.end());
            co_await this->_connection.send((*round).view);

            if((*round).view.size() == 0)
                break;
        }

        co_return add;
    }

    template <typename type> void broadcast <type> :: link :: start(const std :: weak_ptr <arc> & warc, const std :: shared_ptr <link> & link)
    {
        this->send(warc, link);
        this->receive(warc, link);
        this->keepalive(link);
    }

    template <typename type> void broadcast <type> :: link :: shutdown()
    {
        this->_guard([&]()
        {
            this->_alive = false;
        });

        this->_pipe.post();
    }

    // Private methods

    template <typename type> void broadcast <type> :: link :: unlink(const std :: weak_ptr <arc> & warc, const std :: shared_ptr <link> & link)
    {
        if(auto arc = warc.lock())
        {
            broadcast broadcast = arc;
            broadcast.unlink(link);
        }
    }

    // Services

    template <typename type> promise <void> broadcast <type> :: link :: send(std :: weak_ptr <arc> warc, std :: shared_ptr <link> link)
    {
        try
        {
            while(this->_guard([&](){return this->_alive;}))
            {
                std :: vector <announcement> announcements;
                std :: unordered_set <blockid, shorthash> advertisements;
                std :: vector <blockid> requests;
                variant <blockid, payload> payload;

                if(auto arc = warc.lock())
                {
                    broadcast broadcast = arc;

                    this->_guard([&]()
                    {
                        if(this->_announcements.size())
                            announcements.swap(this->_announcements);
                        else if(this->_advertisements.size())
                            advertisements.swap(this->_advertisements);
                        else if(this->_requests.pending.size())
                        {
                            requests.swap(this->_requests.pending);

                            if(!(this->_requests.local.size()))
                                this->_chrono.last = now();

                            this->_requests.local.insert(this->_requests.local.end(), requests.begin(), requests.end());
                        }
                        else if(this->_requests.remote.size())
                        {
                            payload = this->_requests.remote.front();
                            this->_requests.remote.pop_front();

                        }
                    });

                    if(payload)
                    {
                        blockid blockid = payload.template reinterpret <class blockid> ();
                        payload = (class payload){.proof = broadcast.proof(blockid.hash)};

                        class block block = broadcast.block(blockid);
                        payload.template reinterpret <class payload> ().messages.reserve(block.size());

                        for(const auto & message : block)
                            payload.template reinterpret <class payload> ().messages.push_back(message);
                    }
                }
                else
                    exception <arc_expired> :: raise(this);

                if(announcements.size())
                {
                    for(const auto & announcement : announcements)
                        this->_blockmasks.local.push(announcement.batch);

                    co_await this->_connection.template send <transaction> (announcements);
                    continue;
                }

                if(advertisements.size())
                {
                    auto offlist = this->_blockmasks.local.pop(advertisements);

                    co_await this->_connection.template send <transaction> (offlist);
                    continue;
                }

                if(requests.size())
                {
                    co_await this->_connection.template send <transaction> (requests);
                    continue;
                }

                if(payload)
                {
                    co_await this->_connection.template send <transaction> (payload.template reinterpret <class payload> ());
                    continue;
                }

                timestamp now = drop :: now();
                if(now - this->_chrono.keepalive > settings :: link :: keepalive)
                {
                    this->_chrono.keepalive = now;
                    co_await this->_connection.template send <transaction> ({});
                }

                co_await this->_pipe.wait();
            }
        }
        catch(...)
        {
            this->shutdown();
            this->unlink(warc, link);
        }
    }

    template <typename type> promise <void> broadcast <type> :: link :: receive(std :: weak_ptr <arc> warc, std :: shared_ptr <link> link)
    {
        try
        {
            while(this->_guard([&](){return this->_alive;}))
            {
                auto transaction = co_await this->_connection.template receive <link :: transaction> ();

                if(auto arc = warc.lock())
                {
                    broadcast broadcast = arc;

                    transaction.match([&](const std :: vector <announcement> & announcements)
                    {
                        for(const auto & announcement : announcements)
                        {
                            this->_blockmasks.remote.push(announcement.batch);
                            broadcast.spot(announcement.batch);

                            if(announcement.available)
                                broadcast.available(announcement.batch.hash, link);
                        }
                    }, [&](const offlist & advertisements)
                    {
                        for(const auto & advertisement : this->_blockmasks.remote.pop(advertisements))
                            broadcast.available(advertisement, link);
                    }, [&](const std :: vector <blockid> & requests)
                    {
                        this->_guard([&]()
                        {
                            this->_requests.remote.insert(this->_requests.remote.end(), requests.begin(), requests.end());
                        });

                        this->_pipe.post();
                    }, [&](const payload & payload)
                    {
                        blockid blockid;

                        this->_guard([&]()
                        {
                            if(this->_requests.local.size())
                            {
                                blockid = this->_requests.local.front();
                                this->_requests.local.pop_front();
                            }
                            else
                                exception <ghost_request> :: raise(this);

                            interval latency = now() - this->_chrono.last;

                            if(this->_chrono.latency == 0)
                                this->_chrono.latency = latency;
                            else
                                this->_chrono.latency = (uint64_t)(((double)(uint64_t)(this->_chrono.latency) + configuration :: link :: lambda * (double)(uint64_t) latency) / (1. + configuration :: link :: lambda));

                            this->_chrono.last = now();
                        });

                        if(hash(payload.proof) != blockid.hash)
                            exception <malformed_block, hash_mismatch> :: raise(this);

                        hash :: state hasher;
                        for(const auto & message : payload.messages)
                            hasher.update(message);

                        if(hasher.finalize() != payload.proof[blockid.sequence])
                            exception <malformed_block, hash_mismatch> :: raise(this);

                        broadcast.dispatch(blockid, payload.proof, payload.messages, link);
                    });
                }
                else
                    exception <arc_expired> :: raise(this);
            }
        }
        catch(...)
        {
            this->shutdown();
            this->unlink(warc, link);
        }
    }

    template <typename type> promise <void> broadcast <type> :: link :: keepalive(std :: shared_ptr <link> link)
    {
        while(this->_guard([&](){return this->_alive;}))
        {
            co_await wait(settings :: link :: keepalive);
            this->_pipe.post();
        }
    }
};

#endif
