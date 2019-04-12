// Forward declarations

namespace psb
{
    template <typename> class broadcast;
};

#if !defined(__forward__) && !defined(__src__broadcast__broadcast__h)
#define __src__broadcast__broadcast__h

// Libraries

#include <iostream> // REMOVE ME
#include <memory>
#include <vector>
#include <unordered_map>

#include <drop/crypto/signature.hpp>
#include <drop/crypto/hash.hpp>
#include <drop/bytewise/bytewise.hpp>
#include <drop/chrono/time.hpp>
#include <drop/thread/guard.hpp>
#include <drop/data/syncset.hpp>

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
                static constexpr size_t size = 2;
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

    public:

        // Methods

        void publish(const class signer :: publickey &, const uint32_t &, const type &, const signature &);

    private:

        // Private methods

        void release(const std :: vector <block> &);
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
        std :: vector <block> blocks;
    };

    template <typename type> struct broadcast <type> :: blockid
    {
        hash hash;
        uint32_t sequence;
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
        std :: unordered_map <uint32_t, std :: vector <std :: weak_ptr <link>>> providers;
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

        // Getters

        const size_t & size() const;

        // Methods

        void push(const message &);

        // Operators

        const message & operator [] (const size_t &) const;
    };

    template <typename type> class broadcast <type> :: sponge
    {
        // Members

        std :: vector <block> _blocks;
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

    template <typename type> class broadcast <type> :: arc
    {
        // Friends

        template <typename> friend class broadcast;

        // Members

        sponge _sponge;
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

        typename syncset <batchinfo> :: round sync() const;
        typename syncset <batchinfo> :: round sync(const typename syncset <batchinfo> :: view &) const;

        void lock();
        void unlock();
    };
};

#endif
