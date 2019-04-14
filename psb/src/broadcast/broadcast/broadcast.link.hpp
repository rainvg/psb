#ifndef __src__broadcast__broadcast__broadcast__link__hpp
#define __src__broadcast__broadcast__broadcast__link__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: link :: link(const connection & connection) : _connection(connection), _state(setup)
    {
    }

    // Methods

    template <typename type> void broadcast <type> :: link :: announce(const announcement & announcement)
    {
        bool post = this->_guard([&]()
        {
            if(this->_state == alive)
            {
                this->_announcements.push_back(announcement);
                return true;
            }
            else
                return false;
        });

        if(post)
            this->_pipe.post();
    }

    template <typename type> void broadcast <type> :: link :: advertise(const blockid & block)
    {
        bool post = this->_guard([&]()
        {
            if(this->_state == alive)
            {
                this->_advertisements.insert(block);
                return true;
            }
            else
                return false;
        });

        if(post)
            this->_pipe.post();
    }

    template <typename type> void broadcast <type> :: link :: request(const blockid & block)
    {
        this->_guard([&]()
        {
            if(this->_state != alive)
                exception <dead_link> :: raise(this);

            this->_requests.local.push_back(block);
        });

        this->_pipe.post();
    }

    template <typename type> void broadcast <type> :: link :: start(const std :: weak_ptr <arc> & warc, const std :: shared_ptr <link> & link)
    {
        this->sync(warc, link);
    }

    // Services

    template <typename type> promise <void> broadcast <type> :: link :: sync(std :: weak_ptr <arc> warc, std :: shared_ptr <link> link)
    {

    }
};

#endif
