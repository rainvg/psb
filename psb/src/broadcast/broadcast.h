// Forward declarations

namespace psb
{
    template <typename> class broadcast;
};

#if !defined(__forward__) && !defined(__src__broadcast__broadcast__h)
#define __src__broadcast__broadcast__h

// Libraries

#include <memory>
#include <vector>
#include <unordered_map>

#include <drop/crypto/signature.hpp>
#include <drop/crypto/hash.hpp>
#include <drop/bytewise/bytewise.hpp>

// Includes

#include "psb/sampler/sampler.hpp"

namespace psb
{
    using namespace drop;

    template <typename type> class broadcast
    {
        // Static asserts

        static_assert(bytewise :: constraints :: serializable <type> () && bytewise :: constraints :: deserializable <type> (), "Broadcast type must be serializable and deserializable.");

        // Service nested structs

        struct message;
        struct blockid;
        struct batchinfo;
        struct batch;
        struct announcement;
        struct transfer;

        // Service nested classes

        class sponge;
        class batchset;
        class blockmask;
        class link;

        class arc;
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
    };

    template <typename type> struct broadcast <type> :: blockid
    {
        hash hash;
        uint32_t sequence;
    };

    template <typename type> struct broadcast <type> :: batch
    {
        batchinfo info;
        std :: vector <std :: shared_ptr <std :: vector <message>>> blocks;
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
};

#endif
