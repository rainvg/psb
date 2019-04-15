// Forward declarations

namespace psb
{
    // Tags

    class malformed_mask;
    class malformed_block;
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
                static constexpr size_t size = 1;
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

        enum lane {fast, secure};

        // Service nested structs

        struct blockid;
        struct announcement;
        struct transfer;

        // Service nested classes

        class sponge;
        class batchset;
        class blockmask;
        class link;

        class arc;

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        broadcast();

    private:

        // Private constructors

        broadcast(const std :: shared_ptr <arc> &);

        // Private getters

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

        void dispatch(const blockid &, const class block &);
        void deliver(const batchinfo &);

        void release(const std :: vector <class block> &);

        template <lane> promise <void> link(const connection &);
        void unlink(const std :: shared_ptr <class link> &);

        // Services

        promise <void> run(std :: weak_ptr <arc>);
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
        // Typedefs

        typedef variant
        <
            std :: vector <announcement>,
            offlist,
            std :: vector <blockid>,
            std :: vector <message>
        > transaction;

        // Members

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

        connection _connection;

        pipe <void> _pipe;
        guard <simple> _guard;

    public:

        // Constructors

        link(const connection &);

        // Methods

        void announce(const announcement &);
        void advertise(const blockid &);
        void request(const blockid &);

        promise <std :: vector <batchinfo>> sync(std :: weak_ptr <arc>, std :: shared_ptr <link>);
        void start(const std :: weak_ptr <arc> &, const std :: shared_ptr <link> &);

    private:

        // Private methods

        void shutdown(const std :: weak_ptr <arc> &, const std :: shared_ptr <link> &);

        // Services

        promise <void> send(std :: weak_ptr <arc>, std :: shared_ptr <link>);
        promise <void> receive(std :: weak_ptr <arc>, std :: shared_ptr <link>);
    };

    template <typename type> class broadcast <type> :: arc
    {
        // Friends

        template <typename> friend class broadcast;

        // Members

        sponge _sponge;

        std :: unordered_map <blockid, class block, shorthash> _blocks;

        std :: unordered_set <hash, shorthash> _announced;
        batchset _delivered;

        std :: unordered_map <hash, transfer, shorthash> _transfers;

        struct
        {
            std :: unordered_set <std :: shared_ptr <class link>> fast;
            std :: unordered_set <std :: shared_ptr <class link>> secure;
            std :: unordered_set <std :: shared_ptr <class link>> idle;
        } _links;

        std :: vector <std :: function <void (const batch &)>> _handlers;

        pipe <void> _pipe;
        guard <simple> _guard;
    };
};

#endif
