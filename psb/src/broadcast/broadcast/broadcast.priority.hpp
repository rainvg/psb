#ifndef __src__broadcast__broadcast__broadcast__priority__hpp
#define __src__broadcast__broadcast__broadcast__priority__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    // iterator

    // Constructors

    template <typename type> broadcast <type> :: priority :: iterator :: iterator(const priority & priority, const size_t & cursor) : _priority(priority), _cursor(cursor)
    {
    }

    // Operators

    template <typename type> typename broadcast <type> :: priority :: iterator & broadcast <type> :: priority :: iterator :: operator ++ ()
    {
        while(++(this->_cursor) < this->_priority._fifo.size() && !(this->_priority._fifo[this->_cursor]));
        return (*this);
    }

    template <typename type> typename broadcast <type> :: priority :: iterator broadcast <type> :: priority :: iterator :: operator ++ (int)
    {
        iterator previous = (*this);
        ++(*this);
        return previous;
    }

    template <typename type> bool broadcast <type> :: priority :: iterator :: operator == (const iterator & rho) const
    {
        return this->_cursor == rho._cursor;
    }

    template <typename type> bool broadcast <type> :: priority :: iterator :: operator != (const iterator & rho) const
    {
        return this->_cursor != rho._cursor;
    }

    template <typename type> hash broadcast <type> :: priority :: iterator :: operator * () const
    {
        return this->_priority._fifo[this->_cursor];
    }

    // priority

    // Constructors

    template <typename type> broadcast <type> :: priority :: priority() : _offset(0), _nonce(0)
    {
    }

    // Iterators

    template <typename type> typename broadcast <type> :: priority :: iterator broadcast <type> :: priority :: begin() const
    {
        return iterator(*this, 0);
    }

    template <typename type> typename broadcast <type> :: priority :: iterator broadcast <type> :: priority :: end() const
    {
        return iterator(*this, this->_fifo.size());
    }

    // Methods

    template <typename type> void broadcast <type> :: priority :: push(const hash & hash)
    {
        this->_fifo.push_back(hash);
        this->_map[hash] = this->_nonce++;
    }

    template <typename type> void broadcast <type> :: priority :: remove(const hash & hash)
    {
        uint64_t priority = this->_map[hash];
        this->_map.erase(hash);
        this->_fifo[priority - this->_offset].erase();

        size_t pop = 0;
        while(!(this->_fifo[pop]))
            pop++;

        this->_fifo.erase(this->_fifo.begin(), this->_fifo.begin() + pop);
        this->_offset += pop;
    }

    // Operators

    template <typename type> uint64_t broadcast <type> :: priority :: operator [] (const hash & hash) const
    {
        return this->_map.at(hash);
    }
};

#endif
