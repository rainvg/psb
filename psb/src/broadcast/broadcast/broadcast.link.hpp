#ifndef __src__broadcast__broadcast__broadcast__link__hpp
#define __src__broadcast__broadcast__broadcast__link__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: link :: link(const connection & connection) : _connection(connection), _alive(true)
    {
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
    }

    // Private methods

    template <typename type> void broadcast <type> :: link :: shutdown(const std :: weak_ptr <arc> & warc, const std :: shared_ptr <link> & link)
    {
        this->_guard([&]()
        {
            this->_alive = false;
        });

        if(auto arc = warc.lock())
        {
            broadcast broadcast = arc;
            broadcast.unlink(link);
        }

        this->_pipe.post();
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
                std :: vector <blockid> pending;
                optional <class block> block;

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
                            pending.swap(this->_requests.pending);
                            this->_requests.local.insert(this->_requests.local.end(), pending.begin(), pending.end());
                        }
                        else if(this->_requests.remote.size())
                        {
                            struct blockid blockid = this->_requests.remote.front();
                            this->_requests.remote.pop_front();
                            block = broadcast.block(blockid);
                        }
                    });
                }
                else
                    exception <arc_expired> :: raise(this);

                if(announcements.size())
                {
                    for(const auto & announcement : announcements)
                        this->_blockmasks.local.push(announcement);

                    co_await this->_connection.send(announcements);
                    continue;
                }

                if(advertisements.size())
                {
                    auto offlist = this->_blockmasks.local.pop(advertisements);

                    co_await this->_connection.send(offlist);
                    continue;
                }

                if(pending.size())
                {
                    co_await this->_connection.send(pending);
                    continue;
                }

                if(block)
                {
                    std :: vector <message> messages;
                    messages.reserve(block.size());

                    for(uint32_t sequence; sequence < block.size(); sequence++)
                        messages.push_back(block[sequence]);

                    co_await this->_connection.send(messages);
                    continue;
                }

                this->_pipe.wait();
            }
        }
        catch(...)
        {
            this->shutdown(warc, link);
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
                            this->_blockmasks.remote.push(announcement);
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
                    }, [&](const std :: vector <message> & messages)
                    {
                        class block block = messages;
                        blockid blockid = this->_guard([&]()
                        {
                            if(this->_requests.local.size())
                            {
                                struct blockid blockid = this->_requests.local.front();
                                this->_requests.local.pop_front();
                                return blockid;
                            }
                            else
                                exception <ghost_request> :: raise(this);
                        });

                        broadcast.dispatch(blockid, block);
                    });
                }
                else
                    exception <arc_expired> :: raise(this);
            }
        }
        catch(...)
        {
            this->shutdown(warc, link);
        }
    }
};

#endif
