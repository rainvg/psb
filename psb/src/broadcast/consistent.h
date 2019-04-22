// Forward declarations

namespace psb
{
    // Tags

    class spot;
    class deliver;

    // Classes

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
            struct sample
            {
                static constexpr size_t size = 320;
                static constexpr size_t threshold = 238;
            };

            struct keepalive
            {
                static constexpr interval interval = 5_s;
            };
        };

        // Service nested structs

        struct index;
        struct subscriber;
        struct echo;

        // Service nested classes

        class verifier;
        class arc;

    public: // REMOVE ME

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        consistent(const sampler <channels> &, const int &);

    private:
    public: // REMOVE ME

        // Private constructors

        consistent(const std :: shared_ptr <arc> &);

    public:

        // Methods

        template <typename etype, std :: enable_if_t <std :: is_same <etype, spot> :: value> * = nullptr> void on(const std :: function <void (const hash &)> &);
        template <typename etype, std :: enable_if_t <std :: is_same <etype, deliver> :: value> * = nullptr> void on(const std :: function <void (const typename broadcast <type> :: batch &)> &);

        void publish(const class signer :: publickey &, const uint32_t &, const type &, const signature &);

    private:
    public: // REMOVE ME

        // Private methods

        void spot(std :: weak_ptr <arc>, const hash &);
        promise <void> dispatch(std :: weak_ptr <arc>, typename broadcast <type> :: batch);
        promise <void> serve(std :: weak_ptr <arc>, connection);

        void check(const hash &);
        void deliver(const typename broadcast <type> :: batch &);

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

        // Operators

        bool operator == (const index &) const;
    };

    template <typename type> struct consistent <type> :: subscriber
    {
        connection connection;
        optional <promise <void>> keepalive;
    };

    template <typename type> struct consistent <type> :: echo
    {
        size_t echoes;
        std :: unordered_map <size_t, size_t> negations;
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

        class pool
        {
            // Friends

            friend class verifier;

            // Members

            verifier * _verifiers;
            size_t _size;

            // Private constructors

            pool();

        public:

            // Destructor

            ~pool();

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

        static pool system;

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

        template <typename> friend class consistent;

        // Members

        sampler <channels> _sampler;

        std :: unordered_map <index, type, shorthash> _messages;
        std :: unordered_map <hash, typename broadcast <type> :: batch, shorthash> _batches;

        std :: unordered_map <hash, offlist, shorthash> _collisions;
        std :: unordered_map <hash, echo, shorthash> _echoes;

        std :: unordered_map <hash, std :: vector <subscriber>, shorthash> _subscribers;

        struct
        {
            std :: vector <std :: function <void (const hash &)>> spot;
            std :: vector <std :: function <void (const typename broadcast <type> :: batch &)>> deliver;
        } _handlers;

        broadcast <type> _broadcast;
        guard <simple> _guard;

    public:

        // Constructors

        arc(const sampler <channels> &, const int &);
    };
};

#endif
