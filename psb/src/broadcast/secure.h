// Forward declarations

namespace psb
{
    // Tags

    class spot;
    class deliver;

    // Classes

    template <typename> class secure;
};

#if !defined(__forward__) && !defined(__src__broadcast__secure__h)
#define __src__broadcast__secure__h

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
#include "consistent.hpp"

namespace psb
{
    using namespace drop;

    template <typename type> class secure
    {
        // Settings

        struct settings
        {
            struct sample
            {
                static constexpr size_t size = 124;
                static constexpr size_t threshold = 74;
            };

            struct keepalive
            {
                static constexpr interval interval = 5_s;
            };
        };

        // Service nested structs

        struct index;
        struct client;
        struct server;

        // Service nested classes

        class arc;

    public: // REMOVE ME

        // Members

        std :: shared_ptr <arc> _arc;

    public:

        // Constructors

        secure(const sampler <channels> &, const int &);

    private:
    public: // REMOVE ME

        // Private constructors

        secure(const std :: shared_ptr <arc> &);

    public:

        // Methods

        template <typename etype, std :: enable_if_t <std :: is_same <etype, spot> :: value> * = nullptr> void on(const std :: function <void (const hash &)> &);
        template <typename etype, std :: enable_if_t <std :: is_same <etype, deliver> :: value> * = nullptr> void on(const std :: function <void (const typename broadcast <type> :: batch &)> &);

        void publish(const class signer :: publickey &, const uint32_t &, const type &, const signature &);

    private:
    public: // REMOVE ME

        // Private methods

        void spot(std :: weak_ptr <arc>, const hash &);
        void dispatch(std :: weak_ptr <arc>, typename broadcast <type> :: batch);

        void check(const hash &);
        void deliver(const typename broadcast <type> :: batch &);

        // Services

        promise <void> accept(std :: weak_ptr <arc>, sampler <channels>);
        promise <void> keepalive(std :: weak_ptr <arc>);

        promise <void> clientsend(std :: weak_ptr <client>);
        promise <void> clientreceive(std :: weak_ptr <arc>, std :: weak_ptr <client>);

        promise <void> serversend(std :: weak_ptr <server>);
        promise <void> serverreceive(std :: weak_ptr <arc>, std :: weak_ptr <server>);
    };

    template <typename type> struct secure <type> :: index
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

    template <typename type> struct secure <type> :: client
    {
        connection connection;
        pipe <optional <hash>> responses;
    };

    template <typename type> struct secure <type> :: server
    {
        connection connection;
        pipe <hash> queries;
    };

    template <typename type> class secure <type> :: arc
    {
        // Friends

        template <typename> friend class secure;

        // Members

        sampler <channels> _sampler;

        std :: unordered_map <index, type, shorthash> _messages;
        std :: unordered_map <hash, typename broadcast <type> :: batch, shorthash> _batches;

        std :: unordered_map <hash, size_t, shorthash> _quorums;

        std :: vector <std :: shared_ptr <client>> _clients;
        std :: vector <std :: shared_ptr <server>> _servers;

        std :: unordered_map <hash, std :: vector <std :: weak_ptr <client>>, shorthash> _subscriptions;

        struct
        {
            std :: vector <std :: function <void (const hash &)>> spot;
            std :: vector <std :: function <void (const typename broadcast <type> :: batch &)>> deliver;
        } _handlers;

        consistent <type> _consistent;
        guard <simple> _guard;

    public:

        // Constructors

        arc(const sampler <channels> &, const int &);
    };
};

#endif
