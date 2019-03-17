// Includes

#include "directory.h"

namespace psb
{
    using namespace drop;

    // directory

    // Constructors

    directory :: directory(const class address :: port & binding) : _arc(std :: make_shared <arc> (tcp :: listen(binding)))
    {
        this->run(this->_arc);
    }

    directory :: directory(const address & binding) : _arc(std :: make_shared <arc> (tcp :: listen(binding)))
    {
        this->run(this->_arc);
    }

    // Private methods

    promise <void> directory :: timeout(std :: shared_ptr <arc> arc, class keyexchanger :: publickey publickey)
    {
        co_await wait(settings :: timeout);

        std :: cout << "Checking timeout for " << publickey << std :: endl;

        arc->_guard([&]()
        {
            timestamp currenttime = now();
            if((arc->_members.find(publickey) != arc->_members.end()) && (currenttime - arc->_members[publickey].lastkeepalive >= settings :: timeout))
            {
                std :: cout << "Removing " << publickey << ": " << arc->_members[publickey].lastkeepalive << " is before " << currenttime << std :: endl;
                arc->_log.push_back(remove{publickey});
                arc->_members.erase(publickey);
            }
        });
    }

    promise <void> directory :: serve(std :: shared_ptr <arc> arc, connection connection)
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
                        std :: cout << "Updating " << publickey << std :: endl;
                        arc->_members[publickey] = {.address = address, .lastkeepalive = now()};

                        arc->_log.push_back(remove{publickey});
                        arc->_log.push_back(add{.publickey = publickey, .address = address});
                    }
                    else
                    {
                        arc->_members[publickey].lastkeepalive = now();
                        std :: cout << "Renewing " << publickey << " to " << arc->_members[publickey].lastkeepalive << std :: endl;
                    }
                }
                else
                {
                    std :: cout << "Adding " << publickey << std :: endl;
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

            this->timeout(arc, publickey);

            co_await wait(1_s);
        }
        catch(const exception <> & exception)
        {
            std :: cout << "Directory exception: " << exception.what() << std :: endl;
        }
    }

    promise <void> directory :: run(std :: shared_ptr <arc> arc)
    {
        while(true)
            this->serve(arc, co_await arc->_listener.accept());
    }

    // arc

    // Constructors

    directory :: arc :: arc(const listener & listener) : _listener(listener)
    {
    }
};
