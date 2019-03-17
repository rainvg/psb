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
#include <drop/thread/guard.hpp>

// Forward includes

#define __forward__
#include "sampler.h"
#undef __forward__

namespace psb
{
    using namespace drop;

    class directory
    {
        // Settings

        struct settings
        {
            struct timeouts
            {
                static constexpr interval keepalive = 30_s;
                static constexpr interval network = 10_s;
            };

            static constexpr interval keepalive = 10_s;
        };

        // Friends

        template <typename> friend class psb :: sampler;

        // Service nested classes

        class arc;

        class membership;
        template <typename> class sampler;

        // Service nested structs

        struct member
        {
            // Members

            class keyexchanger :: publickey publickey;
            address address;

            // Bytewise

            $bytewise(address);
            $bytewise(publickey);
        };

        struct entry
        {
            address address;
            timestamp lastkeepalive;
        };

        // Service typedefs

        typedef member add;
        typedef class keyexchanger :: publickey remove;
        typedef variant <add, remove> update;

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        directory(const class address :: port &);
        directory(const address &);

    private:

        // Private methods

        promise <void> timeout(std :: weak_ptr <arc>, class keyexchanger :: publickey);

        promise <void> serve(std :: weak_ptr <arc>, connection);
        promise <void> run(std :: weak_ptr <arc>);

    public:

        // Static methods

        template <typename ctype> static psb :: sampler <ctype> sample(const address &);
    };

    class directory :: arc
    {
        // Friends

        friend class directory;

        // Members

        std :: unordered_map <class keyexchanger :: publickey, entry, shorthash> _members;
        std :: vector <update> _log;

        guard <simple> _guard;
        listener _listener;

    public:

        // Constructors

        arc(const listener &);
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
        // Service nested classes

        class arc;

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        sampler(const address &);

    private:

        // Private methods

        promise <void> keepalive(std :: weak_ptr <arc>);

        // Private static methods

        static listener listen(class address :: port &);
    };

    template <typename ctype> class directory :: sampler <ctype> :: arc
    {
        // Friends

        template <typename> friend class directory :: sampler;

        // Members

        address _directory;
        keyexchanger _keyexchanger;

        membership _membership;
        uint64_t _version;

        guard <simple> _guard;

        class address :: port _port;
        listener _listener;

    public:

        // Constructors

        arc(const address &);
    };
};

#endif
