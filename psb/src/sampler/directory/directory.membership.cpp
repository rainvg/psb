// Includes

#include "../directory.h"

namespace psb
{
    using namespace drop;

    // Getters

    address directory :: membership :: address(const class keyexchanger :: publickey & publickey) const
    {
        size_t index = this->_indexes.find(publickey)->second;
        return (*(this->_members[index])).address;
    }

    // Methods

    void directory :: membership :: add(const member & member)
    {
        this->_members.push_back(member);
        this->_indexes[member.publickey] = this->_members.size() - 1;
    }

    void directory :: membership :: remove(const class keyexchanger :: publickey & publickey)
    {
        size_t index = this->_indexes[publickey];
        this->_members[index].erase();
        this->_indexes.erase(publickey);

        if((this->_members.size() - this->_indexes.size()) >= (settings :: defragthreshold * this->_indexes.size()))
        {
            size_t cursor = 0;
            for(auto member : this->_members)
            {
                if(member)
                    this->_members[cursor++] = member;
            }

            this->_members.resize(cursor);

            this->_indexes.clear();
            for(size_t index = 0; index < this->_members.size(); index++)
                this->_indexes[(*(this->_members[index])).publickey] = index;
        }
    }

    directory :: member directory :: membership :: pick()
    {
        if(this->_members.empty())
            exception <membership_empty> :: raise(this);

        while(true)
        {
            size_t index = randombytes_uniform(this->_members.size());

            if(this->_members[index])
                return *(this->_members[index]);
        }
    }
};
