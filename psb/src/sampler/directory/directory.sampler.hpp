#ifndef __src__sampler__directory__directory__sampler__hpp
#define __src__sampler__directory__directory__sampler__hpp

#include "../directory.h"

namespace psb
{
    using namespace drop;

    // sampler

    // Constructors

    template <typename ctype> directory :: sampler <ctype> :: sampler(const address & directory) : _arc(std :: make_shared <arc> (directory))
    {
        this->_arc->_listener.template set <timeouts :: accept> (settings :: timeouts :: network);

        this->listen(this->_arc);
        this->keepalive(this->_arc);
    }

    // Methods

    template <typename ctype> template <ctype channel> promise <connection> directory :: sampler <ctype> :: connectunbiased() const
    {
        while(true)
        {
            try
            {
                auto member = this->_arc->_guard([&]()
                {
                    return this->_arc->_membership.pick();
                });

                auto connection = co_await tcp :: connect(member.address); // TODO: Add timeout here.

                connection.template set <timeouts :: send> (settings :: timeouts :: network);
                connection.template set <timeouts :: receive> (settings :: timeouts :: network);

                co_await connection.template secure <client> (this->_arc->_keyexchanger, member.publickey);
                co_await connection.template send(uint8_t(channel));

                if(co_await connection.template receive <bool> ())
                    co_return connection;
            }
            catch(...)
            {
            }

            co_await wait(settings :: intervals :: retry);
        }
    }

    template <typename ctype> template <ctype channel> promise <connection> directory :: sampler <ctype> :: connectbiased() const
    {
        return this->connectunbiased <channel> ();
    }

    template <typename ctype> template <ctype channel> promise <connection> directory :: sampler <ctype> :: accept() const
    {
        promise <connection> acceptor;

        this->_arc->_guard([&]()
        {
            if(this->_arc->_acceptors.find(uint8_t(channel)) != this->_arc->_acceptors.end())
                exception <channel_busy> :: raise(this);

            this->_arc->_acceptors[uint8_t(channel)] = acceptor;
        });

        return acceptor;
    }

    // Private methods

    template <typename ctype> promise <void> directory :: sampler <ctype> :: serve(connection connection, std :: weak_ptr <arc> warc) const
    {
        if(auto arc = warc.lock())
        {
            try
            {
                co_await connection.secure <server> (arc->_keyexchanger);
                auto channel = co_await connection.receive <uint8_t> ();

                optional <promise <class connection>> acceptor = arc->_guard([&]() -> optional <promise <class connection>>
                {
                    if(arc->_acceptors.find(channel) != arc->_acceptors.end())
                    {
                        promise <class connection> acceptor = arc->_acceptors[channel];
                        arc->_acceptors.erase(channel);
                        return acceptor;
                    }

                    return optional <promise <class connection>> ();
                });

                co_await connection.send <bool> (acceptor);

                if(acceptor)
                    (*acceptor).resolve(connection);
            }
            catch(...)
            {
            }
        }
    }

    template <typename ctype> promise <void> directory :: sampler <ctype> :: listen(std :: weak_ptr <arc> warc) const
    {
        while(true)
        {
            if(auto arc = warc.lock())
            {
                try
                {
                    auto connection = co_await arc->_listener.accept();
                    this->serve(connection, warc);
                }
                catch(...)
                {
                }
            }
            else
                break;
        }
    }

    template <typename ctype> promise <void> directory :: sampler <ctype> :: keepalive(std :: weak_ptr <arc> warc) const
    {
        while(true)
        {
            if(auto arc = warc.lock())
            {
                bool exception = false;

                try
                {
                    auto connection = co_await tcp :: connect(arc->_directory);
                    co_await connection.send(arc->_keyexchanger.publickey(), arc->_port, arc->_version);

                    if(!arc->_version)
                    {
                        auto response = co_await connection.template receive <uint64_t, std :: vector <member>> ();
                        auto & version = std :: get <0> (response);
                        auto & members = std :: get <1> (response);

                        arc->_version = version;
                        arc->_guard([&]()
                        {
                            for(const auto & member : members)
                                arc->_membership.add(member);
                        });
                    }
                    else
                    {
                        auto log = co_await connection.template receive <std :: vector <update>> ();
                        arc->_version += log.size();

                        arc->_guard([&]()
                        {
                            for(const auto & update : log)
                            {
                                update.match([&](const add & member)
                                {
                                    arc->_membership.add(member);
                                }, [&](const remove & publickey)
                                {
                                    arc->_membership.remove(publickey);
                                });
                            }
                        });
                    }

                    co_await wait(settings :: intervals :: keepalive);
                }
                catch(...)
                {
                    exception = true; // Cannot co_await inside of a catch.
                }

                if(exception)
                    co_await wait(settings :: intervals :: retry);
            }
            else
                break;
        }
    }

    // Private static methods

    template <typename ctype> listener directory :: sampler <ctype> :: listen(class address :: port & port)
    {
        while(true)
        {
            try
            {
                port = 49152 + randombytes_uniform(16383);
                return tcp :: listen(port);
            }
            catch(...)
            {
            }
        }
    }

    // arc

    template <typename ctype> directory :: sampler <ctype> :: arc :: arc(const address & directory) : _directory(directory), _version(0), _listener(listen(this->_port))
    {
    }
};

#endif
