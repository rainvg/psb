// Includes

#include "directory.h"

namespace psb
{
    using namespace drop;

    // directory

    // Constructors

    directory :: directory(const class address :: port & binding) : _arc(std :: make_shared <arc> (tcp :: listen(binding)))
    {
        this->_arc->_listener.set <timeouts :: accept> (settings :: timeouts :: network);
        this->run(this->_arc);
    }

    directory :: directory(const address & binding) : _arc(std :: make_shared <arc> (tcp :: listen(binding)))
    {
        this->_arc->_listener.set <timeouts :: accept> (settings :: timeouts :: network);
        this->run(this->_arc);
    }

    // Private methods

    promise <void> directory :: timeout(std :: weak_ptr <arc> warc, class keyexchanger :: publickey publickey)
    {
        co_await wait(settings :: timeouts :: keepalive);

        if(auto arc = warc.lock())
        {
            arc->_guard([&]()
            {
                timestamp currenttime = now();
                if((arc->_members.find(publickey) != arc->_members.end()) && (currenttime - arc->_members[publickey].lastkeepalive >= settings :: timeouts :: keepalive))
                {
                    arc->_log.push_back(remove{publickey});
                    arc->_members.erase(publickey);
                }
            });
        }
    }

    promise <void> directory :: serve(std :: weak_ptr <arc> warc, connection connection)
    {
        if(auto arc = warc.lock())
        {
            try
            {
                auto keepalive = co_await connection.receive <class keyexchanger :: publickey, class address :: port, uint64_t> ();

                auto & publickey = std :: get <0> (keepalive); // Structured binding cannot (yet?) be captured by lambdas.
                auto & port = std :: get <1> (keepalive);
                auto & version = std :: get <2> (keepalive);

                address address(connection.remote().ip(), port);

                bool flush = !version;

                static thread_local std :: vector <member> members;
                static thread_local std :: vector <update> log;

                members.clear();
                log.clear();

                arc->_guard([&]()
                {
                    if(arc->_members.find(publickey) != arc->_members.end())
                    {
                        if(arc->_members[publickey].address != address)
                        {
                            arc->_members[publickey] = {.address = address, .lastkeepalive = now()};

                            arc->_log.push_back(remove{publickey});
                            arc->_log.push_back(add{.publickey = publickey, .address = address});
                        }
                        else
                            arc->_members[publickey].lastkeepalive = now();
                    }
                    else
                    {
                        arc->_members[publickey] = {.address = address, .lastkeepalive = now()};
                        arc->_log.push_back(add{.publickey = publickey, .address = address});
                    }

                    if(flush)
                    {
                        members.reserve(arc->_members.size());

                        for(const auto & entry : arc->_members)
                            members.push_back(member{.publickey = entry.first, .address = entry.second.address});
                    }
                    else
                    {
                        log.reserve(arc->_log.size() - version);

                        for(size_t v = version; v < arc->_log.size(); v++)
                            log.push_back(arc->_log[v]);
                    }

                    version = arc->_log.size();
                });

                if(flush)
                    co_await connection.send(version, members);
                else
                    co_await connection.send(log);

                this->timeout(warc, publickey);

                co_await wait(1_s);
            }
            catch(const exception <> & exception)
            {
            }
        }
    }

    // Services

    promise <void> directory :: run(std :: weak_ptr <arc> warc)
    {
        while(true)
        {
            if(auto arc = warc.lock())
            {
                try
                {
                    auto connection = co_await arc->_listener.accept();

                    connection.set <timeouts :: send> (settings :: timeouts :: network);
                    connection.set <timeouts :: receive> (settings :: timeouts :: network);

                    this->serve(warc, connection);
                }
                catch(...)
                {
                }
            }
            else
                break;
        }
    }

    // arc

    // Constructors

    directory :: arc :: arc(const listener & listener) : _listener(listener)
    {
    }
};
