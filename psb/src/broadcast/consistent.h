// Forward declarations

namespace psb
{
    template <typename> class consistent;
};

#if !defined(__forward__) && !defined(__src__broadcast__consistent__h)
#define __src__broadcast__consistent__h

// Libraries

#include <memory>
#include <thread>

#include <drop/crypto/signature.hpp>
#include <drop/crypto/hash.hpp>
#include <drop/bytewise/bytewise.hpp>
#include <drop/chrono/time.hpp>
#include <drop/thread/guard.hpp>
#include <drop/crypto/shorthash.hpp>
#include <drop/network/connection.hpp>
#include <drop/data/offlist.hpp>
#include <drop/thread/semaphore.h>

// Includes

#include "psb/sampler/sampler.hpp"
#include "channels.h"
#include "broadcast.hpp"

namespace psb
{
    using namespace drop;

    template <typename type> class consistent
    {
        // Settings

        struct settings
        {
            struct keepalive
            {
                static constexpr interval interval = 5_s;
            };
        };

        // Service nested structs

        struct index;
        struct subscriber;

        // Service nested classes

        class verifier;
        class arc;

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        consistent(const sampler <channels> &);

    private:

        // Private constructors

        consistent(const std :: shared_ptr <arc> &);

        // Private methods

        promise <void> dispatch(std :: weak_ptr <arc>, const typename broadcast <type> :: batch &);
        promise <void> serve(std :: weak_ptr <arc>, connection);

        // Services

        promise <void> accept(std :: weak_ptr <arc>, sampler <channels>);
        promise <void> keepalive(std :: weak_ptr <arc>);
    };

    template <typename type> struct consistent <type> :: index
    {
        // Public members

        class signer :: publickey feed;
        uint32_t sequence;

        // Bytewise

        $bytewise(feed);
        $bytewise(sequence);
    };

    template <typename type> struct consistent <type> :: subscriber
    {
        connection connection;
        promise <void> keepalive;
    };

    template <typename type> class consistent <type> :: verifier
    {
        // Service nested structs

        struct workunit
        {
            typename broadcast <type> :: batch batch;
            promise <std :: vector <uint32_t>> promise;
        };

        // Service nested classes

        class system
        {
            // Friends

            friend class verifier;

            // Members

            verifier * _verifiers;
            size_t _size;

            // Private constructors

            system();

        public:

            // Destructor

            ~system();

            // Methods

            verifier & get();

        private:

            // Static members

            thread_local static size_t roundrobin;
        };

        // Members

        std :: deque <workunit> _workunits;
        guard <simple> _guard;

        bool _alive;

        semaphore _semaphore;
        std :: thread _thread;

    public:

        // Public static members

        static system system;

        // Constructors

        verifier();

        // Destructor

        ~verifier();

        // Methods

        promise <std :: vector <uint32_t>> verify(const typename broadcast <type> :: batch &);

    private:

        // Services

        void run();
    };

    template <typename type> class consistent <type> :: arc
    {
        // Friends

        template <typename> class consistent;

        // Members

        sampler <channels> _sampler;

        std :: unordered_map <index, type, shorthash> _messages;
        std :: unordered_map <hash, offlist, shorthash> _batches;

        std :: unordered_map <hash, std :: vector <subscriber>, shorthash> _subscribers;

        broadcast <type> _broadcast;
        guard <simple> _guard;

    public:

        // Constructors

        arc(const sampler <channels> &);
    };
};

#endif
