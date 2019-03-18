#ifndef __src__sampler__sampler__hpp
#define __src__sampler__sampler__hpp

// Includes

#include "sampler.h"

namespace psb
{
    using namespace drop;

    // Constructors

    template <typename ctype> template <typename type, std :: enable_if_t <parameters :: in <type, typename sampler <ctype> :: variant> :: value> *> sampler <ctype> :: sampler(const type & sampler) : _sampler(sampler)
    {
    }

    // Methods

    template <typename ctype> template <ctype channel> promise <connection> sampler <ctype> :: connectunbiased()
    {
        promise <connection> connection;

        this->_sampler.match([&](const auto & sampler)
        {
            connection = sampler.template connectunbiased <channel> ();
        });

        return connection;
    }

    template <typename ctype> template <ctype channel> promise <connection> sampler <ctype> :: connectbiased()
    {
        promise <connection> connection;

        this->_sampler.match([&](const auto & sampler)
        {
            connection = sampler.template connectbiased <channel> ();
        });

        return connection;
    }

    template <typename ctype> template <ctype channel> promise <connection> sampler <ctype> :: connect()
    {
        return this->connectunbiased <channel> ();
    }

    template <typename ctype> template <ctype channel> promise <connection> sampler <ctype> :: accept()
    {
        promise <connection> connection;

        this->_sampler.match([&](const auto & sampler)
        {
            connection = sampler.template accept <channel> ();
        });

        return connection;
    }

};

#endif
