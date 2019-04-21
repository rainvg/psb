// Forward declarations

namespace psb
{
    // Tags

    class malformed_mask;
    class malformed_block;
    class hash_mismatch;
    class ghost_request;
    class out_of_range;
    class dead_link;
    class arc_expired;

    // Classes

    template <typename> class broadcast;
};

#if !defined(__forward__) && !defined(__src__broadcast__broadcast__h)
#define __src__broadcast__broadcast__h

// Libraries

#include <iostream> // REMOVE ME
#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include <drop/crypto/signature.hpp>
#include <drop/crypto/hash.hpp>
#include <drop/bytewise/bytewise.hpp>
#include <drop/chrono/time.hpp>
#include <drop/thread/guard.hpp>
#include <drop/data/syncset.hpp>
#include <drop/data/offlist.hpp>
#include <drop/crypto/shorthash.hpp>
#include <drop/async/pipe.hpp>
#include <drop/network/connection.hpp>

// Includes

#include "psb/sampler/sampler.hpp"
#include "channels.h"

namespace psb
{
    using namespace drop;

    template <typename type> class broadcast
    {
        // Static asserts

        static_assert(bytewise :: constraints :: serializable <type> () && bytewise :: constraints :: deserializable <type> (), "Broadcast type must be serializable and deserializable.");

        // Settings

        struct settings
        {
            struct block
            {
                static constexpr size_t size = 16;
            };

            struct link
            {
                static constexpr interval keepalive = 5_s;
            };

            struct collect
            {
                static constexpr interval interval = 2_s;
            };
        };

    public:

        // Configuration

        struct configuration
        {
            struct sponge
            {
                static size_t capacity;
                static interval timeout;
            };

            struct link
            {
                static double lambda;
            };

            struct lanes
            {
                struct fast
                {
                    static size_t links;
                    static size_t requests;

                    struct churn
                    {
                        static size_t period;
                        static double percentile;
                    };
                };

                struct secure
                {
                    struct links
                    {
                        static size_t max;
                        static size_t min;
                    };

                    static size_t requests;
                };
            };
        };

    public:

        // Nested structs

        struct message;
        struct batchinfo;
        struct batch;

        // Nested classes

        class block;

    private:
    public: // REMOVE ME

        // Service nested enum

        enum lane {fast, secure, guest};

        // Service nested structs

        struct blockid;
        struct announcement;
        struct transfer;

        // Service nested classes

        class sponge;
        class batchset;
        class blockmask;
        class link;
        class priority;

        class arc;

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        broadcast(const sampler <channels> &, const int &);

    private:

        // Private constructors

        broadcast(const std :: shared_ptr <arc> &);

        // Private getters

        std :: vector <hash> proof(const hash &) const;
        block block(const blockid &) const;

        const batchset & delivered() const;
        bool announced(const hash &) const;

    public:

        // Methods

        template <typename etype, std :: enable_if_t <std :: is_same <etype, batch> :: value> * = nullptr> void on(const std :: function <void (const batch &)> &);
        void publish(const class signer :: publickey &, const uint32_t &, const type &, const signature &);

    private:
    public: // REMOVE ME

        // Private methods

        void spot(const batchinfo &);
        void announce(const announcement &);

        void available(const hash &, const std :: shared_ptr <link> &);
        void available(const blockid &, const std :: shared_ptr <link> &);

        void dispatch(const blockid &, const std :: vector <hash> &, const class block &, const std :: shared_ptr <link> &);
        void deliver(const batchinfo &);

        void release(const std :: vector <class block> &);

        template <lane, typename... connections> promise <void> link(const connections & ...);
        void unlink(const std :: shared_ptr <class link> &, const std :: vector <blockid> &);

        // Services

        promise <void> drive(std :: weak_ptr <arc>);
        promise <void> accept(std :: weak_ptr <arc>, sampler <channels>);
        promise <void> collect(std :: weak_ptr <arc>);
    };

    template <typename type> struct broadcast <type> :: message
    {
        // Public members

        class signer :: publickey feed;
        uint32_t sequence;
        type payload;
        signature signature;

        // Bytewise

        $bytewise(feed);
        $bytewise(sequence);
        $bytewise(payload);
        $bytewise(signature);
    };

    template <typename type> struct broadcast <type> :: batchinfo
    {
        // Public members

        hash hash;
        uint32_t size;

        // Bytewise

        $bytewise(hash);
        $bytewise(size);

        // Operators

        bool operator == (const batchinfo &) const;
    };

    template <typename type> struct broadcast <type> :: batch
    {
        batchinfo info;
        std :: vector <class block> blocks;
    };

    template <typename type> struct broadcast <type> :: blockid
    {
        // Public members

        hash hash;
        uint32_t sequence;

        // Bytewise

        $bytewise(hash);
        $bytewise(sequence);

        // Operators

        bool operator < (const blockid &) const;
        bool operator == (const blockid &) const;
    };

    template <typename type> struct broadcast <type> ::  announcement
    {
        // Public members

        batchinfo batch;
        bool available;

        // Bytewise

        $bytewise(batch);
        $bytewise(available);
    };

    template <typename type> struct broadcast <type> ::  transfer
    {
        uint32_t size;
        std :: unordered_map <uint32_t, std :: vector <std :: weak_ptr <class link>>> providers;
    };

    template <typename type> class broadcast <type> :: block
    {
        // Service nested structs

        struct arc
        {
            std :: array <message, settings :: block :: size> messages;
            size_t size;
        };

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        block();
        block(const std :: vector <message> &);

        // Getters

        const size_t & size() const;

        // Iterators

        inline auto begin() const;
        inline auto end() const;

        // Methods

        void push(const message &);

        // Operators

        const message & operator [] (const size_t &) const;
    };

    template <typename type> class broadcast <type> :: sponge
    {
        // Members

        std :: vector <class block> _blocks;
        size_t _nonce;

        guard <simple> _guard;

    public:

        // Constructors

        sponge();

        // Methods

        void push(const std :: weak_ptr <arc> &, const message &);

    private:

        // Private methods

        void flush(const std :: weak_ptr <arc> &, const size_t &);
        promise <void> timeout(std :: weak_ptr <arc>, size_t);
    };

    template <typename type> class broadcast <type> :: batchset
    {
        // Members

        syncset <batchinfo> _syncset;
        std :: vector <batchinfo> _buffer;
        size_t _locks;

    public:

        // Constructors

        batchset();

        // Getters

        size_t size() const;
        const std :: vector <batchinfo> & buffer() const;

        // Methods

        void add(const batchinfo &);
        bool find(const batchinfo &) const;

        typename syncset <batchinfo> :: round sync() const;
        typename syncset <batchinfo> :: round sync(const typename syncset <batchinfo> :: view &) const;

        void lock();
        void unlock();
    };

    template <typename type> class broadcast <type> :: blockmask
    {
        // Settings

        struct settings
        {
            static constexpr size_t defragthreshold = 1;
        };

        // Members

        std :: vector <optional <blockid>> _blocks;
        size_t _size;

    public:

        // Constructors

        blockmask();

        // Methods

        void push(const batchinfo &);

        offlist pop(const std :: unordered_set <blockid, shorthash> &);
        std :: unordered_set <blockid, shorthash> pop(const offlist &);

    private:

        // Private methods

        void defrag();
    };

    template <typename type> class broadcast <type> :: link
    {
        // Service nested structs

        struct payload
        {
            // Public members

            std :: vector <hash> proof;
            std :: vector <message> messages;

            // Bytewise

            $bytewise(proof);
            $bytewise(messages);
        };

        // Typedefs

        typedef variant
        <
            std :: vector <announcement>,
            offlist,
            std :: vector <blockid>,
            payload
        > transaction;

        // Members

        int _id;
        bool _alive;

        struct
        {
            blockmask local;
            blockmask remote;
        } _blockmasks;

        std :: vector <announcement> _announcements;
        std :: unordered_set <blockid, shorthash> _advertisements;

        struct
        {
            std :: vector <blockid> pending;
            std :: deque <blockid> local;
            std :: deque <blockid> remote;
        } _requests;

        struct
        {
            timestamp last;
            interval latency;
            timestamp keepalive;
        } _chrono;

        connection _connection;

        pipe <void> _pipe;
        guard <simple> _guard;

    public:

        // Constructors

        link(const connection &, const int &);

        // Getters

        int id() const;
        size_t requests();
        interval latency();

        // Methods

        void announce(const announcement &);
        void advertise(const blockid &);
        void request(const blockid &);

        promise <std :: vector <batchinfo>> sync(std :: weak_ptr <arc>, std :: shared_ptr <link>);
        void start(const std :: weak_ptr <arc> &, const std :: shared_ptr <link> &);

        void shutdown();

    private:

        // Private methods

        void unlink(const std :: weak_ptr <arc> &, const std :: shared_ptr <link> &);

        // Services

        promise <void> send(std :: weak_ptr <arc>, std :: shared_ptr <link>);
        promise <void> receive(std :: weak_ptr <arc>, std :: shared_ptr <link>);
        promise <void> keepalive(std :: shared_ptr <link>);
    };

    template <typename type> class broadcast <type> :: priority
    {
        // Service nested classes

        class iterator : public std :: iterator <std::input_iterator_tag, size_t, size_t, const hash *, hash>
        {
            // Members

            const priority & _priority;
            size_t _cursor;

        public:

            // Constructors

            iterator(const priority &, const size_t &);

            // Operators

            iterator & operator ++ ();
            iterator operator ++ (int);

            bool operator == (const iterator &) const;
            bool operator != (const iterator &) const;

            hash operator * () const;
        };

        // Members

        std :: deque <optional <hash>> _fifo;
        std :: unordered_map <hash, uint64_t, shorthash> _map;

        uint64_t _offset;
        uint64_t _nonce;

    public:

        // Constructors

        priority();

        // Iterators

        iterator begin() const;
        iterator end() const;

        // Methods

        void push(const hash &);
        void remove(const hash &);

        // Operators

        uint64_t operator [] (const hash &) const;
    };

    template <typename type> class broadcast <type> :: arc
    {
        // Friends

        template <typename> friend class broadcast;

        // Members

        int _id;

        sponge _sponge;

        std :: unordered_map <hash, std :: vector <hash>, shorthash> _proofs;
        std :: unordered_map <blockid, class block, shorthash> _blocks;

        std :: unordered_set <hash, shorthash> _announced;
        batchset _delivered;

        priority _priority;
        std :: unordered_map <hash, transfer, shorthash> _transfers;
        std :: unordered_map <std :: shared_ptr <class link>, std :: unordered_set <blockid, shorthash>> _providers;

        struct
        {
            size_t fast;
            size_t secure;
        } _handshakes;

        struct
        {
            size_t trigger;
        } _churn;

        struct
        {
            std :: unordered_set <std :: shared_ptr <class link>> fast;
            std :: unordered_set <std :: shared_ptr <class link>> secure;
            std :: unordered_set <std :: shared_ptr <class link>> idle;

            std :: unordered_set <std :: shared_ptr <class link>> guest;
            
            std :: vector <std :: shared_ptr <class link>> dead;
        } _links;

        struct
        {
            std :: unordered_set <blockid, shorthash> all;
            std :: unordered_set <blockid, shorthash> secure;

            std :: vector <blockid> failed;
        } _requests;

        std :: vector <std :: function <void (const batch &)>> _handlers;

        sampler <channels> _sampler;

        pipe <void> _pipe;
        guard <recursive> _guard;

    public:

        // Constructors

        arc(const sampler <channels> &, const int &);
    };
};

#endif
