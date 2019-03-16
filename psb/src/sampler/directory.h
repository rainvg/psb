namespace psb
{
    class directory;
};

#if !defined(__forward__) && !defined(__src__sampler__directory__h)
#define __src__sampler__directory__h

// Forward includes

#define __forward__
#include "sampler.h"
#undef __forward__

namespace psb
{
    class directory
    {
        // Friends

        template <typename> friend class psb :: sampler;

        // Service nested classes

        template <typename> class sampler;
    };

    template <typename> class directory :: sampler
    {
    };
};

#endif
