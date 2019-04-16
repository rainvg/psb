#ifndef __src__broadcast__broadcast__hpp
#define __src__broadcast__broadcast__hpp

// Includes

#include "broadcast.h"
#include "broadcast/broadcast.structs.hpp"
#include "broadcast/broadcast.block.hpp"
#include "broadcast/broadcast.sponge.hpp"
#include "broadcast/broadcast.batchset.hpp"
#include "broadcast/broadcast.blockmask.hpp"
#include "broadcast/broadcast.link.hpp"
#include "broadcast/broadcast.priority.hpp"

namespace psb
{
    using namespace drop;

    // Configuration

    template <typename type> size_t broadcast <type> :: configuration :: sponge :: capacity = 256;
    template <typename type> interval broadcast <type> :: configuration :: sponge :: timeout = 5_s;

    // Constructors

    template <typename type> broadcast <type> :: broadcast() : _arc(std :: make_shared <arc> ())
    {
        this->run(this->_arc);
    }

    // Private constructors

    template <typename type> broadcast <type> :: broadcast(const std :: shared_ptr <arc> & arc) : _arc(arc)
    {
    }

    // Private getters

    template <typename type> class broadcast <type> :: block broadcast <type> :: block(const blockid & blockid) const
    {
        return this->_arc->_guard([&]()
        {
            return this->_arc->_blocks[blockid];
        });
    }

    template <typename type> const typename broadcast <type> :: batchset & broadcast <type> :: delivered() const
    {
        return this->_arc->_delivered; // This is called only when lock > 0
    }

    template <typename type> bool broadcast <type> :: announced(const hash & hash) const
    {
        return this->_arc->_announced.find(hash) != this->_arc->_announced.end();
    }

    // Methods

    template <typename type> template <typename etype, std :: enable_if_t <std :: is_same <etype, typename broadcast <type> :: batch> :: value> *> void broadcast <type> :: on(const std :: function <void (const batch &)> & handler)
    {
        this->_arc->_guard([&]()
        {
            this->_arc->_handlers.push_back(handler);
        });
    }

    template <typename type> void broadcast <type> :: publish(const class signer :: publickey & feed, const uint32_t & sequence, const type & payload, const signature & signature)
    {
        this->_arc->_sponge.push(this->_arc, {.feed = feed, .sequence = sequence, .payload = payload, .signature = signature});
    }

    // Private methods

    template <typename type> void broadcast <type> :: spot(const batchinfo & batch)
    {
        this->_arc->_guard([&]()
        {
            if(!(this->announced(batch.hash)))
            {
                std :: cout << "Spotted new batch: " << batch.hash << " (" << batch.size << ")" << std :: endl;
                this->_arc->_transfers[batch.hash] = transfer{.size = batch.size};
                for(uint32_t sequence = 0; sequence < batch.size; sequence++)
                    this->_arc->_transfers[batch.hash].providers[sequence] = std :: vector <std :: weak_ptr <class link>> ();

                this->_arc->_priority.push(batch.hash);
                this->announce({.batch = batch, .available = false});
            }
        });
    }

    template <typename type> void broadcast <type> :: announce(const announcement & announcement)
    {
        this->_arc->_announced.insert(announcement.batch.hash);

        for(const auto & link : this->_arc->_links.fast)
            link->announce(announcement);

        for(const auto & link : this->_arc->_links.secure)
            link->announce(announcement);
    }

    template <typename type> void broadcast <type> :: available(const hash & hash, const std :: shared_ptr <class link> & link)
    {
        std :: cout << "Batch " << hash << " available on link " << link << std :: endl;
        this->_arc->_guard([&]()
        {
            auto transfer = this->_arc->_transfers.find(hash);

            if(transfer != this->_arc->_transfers.end())
            {
                for(uint32_t sequence = 0; sequence < transfer->second.providers.size(); sequence++)
                {
                    auto block = transfer->second.providers.find(sequence);
                    if(block != transfer->second.providers.end())
                        block->second.push_back(link);
                }
            }
        });
    }

    template <typename type> void broadcast <type> :: available(const blockid & block, const std :: shared_ptr <class link> & link)
    {
        std :: cout << "Block " << block.hash << "." << block.sequence << " available on link " << link << std :: endl;
        this->_arc->_guard([&]()
        {
            auto transfer = this->_arc->_transfers.find(block.hash);

            if(transfer != this->_arc->_transfers.end())
            {
                auto provider = transfer->second.providers.find(block.sequence);
                if(provider != transfer->second.providers.end())
                    provider->second.push_back(link);
            }
        });
    }

    template <typename type> void broadcast <type> :: dispatch(const blockid & blockid, const class block & block)
    {
        uint32_t size;

        bool deliver = this->_arc->_guard([&]()
        {
            if(this->_arc->_blocks.find(blockid) == this->_arc->_blocks.end())
            {
                std :: cout << "Block " << blockid.hash << "." << blockid.sequence << " obtained." << std :: endl;
                this->_arc->_blocks[blockid] = block;
            }

            auto transfer = this->_arc->_transfers.find(blockid.hash);
            if(transfer != this->_arc->_transfers.end())
            {
                for(const auto & link : this->_arc->_links.fast)
                    link->advertise(blockid);

                for(const auto & link : this->_arc->_links.secure)
                    link->advertise(blockid);

                transfer->second.providers.erase(blockid.sequence);

                std :: cout << "Missing blocks: " << transfer->second.providers.size() << std :: endl;
                if(transfer->second.providers.size() == 0)
                {
                    size = transfer->second.size;
                    this->_arc->_transfers.erase(blockid.hash);
                    return true;
                }
            }

            return false;
        });

        if(deliver)
        {
            std :: cout << "Delivering batch " << blockid.hash << " (" << size << ")" << std :: endl;
            this->deliver({.hash = blockid.hash, .size = size});
        }
    }

    template <typename type> void broadcast <type> :: deliver(const batchinfo & info)
    {
        batch batch{.info = info};
        batch.blocks.reserve(info.size);

        std :: vector <std :: function <void (const struct batch &)>> handlers = this->_arc->_guard([&]()
        {
            for(uint32_t sequence = 0; sequence < info.size; sequence++)
                batch.blocks.push_back(this->_arc->_blocks[{.hash = info.hash, .sequence = sequence}]);

            this->_arc->_delivered.add(info);
            this->_arc->_priority.remove(info.hash);

            return this->_arc->_handlers;
        });

        for(const auto & handler : handlers)
            handler(batch);
    }

    template <typename type> void broadcast <type> :: release(const std :: vector <class block> & blocks)
    {
        std :: cout << "Releasing " << blocks.size() << " blocks:" << std :: endl;
        hash :: state hasher;

        for(const auto & block : blocks)
            for(const auto & message : block)
            {
                std :: cout << " -> " << message.feed << "." << message.sequence << ": " << message.payload << std :: endl;
                hasher.update(message);
            }

        batchinfo info = {.hash = hasher.finalize(), .size = static_cast <uint32_t> (blocks.size())};
        std :: cout << "Batch info: " << info.hash << " (" << info.size << ")" << std :: endl;

        enum {delivered, transferring, released} state = this->_arc->_guard([&]()
        {
            if(this->_arc->_delivered.find(info))
                return delivered;

            if(this->_arc->_transfers.find(info.hash) != this->_arc->_transfers.end())
                return transferring;

            for(uint32_t sequence = 0; sequence < info.size; sequence++)
                this->_arc->_blocks[{.hash = info.hash, .sequence = sequence}] = blocks[sequence];

            this->announce({.batch = info, .available = true});
            return released;
        });

        if(state == transferring)
        {
            for(uint32_t sequence = 0; sequence < info.size; sequence++)
                this->dispatch({.hash = info.hash, .sequence = sequence}, blocks[sequence]);
        }
        else if(state == released)
            this->deliver(info);
    }

    template <typename type> template <enum broadcast <type> :: lane linklane> promise <void> broadcast <type> :: link(const connection & connection)
    {
        std :: weak_ptr <arc> warc = this->_arc;
        auto link = std :: make_shared <class link> (connection);

        std :: cout << "Linking " << connection.remote() << ": " << link << std :: endl;

        this->_arc->_guard([&]()
        {
            this->_arc->_delivered.lock();
        });

        try
        {
            std :: vector <batchinfo> sync = co_await link->sync(warc, link);

            if(auto arc = warc.lock())
            {
                broadcast self = arc;

                for(const auto & batch : sync)
                    self.spot(batch);

                arc->_guard([&]()
                {
                    for(const auto & batch : arc->_delivered.buffer())
                        link->announce({.batch = batch, .available = true});

                    for(const auto & [hash, transfer] : arc->_transfers)
                    {
                        link->announce({.batch = {.hash = hash, .size = transfer.size}, .available = false});

                        for(uint32_t sequence = 0; sequence < transfer.size; sequence++)
                            if(transfer.providers.find(sequence) == transfer.providers.end())
                                link->advertise({.hash = hash, .sequence = sequence});
                    }

                    arc->_delivered.unlock();

                    if constexpr (linklane == fast)
                    {
                        arc->_links.fast.insert(link);
                        arc->_links.idle.insert(link);
                    }
                    else
                        arc->_links.secure.insert(link);
                });

                for(const auto & batch : sync)
                    self.available(batch.hash, link);

                link->start(warc, link);
            }
        }
        catch(...)
        {
            if(auto arc = warc.lock())
            {
                arc->_guard([&]()
                {
                    arc->_delivered.unlock();
                });
            }
        }
    }

    template <typename type> void broadcast <type> :: unlink(const std :: shared_ptr <class link> & link)
    {
        std :: cout << "Unlinking " << link << "." << std :: endl;

        this->_arc->_guard([&]()
        {
            this->_arc->_links.fast.erase(link);
            this->_arc->_links.secure.erase(link);
            this->_arc->_links.idle.erase(link);
        });
    }

    // Services

    template <typename type> promise <void> broadcast <type> :: run(std :: weak_ptr <arc> warc)
    {
        while(true)
        {
            if(auto arc = warc.lock())
            {
                arc->_guard([&]()
                {
                    for(const auto & [hash, transfer] : arc->_transfers)
                    {
                        for(const auto & [sequence, providers] : transfer.providers)
                        {
                            for(const auto & provider : providers)
                            {
                                if(auto link = provider.lock())
                                {
                                    std :: cout << "Requesting block " << hash << "." << sequence << " from " << link << std :: endl;
                                    link->request({.hash = hash, .sequence = sequence});
                                    return;
                                }
                            }
                        }
                    }
                });
            }
            else
                break;

            co_await wait(0.1_s);
        }
    }
};

#endif
