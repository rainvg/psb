#ifndef __src__sampler__directory__directory__sampler__hpp
#define __src__sampler__directory__directory__sampler__hpp

#include "../directory.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename ctype> directory :: sampler <ctype> :: sampler(const address & directory) : _directory(directory), _version(0), _listener(this->listen())
    {
        this->keepalive();
    }

    // Private methods

    template <typename ctype> listener directory :: sampler <ctype> :: listen()
    {
        while(true)
        {
            try
            {
                this->_port = 49152 + randombytes_uniform(16383);
                return tcp :: listen(this->_port);
            }
            catch(...)
            {
            }
        }
    }

    template <typename ctype> promise <void> directory :: sampler <ctype> :: keepalive()
    {
        while(true)
        {
            try
            {
                std :: cout << "Sending keepalive." << std :: endl;

                auto connection = co_await tcp :: connect(this->_directory);
                co_await connection.send(this->_keyexchanger.publickey(), this->_port, this->_version);

                if(!this->_version)
                {
                    auto response = co_await connection.template receive <uint64_t, std :: vector <member>> ();
                    auto & version = std :: get <0> (response);
                    auto & members = std :: get <1> (response);

                    this->_version = version;

                    std :: cout << "Received " << members.size() << " members" << (members.size() ? ":" : ".") << std :: endl;

                    this->_guard([&]()
                    {
                        for(const auto & member : members)
                        {
                            std :: cout << "\t" << member.publickey << ": " << member.address << std :: endl;
                            this->_membership.add(member);
                        }
                    });
                }
                else
                {
                    auto log = co_await connection.template receive <std :: vector <update>> ();
                    this->_version += log.size();

                    std :: cout << "Received " << log.size() << " updates" << (log.size() ? ":" : ".") << std :: endl;

                    this->_guard([&]()
                    {
                        for(const auto & update : log)
                        {
                            update.match([&](const add & member)
                            {
                                std :: cout << "\tAdd " << member.publickey << ": " << member.address << std :: endl;
                                this->_membership.add(member);
                            }, [&](const remove & publickey)
                            {
                                std :: cout << "\tRemove " << publickey << std :: endl;
                                this->_membership.remove(publickey);
                            });
                        }
                    });
                }

                co_await wait(settings :: keepalive);
            }
            catch(const exception <> & exception)
            {
                std :: cout << "Sampler exception: " << exception.what() << std :: endl;
            }
        }
    }
};

#endif
