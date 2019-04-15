#include "framework/test.hpp"

// Libraries

#include <iostream>
#include <string>

#include <drop/network/connection.hpp>
#include <drop/network/tcp.hpp>

// Includes

#include "psb/broadcast/broadcast.hpp"

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

    void cli(broadcast <std :: string> & broadcast)
    {
        while(true)
        {
            std :: string message;
            std :: getline(std :: cin, message);

            signer signer;
            broadcast.publish(signer.publickey(), rand(), message, signer.sign(message));
        }
    }

    // Tests

    $test("broadcast/sponge", []
    {
        broadcast <std :: string> :: configuration :: sponge :: capacity = 2;

        broadcast <std :: string> broadcast;

        signer feed;
        uint32_t sequence = 0;

        auto publish = [&](const std :: string & message)
        {
            broadcast.publish(feed.publickey(), sequence, message, feed.sign(sequence, message));
            sequence++;
        };

        publish("Hello World!");
        publish("Nice to be around!");
        publish("Would like to chat a bit!");
        publish("But probably I should hang up for this batch. Byeee!");

        publish("And here we are in the next batch!");
        sleep(1_s);
        publish("Still there?");

        sleep(10_s);
    });

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
            for(uint32_t sequence = 0; sequence < block.size(); sequence++)
                std :: cout << " -> " << block[sequence].feed << "." << block[sequence].sequence << ": " << block[sequence].payload << std :: endl;

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

    $test("broadcast/alice", []
    {
        broadcast <std :: string> :: configuration :: sponge :: capacity = 2;
        broadcast <std :: string> mybroadcast;
        mybroadcast.on <broadcast <std :: string> :: batch> (deliver);

        std :: thread receiver([&]()
        {
            auto listener = tcp :: listen(1234);

            auto connection = listener.acceptsync();
            connection.securesync <peer> (keyexchanger());

            std :: cout << "Connection incoming. Establishing link." << std :: endl;

            mybroadcast.link(connection);
        });

        cli(mybroadcast);
    });

    $test("broadcast/bob", []
    {
        broadcast <std :: string> :: configuration :: sponge :: capacity = 2;
        broadcast <std :: string> mybroadcast;
        mybroadcast.on <broadcast <std :: string> :: batch> (deliver);

        auto connection = tcp :: connectsync({"127.0.0.1", 1234});
        connection.securesync <peer> (keyexchanger());

        mybroadcast.link(connection);

        cli(mybroadcast);
    });
};
