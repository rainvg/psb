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
        this->keepalive(this->_arc);
    }

    // Private methods

    template <typename ctype> promise <void> directory :: sampler <ctype> :: keepalive(std :: shared_ptr <arc> arc)
    {
        while(true)
        {
            try
            {
                std :: cout << "Sending keepalive." << std :: endl;

                auto connection = co_await tcp :: connect(arc->_directory);
                co_await connection.send(arc->_keyexchanger.publickey(), arc->_port, arc->_version);

                if(!arc->_version)
                {
                    auto response = co_await connection.template receive <uint64_t, std :: vector <member>> ();
                    auto & version = std :: get <0> (response);
                    auto & members = std :: get <1> (response);

                    arc->_version = version;

                    std :: cout << "Received " << members.size() << " members" << (members.size() ? ":" : ".") << std :: endl;

                    arc->_guard([&]()
                    {
                        for(const auto & member : members)
                        {
                            std :: cout << "\t" << member.publickey << ": " << member.address << std :: endl;
                            arc->_membership.add(member);
                        }
                    });
                }
                else
                {
                    auto log = co_await connection.template receive <std :: vector <update>> ();
                    arc->_version += log.size();

                    std :: cout << "Received " << log.size() << " updates" << (log.size() ? ":" : ".") << std :: endl;

                    arc->_guard([&]()
                    {
                        for(const auto & update : log)
                        {
                            update.match([&](const add & member)
                            {
                                std :: cout << "\tAdd " << member.publickey << ": " << member.address << std :: endl;
                                arc->_membership.add(member);
                            }, [&](const remove & publickey)
                            {
                                std :: cout << "\tRemove " << publickey << std :: endl;
                                arc->_membership.remove(publickey);
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
