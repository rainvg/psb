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
#include "broadcast/broadcast.arc.hpp"

namespace psb
{
    using namespace drop;

    // Configuration

    template <typename type> size_t broadcast <type> :: configuration :: sponge :: capacity = 256;
    template <typename type> interval broadcast <type> :: configuration :: sponge :: timeout = 5_s;
    template <typename type> double broadcast <type> :: configuration :: link :: lambda = 0.1;

    template <typename type> size_t broadcast <type> :: configuration :: lanes :: fast :: links = 5;
    template <typename type> size_t broadcast <type> :: configuration :: lanes :: fast :: requests = 2;

    template <typename type> size_t broadcast <type> :: configuration :: lanes :: fast :: churn :: period = 10;
    template <typename type> double broadcast <type> :: configuration :: lanes :: fast :: churn :: percentile = 0.2;

    template <typename type> size_t broadcast <type> :: configuration :: lanes :: secure :: links :: max = 8;
    template <typename type> size_t broadcast <type> :: configuration :: lanes :: secure :: links :: min = 6;
    template <typename type> size_t broadcast <type> :: configuration :: lanes :: secure :: requests = 3;

    // Constructors

    template <typename type> broadcast <type> :: broadcast(const sampler <channels> & sampler, const int & id) : _arc(std :: make_shared <arc> (sampler, id))
    {
        this->drive(this->_arc);
        this->accept(this->_arc, sampler);

        [](std :: weak_ptr <arc> warc) -> promise <void>
        {
            while(true)
            {
                co_await wait(settings :: drive :: wakeinterval);
                if(auto arc = warc.lock())
                    arc->_pipe.post();
                else
                    break;
            }
        }(this->_arc);
    }

    // Private constructors

    template <typename type> broadcast <type> :: broadcast(const std :: shared_ptr <arc> & arc) : _arc(arc)
    {
    }

    // Private getters

    template <typename type> std :: vector <hash> broadcast <type> :: proof(const hash & batch) const
    {
        return this->_arc->_guard([&]()
        {
            return this->_arc->_proofs[batch];
        });
    }

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

                std :: cout << "Transfer for " << batch.hash << " has now size " << this->_arc->_transfers[batch.hash].providers.size() << std :: endl;

                this->_arc->_priority.push(batch.hash);

                std :: cout << "Priority updated:" << std :: endl;
                for(const auto & hash : this->_arc->_priority)
                    std :: cout << "\t" << hash << std :: endl;

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

        for(const auto & link : this->_arc->_links.guest)
            link->announce(announcement);
    }

    template <typename type> void broadcast <type> :: available(const hash & hash, const std :: shared_ptr <class link> & link)
    {
        std :: cout << "Batch " << hash << " available on link " << link->id() << std :: endl;

        bool post = false;
        this->_arc->_guard([&]()
        {
            auto transfer = this->_arc->_transfers.find(hash);

            if(transfer != this->_arc->_transfers.end())
            {
                for(uint32_t sequence = 0; sequence < transfer->second.providers.size(); sequence++)
                {
                    auto block = transfer->second.providers.find(sequence);
                    if(block != transfer->second.providers.end())
                    {
                        block->second.push_back(link);

                        if(this->_arc->_providers.find(link) != this->_arc->_providers.end())
                            this->_arc->_providers[link].insert({.hash = hash, .sequence = sequence});

                        post = true;
                    }
                }
            }
        });

        if(post)
            this->_arc->_pipe.post();
    }

    template <typename type> void broadcast <type> :: available(const blockid & block, const std :: shared_ptr <class link> & link)
    {
        std :: cout << "Block " << block.hash << "." << block.sequence << " available on link " << link->id() << std :: endl;

        bool post = false;
        this->_arc->_guard([&]()
        {
            auto transfer = this->_arc->_transfers.find(block.hash);

            if(transfer != this->_arc->_transfers.end())
            {
                auto provider = transfer->second.providers.find(block.sequence);
                if(provider != transfer->second.providers.end())
                {
                    provider->second.push_back(link);

                    if(this->_arc->_providers.find(link) != this->_arc->_providers.end())
                        this->_arc->_providers[link].insert(block);

                    post = true;
                }
            }
        });

        if(post)
            this->_arc->_pipe.post();
    }

    template <typename type> void broadcast <type> :: dispatch(const blockid & blockid,  const std :: vector <hash> & proof, const class block & block, const std :: shared_ptr <class link> & link)
    {
        uint32_t size;

        bool deliver = this->_arc->_guard([&]()
        {
            this->_arc->_requests.all.erase(blockid);
            this->_arc->_requests.secure.erase(blockid);

            if(this->_arc->_proofs.find(blockid.hash) == this->_arc->_proofs.end())
                this->_arc->_proofs[blockid.hash] = proof;

            if(this->_arc->_blocks.find(blockid) == this->_arc->_blocks.end())
            {
                std :: cout << "Block " << blockid.hash << "." << blockid.sequence << " obtained." << std :: endl;
                std :: cout << "Link " << link->id() << " has a latency of " << link->latency() << std :: endl;
                this->_arc->_blocks[blockid] = block;
            }

            if(link && (this->_arc->_links.fast.find(link) != this->_arc->_links.fast.end()))
            {
                this->_arc->_churn.trigger++;

                if(link->requests() < configuration :: lanes :: fast :: requests)
                {
                    std :: cout << "Adding " << link->id() << " to idle." << std :: endl;
                    this->_arc->_links.idle.insert(link);
                }
            }

            auto transfer = this->_arc->_transfers.find(blockid.hash);
            if(transfer != this->_arc->_transfers.end())
            {
                for(const auto & link : this->_arc->_links.fast)
                    link->advertise(blockid);

                for(const auto & link : this->_arc->_links.secure)
                    link->advertise(blockid);

                for(const auto & link : this->_arc->_links.guest)
                    link->advertise(blockid);

                transfer->second.providers.erase(blockid.sequence);

                std :: cout << "Missing blocks: " << transfer->second.providers.size() << std :: endl;
                if(transfer->second.providers.size() == 0)
                {
                    size = transfer->second.size;

                    this->_arc->_transfers.erase(blockid.hash);
                    this->_arc->_priority.remove(blockid.hash);

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

        this->_arc->_pipe.post();
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

            return this->_arc->_handlers;
        });

        for(const auto & handler : handlers)
            handler(batch);
    }

    template <typename type> void broadcast <type> :: release(const std :: vector <class block> & blocks)
    {
        std :: cout << "Releasing " << blocks.size() << " blocks:" << std :: endl;

        std :: vector <hash> proof;

        for(const auto & block : blocks)
        {
            hash :: state hasher;
            for(const auto & message : block)
            {
                hasher.update(message);
                std :: cout << " -> " << message.feed << "." << message.sequence << ": " << message.payload << std :: endl;
            }

            proof.push_back(hasher.finalize());
        }

        batchinfo info = {.hash = proof, .size = static_cast <uint32_t> (blocks.size())};
        std :: cout << "Batch info: " << info.hash << " (" << info.size << ")" << std :: endl;

        enum {delivered, transferring, released} state = this->_arc->_guard([&]()
        {
            if(this->_arc->_delivered.find(info))
                return delivered;

            if(this->_arc->_transfers.find(info.hash) != this->_arc->_transfers.end())
                return transferring;

            this->_arc->_proofs[info.hash] = proof;

            for(uint32_t sequence = 0; sequence < info.size; sequence++)
                this->_arc->_blocks[{.hash = info.hash, .sequence = sequence}] = blocks[sequence];

            this->announce({.batch = info, .available = true});
            return released;
        });

        if(state == transferring)
        {
            for(uint32_t sequence = 0; sequence < info.size; sequence++)
                this->dispatch({.hash = info.hash, .sequence = sequence}, proof, blocks[sequence], nullptr);
        }
        else if(state == released)
            this->deliver(info);
    }

    template <typename type> template <enum broadcast <type> :: lane linklane, typename... connections> promise <void> broadcast <type> :: link(const connections & ... incoming)
    {
        std :: weak_ptr <arc> warc = this->_arc;

        auto sampler = this->_arc->_guard([&]()
        {
            if constexpr (linklane == fast)
                (this->_arc->_handshakes.fast)++;
            else if constexpr (linklane == secure)
                (this->_arc->_handshakes.secure)++;

            this->_arc->_delivered.lock();
            return this->_arc->_sampler;
        });

        try
        {
            auto connection = co_await [&]() -> promise <class connection>
            {
                if constexpr (sizeof...(incoming))
                {
                    co_return [](const auto & connection)
                    {
                        return connection;
                    }(incoming...);
                }
                else
                    co_return co_await sampler.template connect <gossip> ();
            }();

            co_await connection.send(this->_arc->_id);
            auto id = co_await connection.template receive <int> ();

            auto link = std :: make_shared <class link> (connection, id);

            std :: cout << "Linking <" << std :: array <const char *, 3> {"fast", "secure", "guest"}[linklane] << "> " << connection.remote() << ": " << link->id() << std :: endl;

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

                    if constexpr (linklane == fast)
                        (this->_arc->_handshakes.fast)--;
                    else if constexpr (linklane == secure)
                        (this->_arc->_handshakes.secure)--;

                    arc->_delivered.unlock();

                    if constexpr (linklane == fast)
                    {
                        arc->_links.fast.insert(link);
                        arc->_links.idle.insert(link);

                        arc->_providers[link] = std :: unordered_set <blockid, shorthash> ();
                    }
                    else if constexpr (linklane == secure)
                        arc->_links.secure.insert(link);
                    else
                        arc->_links.guest.insert(link);
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
                    if constexpr (linklane == fast)
                        (this->_arc->_handshakes.fast)--;
                    else if constexpr (linklane == secure)
                        (this->_arc->_handshakes.secure)--;

                    arc->_delivered.unlock();
                });
            }
        }

        if(auto arc = warc.lock())
            arc->_pipe.post();
    }

    template <typename type> void broadcast <type> :: unlink(const std :: shared_ptr <class link> & link)
    {
        this->_arc->_guard([&]()
        {
            if(this->_arc->_links.fast.erase(link))
                std :: cout << "Unlinking  <fast> : " << link->id() << std :: endl;
            if(this->_arc->_links.secure.erase(link))
                std :: cout << "Unlinking  <secure> : " << link->id() << std :: endl;
            this->_arc->_links.idle.erase(link);

            if(this->_arc->_links.guest.erase(link))
                std :: cout << "Unlinking  <guest> : " << link->id() << std :: endl;

            this->_arc->_providers.erase(link);
        });

        this->_arc->_pipe.post();
    }

    // Services

    template <typename type> promise <void> broadcast <type> :: drive(std :: weak_ptr <arc> warc)
    {
        while(auto arc = warc.lock())
        {
            struct
            {
                size_t fast;
                size_t secure;
            } handshakes;

            this->_arc->_guard([&]()
            {
                std :: cout << "Verifying churn: treshold is " << (configuration :: lanes :: fast :: churn :: period * configuration :: lanes :: fast :: links) << ", trigger is " << this->_arc->_churn.trigger << std :: endl;
                if(this->_arc->_churn.trigger >= (configuration :: lanes :: fast :: churn :: period * configuration :: lanes :: fast :: links))
                {
                    this->_arc->_churn.trigger = 0;

                    std :: vector <std :: shared_ptr <class link>> links;
                    for(const auto & link : this->_arc->_links.fast)
                    {
                        links.push_back(link);
                        std :: cout << " -> " << link->id() << " has latency " << link->latency() << std :: endl;
                    }

                    std :: sort(links.begin(), links.end(), [&](const std :: shared_ptr <class link> & lho, const std :: shared_ptr <class link> & rho)
                    {
                        return lho->latency() > rho->latency();
                    });

                    for(size_t index = 0; index < configuration :: lanes :: fast :: churn :: percentile * links.size(); index++)
                    {
                        std :: cout << "Unlinking slow link " << links[index]->id() << " from fast lane, its latency is " << links[index]->latency() << std :: endl;
                        links[index]->shutdown();
                    }
                }

                handshakes.fast = configuration :: lanes :: fast :: links - this->_arc->_handshakes.fast - this->_arc->_links.fast.size();

                if((this->_arc->_handshakes.secure == 0) && (this->_arc->_links.secure.size() < configuration :: lanes :: secure :: links :: min))
                {
                    for(const auto & link : this->_arc->_links.secure)
                        link->shutdown();

                    this->_arc->_links.secure.clear();
                    handshakes.secure = configuration :: lanes :: secure :: links :: max;
                }
                else
                    handshakes.secure = 0;

                size_t requests = configuration :: lanes :: secure :: requests - this->_arc->_requests.secure.size();
                std :: cout << std :: endl << "Processing secure lane: " << requests << " requests missing." << std :: endl;

                if(requests)
                {
                    [&]()
                    {
                        for(const auto & hash : this->_arc->_priority)
                        {
                            std :: cout << "Processing batch " << hash << " which has transfer of size " << this->_arc->_transfers[hash].providers.size() << std :: endl;

                            std :: vector <uint32_t> sequences;
                            for(const auto & [sequence, providers] : this->_arc->_transfers[hash].providers)
                            {
                                std :: cout << "Sequence " << sequence << " has " << providers.size() << " providers." << std :: endl;
                                if(providers.size() && (this->_arc->_requests.all.find({.hash = hash, .sequence = sequence}) == this->_arc->_requests.all.end()))
                                {
                                    std :: cout << "Available and not yet requeted block: " << sequence << std :: endl;
                                    sequences.push_back(sequence);
                                }
                            }

                            if(!sequences.size())
                            {
                                std :: cout << "Nothing interesting in this batch." << std :: endl;
                                continue;
                            }

                            shorthash shorthash;
                            std :: sort(sequences.begin(), sequences.end(), [&](const uint32_t & lho, const uint32_t & rho)
                            {
                                return shorthash(lho) > shorthash(rho);
                            });

                            for(const auto & sequence : sequences)
                            {
                                struct
                                {
                                    std :: shared_ptr <class link> provider;
                                    interval latency;
                                } best {.latency = std :: numeric_limits <uint64_t> :: max()};

                                std :: cout << "looking for best provider for block: " << sequence << std :: endl;

                                for(const auto & wlink : this->_arc->_transfers[hash].providers[sequence])
                                {
                                    if(auto provider = wlink.lock())
                                    {
                                        if(provider->latency() * provider->requests() < best.latency)
                                        {
                                            std :: cout << "New best found: " << provider->id() << std :: endl;
                                            best = {.latency = provider->latency() * provider->requests(), .provider = provider};
                                        }
                                    }
                                }

                                if(best.latency < std :: numeric_limits <uint64_t> :: max())
                                {
                                    try
                                    {
                                        blockid blockid = {.hash = hash, .sequence = sequence};
                                        best.provider->request(blockid);
                                        std :: cout << "Secure request done: " << hash << "." << sequence << std :: endl;
                                        this->_arc->_requests.all.insert(blockid);
                                        this->_arc->_requests.secure.insert(blockid);

                                        if(!(--requests))
                                            return;
                                    }
                                    catch(...)
                                    {
                                        std :: cout << "Something went wrong while doing secure request." << std :: endl;
                                    }
                                }
                            }
                        }
                    }();
                }

                std :: cout << std :: endl;


                std :: cout << std :: endl << "Processing fast lane" << std :: endl;
                for(size_t requests = 0; requests < configuration :: lanes :: fast :: requests; requests++)
                {
                    for(const auto & idle : this->_arc->_links.idle)
                    {
                        std :: cout << "Idle loop: " << idle->id() << std :: endl;
                        if(idle->requests() == requests)
                        {
                            std :: cout << "Link has " << requests << " requests." << std :: endl;
                            struct
                            {
                                hash hash;
                                std :: vector <uint32_t> sequences;
                                uint64_t priority;
                            } best {.priority = std :: numeric_limits <uint64_t> :: max()};

                            std :: vector <blockid> pop;

                            for(const auto & blockid : this->_arc->_providers[idle])
                            {
                                std :: cout << "Block " << blockid.hash << "." << blockid.sequence << " available." << std :: endl;
                                if((this->_arc->_transfers.find(blockid.hash) == this->_arc->_transfers.end()) || (this->_arc->_transfers[blockid.hash].providers.find(blockid.sequence) == this->_arc->_transfers[blockid.hash].providers.end()))
                                {
                                    std :: cout << "Already obtained." << std :: endl;
                                    pop.push_back(blockid);
                                    continue;
                                }

                                if(this->_arc->_requests.all.find(blockid) != this->_arc->_requests.all.end())
                                {
                                    std :: cout << "Already requested." << std :: endl;
                                    continue;
                                }

                                if(blockid.hash == best.hash)
                                {
                                    std :: cout << "Hash already seen." << std :: endl;
                                    best.sequences.push_back(blockid.sequence);
                                }
                                else if(this->_arc->_priority[blockid.hash] < best.priority)
                                {
                                    std :: cout << "Hash has higher priority." << std :: endl;
                                    best.hash = blockid.hash;
                                    best.sequences.clear();
                                    best.sequences.push_back(blockid.sequence);
                                    best.priority = this->_arc->_priority[blockid.hash];
                                }
                            }

                            if(best.priority < std :: numeric_limits <uint64_t> :: max())
                            {
                                std :: cout << "Requesting block one block out of " << best.sequences.size() << " availables." << std :: endl;
                                blockid blockid = {.hash = best.hash, .sequence = best.sequences[rand() % best.sequences.size()]};
                                std :: cout << "Requesting " << blockid.hash << "." << blockid.sequence << std :: endl;
                                idle->request(blockid);
                                this->_arc->_requests.all.insert(blockid);
                            }

                            for(const auto & blockid : pop)
                            {
                                std :: cout << "Popping block " << blockid.hash << "." << blockid.sequence << std :: endl;
                                this->_arc->_providers[idle].erase(blockid);
                            }
                        }
                    }
                }
                std :: cout << std :: endl;

                std :: vector <std :: shared_ptr <class link>> pop;
                for(const auto & idle : this->_arc->_links.idle)
                {
                    if(idle->requests() >= configuration :: lanes :: fast :: requests)
                        pop.push_back(idle);
                }

                for(const auto & idle : pop)
                    this->_arc->_links.idle.erase(idle);
            });

            for(size_t handshake = 0; handshake < handshakes.fast; handshake++)
                this->link <fast> ();

            for(size_t handshake = 0; handshake < handshakes.secure; handshake++)
                this->link <secure> ();

            co_await arc->_pipe.wait();
        }
    }

    template <typename type> promise <void> broadcast <type> :: accept(std :: weak_ptr <arc> warc, sampler <channels> sampler)
    {
        while(true)
        {
            auto connection = co_await sampler.accept <gossip> ();

            if(auto arc = warc.lock())
            {
                broadcast broadcast = arc;
                broadcast.link <guest> (connection);
            }
            else
                break;
        }
    }
};

#endif
