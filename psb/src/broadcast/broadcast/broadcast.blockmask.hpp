#ifndef __src__broadcast__broadcast__broadcast__blockmask__hpp
#define __src__broadcast__broadcast__broadcast__blockmask__hpp

// Includes

#include "../broadcast.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename type> broadcast <type> :: blockmask :: blockmask() : _size(0)
    {
    }

    // Methods

    template <typename type> void broadcast <type> :: blockmask :: push(const batchinfo & batch)
    {
        for(uint32_t sequence = 0; sequence < batch.size; sequence++)
            this->_blocks.push_back(blockid{.hash = batch.hash, .sequence = sequence});

        this->_size += batch.size;
    }

    template <typename type> offlist broadcast <type> :: blockmask :: pop(const std :: unordered_set <blockid, shorthash> & remove)
    {
        offlist offlist;
        uint32_t index = 0;

        for(auto & block : this->_blocks)
        {
            if(block)
            {
                if(remove.find(*block) != remove.end())
                {
                    offlist.add(index);
                    block.erase();
                    this->_size--;
                }

                index++;
            }
        }

        this->defrag();
        return offlist;
    }

    template <typename type> std :: unordered_set <typename broadcast <type> :: blockid, shorthash> broadcast <type> :: blockmask :: pop(const offlist & offlist)
    {
        std :: unordered_set <blockid, shorthash> remove;

        if(!(offlist.size()))
            return remove;

        if(!(this->_blocks.size()))
            exception <malformed_mask, out_of_range> :: raise(this);

        size_t cursor = 0;
        int32_t blockindex = (this->_blocks.front() ? 0 : -1);

        for(int32_t offlistindex : offlist)
        {
            while(blockindex < offlistindex)
            {
                cursor++;
                if(cursor >= this->_blocks.size())
                    exception <malformed_mask, out_of_range> :: raise(this);

                if(this->_blocks[cursor])
                    blockindex++;
            }

            remove.insert(*(this->_blocks[cursor]));
            this->_blocks[cursor].erase();
            this->_size--;
        }

        this->defrag();
        return remove;
    }

    // Private methods

    template <typename type> void broadcast <type> :: blockmask :: defrag()
    {
        if((this->_blocks.size() - this->_size) >= (settings :: defragthreshold * this->_size))
        {
            size_t write = 0;
            for(size_t read = 0; read < this->_blocks.size(); read++)
                if(this->_blocks[read])
                    this->_blocks[write++] = this->_blocks[read];

            this->_blocks.resize(write);
        }
    }
};

#endif
