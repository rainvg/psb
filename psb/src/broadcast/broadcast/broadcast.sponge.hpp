#ifndef __src__broadcast__broadcast__broadcast__sponge__hpp
#define __src__broadcast__broadcast__broadcast__sponge__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: sponge :: sponge()
    {
        this->_messages.reserve(configuration :: sponge :: capacity);
    }

    // Methods

    template <typename type> void broadcast <type> :: sponge :: push(const std :: weak_ptr <arc> & warc, const message & message)
    {
        size_t size;
        size_t nonce;

        this->_guard([&]()
        {
            this->_messages.push_back(message);
            size = this->_messages.size();
            nonce = this->_nonce;
        });

        if(size == 1)
            this->timeout(warc, this->_nonce);
        else if(size == configuration :: sponge :: capacity)
            this->flush(warc, this->_nonce);
    }

    // Private methods

    template <typename type> void broadcast <type> :: sponge :: flush(const std :: weak_ptr <arc> & warc, const size_t & nonce)
    {
        if(auto arc = warc.lock())
        {
            std :: vector <message> messages;

            bool release = this->_guard([&]() // `this` is guaranteed to exist if the `arc` exists.
            {
                if(this->_nonce == nonce)
                {
                    messages.swap(this->_messages);
                    this->_messages.reserve(configuration :: sponge :: capacity);
                    this->_nonce++;

                    return true;
                }
                else
                    return false;
            });

            if(release)
            {
                broadcast broadcast(arc);
                broadcast.release(messages);
            }
        }
    }

    template <typename type> promise <void> broadcast <type> :: sponge :: timeout(std :: weak_ptr <arc> warc, size_t nonce)
    {
        co_await wait(configuration :: sponge :: timeout);
        this->flush(warc, nonce); // `this` is not guaranteed to exist, but `flush()` locks `warc` before touching any member.
    }
};

#endif
