namespace psb
{
    template <typename> class sampler;
};

#if !defined(__forward__) && !defined(__src__sampler__sampler__h)
#define __src__sampler__sampler__h

// Libraries

#include <memory>
#include <type_traits>

#include <drop/data/variant.hpp>
#include <drop/network/connection.hpp>
#include <drop/utils/parameters.h>

// Includes

#include "directory.h"

namespace psb
{
    using namespace drop;

    template <typename ctype> class sampler
    {
        // Static asserts

        static_assert(std :: is_enum <ctype> :: value, "The template parameter of a sampler must be an enumeration type.");

        // Service typedefs

        typedef variant <directory :: sampler <ctype>> variant;

        // Members

        variant _sampler;

    public:

        // Constructors

        template <typename type, std :: enable_if_t <parameters :: in <type, variant> :: value> * = nullptr> sampler(const type &);

        // Methods

        template <ctype> promise <connection> connectunbiased();
        template <ctype> promise <connection> connectbiased();
        template <ctype> promise <connection> connect();

        template <ctype> promise <connection> accept();
    };
};

#endif
