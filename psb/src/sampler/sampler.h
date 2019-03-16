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
        // Constraints

        struct constraints
        {
            template <typename, typename...> static constexpr bool construct();
        };

        // Static asserts

        static_assert(std :: is_enum <ctype> :: value, "The template parameter of a sampler must be an enumeration type.");

        // Service typedefs

        typedef variant <directory :: sampler <ctype>> variant;

        // Members

        std :: shared_ptr <variant> _arc;

        // Private constructors

        sampler(const std :: shared_ptr <variant> &);

    public:

        // Methods

        template <ctype> promise <connection> connectunbiased();
        template <ctype> promise <connection> connectbiased();
        template <ctype> promise <connection> connect();

        template <ctype> promise <connection> accept();

        // Static methods

        template <typename type, typename... atypes, std :: enable_if_t <constraints :: template construct <type, atypes...> ()> * = nullptr> static sampler construct(atypes && ...);
    };
};

#endif
