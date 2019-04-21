// Forward declarations

namespace psb
{
    template <typename> class consistent;
};

#if !defined(__forward__) && !defined(__src__broadcast__consistent__h)
#define __src__broadcast__consistent__h

// Libraries

#include <memory>

// Includes

#include "psb/sampler/sampler.hpp"
#include "channels.h"
#include "broadcast.hpp"

namespace psb
{
    using namespace drop;

    template <typename type> class consistent
    {

        // Service nested classe

        class arc;

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        consistent(const sampler <channels> &);
    };

    template <typename type> class consistent <type> :: arc
    {
        // Friends

        template <typename> class consistent;

        // Members

        sampler <channels> _sampler;

        broadcast <type> _broadcast;
        guard <simple> _guard;

    public:

        // Constructors

        arc(const sampler <channels> &);
    };
};

#endif
