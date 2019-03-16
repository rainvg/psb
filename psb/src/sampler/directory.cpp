// Includes

#include "directory.h"

namespace psb
{
    using namespace drop;

    // directory

    // Constructors

    directory :: directory(const class address :: port & binding) : _listener(tcp :: listen(binding))
    {
        this->run();
    }

    directory :: directory(const address & binding) : _listener(tcp :: listen(binding))
    {
        this->run();
    }

    // Private methods

    promise <void> directory :: serve(connection connection)
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

            this->_guard([&]()
            {
                if(this->_members.find(publickey) != this->_members.end())
                {
                    if(this->_members[publickey].address != address)
                    {
                        this->_members[publickey] = {.address = address, .lastkeepalive = now()};

                        this->_log.push_back(remove{publickey});
                        this->_log.push_back(add{.publickey = publickey, .address = address});
                    }
                    else
                        this->_members[publickey].lastkeepalive = now();
                }
                else
                {
                    this->_members[publickey] = {.address = address, .lastkeepalive = now()};
                    this->_log.push_back(add{.publickey = publickey, .address = address});
                }

                if(flush)
                {
                    members.reserve(this->_members.size());

                    for(const auto & entry : this->_members)
                        members.push_back(member{.publickey = entry.first, .address = entry.second.address});
                }
                else
                {
                    log.reserve(this->_log.size() - version);

                    for(size_t v = version; v < this->_log.size(); v++)
                        log.push_back(this->_log[v]);
                }

                version = this->_log.size();
            });

            if(flush)
                co_await connection.send(version, members);
            else
                co_await connection.send(log);

            [=,this]()-> promise <void>
            {
                co_await wait(settings :: timeout);

                this->_guard([&]()
                {
                    if((this->_members.find(publickey) != this->_members.end()) && (now() - this->_members[publickey].lastkeepalive >= settings :: timeout))
                    {
                        this->_log.push_back(remove{publickey});
                        this->_members.erase(publickey);
                    }
                });
            }();

            co_await wait(1_s);
        }
        catch(const exception <> & exception)
        {
            std :: cout << "Directory exception: " << exception.what() << std :: endl;
        }
    }

    promise <void> directory :: run()
    {
        while(true)
            this->serve(co_await this->_listener.accept());

    }
};
