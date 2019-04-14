#ifndef __src__broadcast__broadcast__broadcast__block__hpp
#define __src__broadcast__broadcast__broadcast__block__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: block :: block() : _arc(new arc{.size = 0})
    {
    }

    template <typename type> broadcast <type> :: block :: block(const std :: vector <message> & messages) : block()
    {
        if(messages.size() > settings :: block :: size)
            exception <malformed_block> :: raise(this);

        for(const auto & message : messages)
            this->push(message);
    }

    // Getters

    template <typename type> const size_t & broadcast <type> :: block :: size() const
    {
        return this->_arc->size;
    }

    // Methods

    template <typename type> void broadcast <type> :: block :: push(const message & message)
    {
        this->_arc->messages[this->_arc->size++] = message;
    }

    // Operators

    template <typename type> const typename broadcast <type> :: message & broadcast <type> :: block :: operator [] (const size_t & index) const
    {
        return this->_arc->messages[index];
    }
};

#endif
