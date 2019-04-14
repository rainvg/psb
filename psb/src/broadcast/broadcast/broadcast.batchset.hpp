#ifndef __src__broadcast__broadcast__broadcast__batchset__hpp
#define __src__broadcast__broadcast__broadcast__batchset__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: batchset :: batchset() : _locks(0)
    {
    }

    // Getters

    template <typename type> size_t broadcast <type> :: batchset :: size() const
    {
        return this->_syncset.size() + this->_buffer.size();
    }

    template <typename type> const std :: vector <typename broadcast <type> :: batchinfo> & broadcast <type> :: batchset :: buffer() const
    {
        return this->_buffer;
    }

    // Methods

    template <typename type> void broadcast <type> :: batchset :: add(const batchinfo & batch)
    {
        if(this->_locks == 0)
            this->_syncset.add(batch);
        else
            this->_buffer.push_back(batch);
    }

    template <typename type> bool broadcast <type> :: batchset :: find(const batchinfo & batch) const
    {
        return this->_syncset.find(batch) || (std :: find(this->_buffer.begin(), this->_buffer.end(), batch) != this->_buffer.end());
    }

    template <typename type> typename syncset <typename broadcast <type> :: batchinfo> :: round broadcast <type> :: batchset :: sync() const
    {
        return this->_syncset.sync();
    }

    template <typename type> typename syncset <typename broadcast <type> :: batchinfo> :: round broadcast <type> :: batchset :: sync(const typename syncset <batchinfo> :: view & view) const
    {
        return this->_syncset.sync(view);
    }

    template <typename type> void broadcast <type> :: batchset :: lock()
    {
        this->_locks++;
    }

    template <typename type> void broadcast <type> :: batchset :: unlock()
    {
        this->_locks--;

        if(!(this->_locks))
        {
            for(const auto & batch : this->_buffer)
                this->_syncset.add(batch);

            this->_buffer.clear();
        }
    }
};

#endif
