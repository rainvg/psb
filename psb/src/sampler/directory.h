namespace psb
{
    class directory;
};

#if !defined(__forward__) && !defined(__src__sampler__directory__h)
#define __src__sampler__directory__h

// Libraries

#include <vector>
#include <unordered_map>

#include <drop/bytewise/bytewise.hpp>
#include <drop/network/connection.hpp>
#include <drop/network/listener.hpp>
#include <drop/crypto/keyexchanger.h>
#include <drop/crypto/shorthash.hpp>
#include <drop/chrono/crontab.h>

// Forward includes

#define __forward__
#include "sampler.h"
#undef __forward__

namespace psb
{
    using namespace drop;

    class directory
    {
        // Friends

        template <typename> friend class psb :: sampler;

        // Service nested classes

        class membership;
        template <typename> class sampler;

        // Service nested structs

        struct member
        {
            // Members

            address address;
            class keyexchanger :: publickey publickey;

            // Bytewise

            $bytewise(address);
            $bytewise(publickey);
        };
    };

    class directory :: membership
    {
        // Settings

        struct settings
        {
            static constexpr size_t defragthreshold = 1;
        };

        // Members

        std :: vector <optional <member>> _members;
        std :: unordered_map <class keyexchanger :: publickey, size_t, shorthash> _indexes;

    public:

        // Getters

        address address(const class keyexchanger :: publickey &) const;

        // Methods

        void add(const member &);
        void remove(const class keyexchanger :: publickey &);

        member pick();
    };

    template <typename> class directory :: sampler
    {
    };
};

#endif
