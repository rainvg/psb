#include "framework/test.hpp"

// Libraries

#include <iostream>
#include <string>
#include <fstream>

#include <drop/network/connection.hpp>
#include <drop/network/tcp.hpp>

// Includes

#include "psb/broadcast/broadcast.hpp"
#include "psb/sampler/directory.hpp"

namespace
{
    // Using

    using namespace psb;

    // Functions

    void testblockmask(const double & churn, const uint32_t & batchmin, const uint32_t & batchmax, const uint64_t & iterations)
    {
        broadcast <std :: string> :: blockmask alice;
        broadcast <std :: string> :: blockmask bob;

        std :: set <broadcast <std :: string> :: blockid> blocks;

        for(uint64_t iteration = 0; iteration < iterations; iteration++)
        {
            broadcast <std :: string> :: batchinfo batchinfo{.hash = iteration, .size = (batchmin + rand() % (batchmax - batchmin))};

            alice.push(batchinfo);
            bob.push(batchinfo);

            for(uint32_t sequence = 0; sequence < batchinfo.size; sequence++)
                blocks.insert({.hash = batchinfo.hash, .sequence = sequence});

            uint32_t maxchurn = double(blocks.size()) * churn;
            uint32_t iterchurn = maxchurn ? (rand() % maxchurn) : maxchurn;

            std :: unordered_set <broadcast <std :: string> :: blockid, shorthash> alicepop;

            for(uint32_t i = 0; i < iterchurn; i++)
                alicepop.insert(*(std :: next(blocks.begin(), rand() % blocks.size())));

            for(const auto & blockid : alicepop)
                blocks.erase(blockid);

            auto offlist = alice.pop(alicepop);
            auto bobpop = bob.pop(offlist);

            if(alicepop != bobpop)
                throw "Different blockids are returned when `pop`ing a set or an offlist.";
        }
    }

    void peer(const uint32_t & iterations)
    {
        auto sampler = directory :: sample <channels> ({"127.0.0.1", 1234});

        std :: cout << "Press enter to start the peer " << getpid() << "." << std :: endl;
        std :: cin.ignore();

        broadcast <uint64_t> mybroadcast(sampler, getpid());

        std :: ofstream log;
        std :: string filename = "tmp/logs/" + std :: to_string(getpid()) + ".txt";

        log.open(filename, std :: ios :: out);

        guard <simple> fileguard;

        mybroadcast.on <broadcast <uint64_t> :: batch> ([&](const auto & batch)
        {
            fileguard([&](){
                log << batch.info.hash << ":" << batch.info.size << std :: endl;
            });
        });

        signer signer;
        for(uint64_t sequence = 0; sequence < iterations; sequence++)
        {
            mybroadcast.publish(signer.publickey(), sequence, sequence, signer.sign(sequence));
            sleep(0.2_s);
        }

        while(true)
            sleep(1_h);
    }

    // Tests

    $test("broadcast/batchset", []
    {
        broadcast <std :: string> :: batchset batchset;

        batchset.add({.hash = std :: string("cat"), .size = 4});
        batchset.add({.hash = std :: string("dog"), .size = 6});

        if(batchset.size() != 2)
            throw "Wrong size after unlocked add.";

        if(batchset.buffer().size() != 0)
            throw "Buffer is not empty after unlocked add.";

        batchset.lock();
        batchset.add({.hash = std :: string("mouse"), .size = 4});

        if(batchset.size() != 3)
            throw "Wrong size after locked add.";

        if(batchset.buffer().size() != 1 && batchset.buffer().back().hash != hash(std :: string("mouse")))
            throw "Wrong buffer after locked add.";

        batchset.lock();
        batchset.add({.hash = std :: string("hamster"), .size = 4});

        if(batchset.size() != 4)
            throw "Wrong size after locked add.";

        if(batchset.buffer().size() != 2 && batchset.buffer().back().hash != hash(std :: string("hamster")))
            throw "Wrong buffer after locked add.";

        batchset.unlock();
        batchset.add({.hash = std :: string("lizard"), .size = 4});

        if(batchset.size() != 5)
            throw "Wrong size after locked add.";

        if(batchset.buffer().size() != 3 && batchset.buffer().back().hash != hash(std :: string("lizard")))
            throw "Wrong buffer after locked add.";

        batchset.unlock();

        if(batchset.size() != 5)
            throw "Wrong size after unlocking.";

        if(batchset.buffer().size())
            throw "Buffer not flushed after unlocking.";
    });

    auto deliver = [](const auto & batch)
    {
        std :: cout << std :: endl;
        std :: cout << "Batch delivered: " << batch.info.hash << std :: endl;
        for(const auto & block : batch.blocks)
            for(const auto & message : block)
                std :: cout << " -> " << message.feed << "." << message.sequence << ": " << message.payload << std :: endl;

        std :: cout << std :: endl;
    };

    $test("broadcast/blockmask", []
    {
        testblockmask(0., 8, 12, 1024);

        testblockmask(0.1, 8, 12, 16384);
        testblockmask(0.5, 8, 12, 16384);
        testblockmask(1., 8, 12, 16384);

        testblockmask(0.1, 1024, 1536, 128);
        testblockmask(0.5, 1024, 1536, 128);
        testblockmask(1., 1024, 1536, 128);
    });

    $test("broadcast/priority", []
    {
        broadcast <uint64_t> :: priority priority;

        std :: set <uint64_t> values;
        std :: unordered_set <hash, shorthash> hashes;

        uint64_t value = 0;

        auto check = [&]()
        {
            for(const uint64_t & value : values)
                if(priority[value] != value)
                    throw "Priority mismatch.";

            size_t size = 0;
            for(const hash & hash : priority)
            {
                size++;
                if(hashes.find(hash) != hashes.end())
                    throw "Element not properly removed.";
            }

            if(size != hashes.size())
                throw "Missing element.";
        };

        for(uint64_t iteration = 0;; iteration++)
        {
            std :: cout << iteration << ": " << values.size() << std :: endl;

            uint64_t add = rand() % 100;
            for(uint64_t i = 0; i < add; i++)
            {
                priority.push(value);

                values.insert(value);
                hashes.insert(value);

                value++;
            }

            check();

            std :: vector <uint64_t> remove;
            for(const uint64_t & value : values)
            {
                if(rand() % 10 == 0)
                    remove.push_back(value);
            }

            for(const uint64_t & value : remove)
            {
                priority.remove(value);

                values.erase(value);
                hashes.erase(value);
            }

            check();
        }
    });

    $test("broadcast/rendezvous", []
    {
        directory directory(1234);

        while(true)
            sleep(1_h);
    });

    $test("broadcast/active", []
    {
        //broadcast <uint64_t> :: configuration :: lanes :: fast :: links = 0;
        peer(1500);
    });

    $test("broadcast/passive", []
    {
        peer(0);
    });
};
