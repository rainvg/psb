#include "framework/test.hpp"

// Libraries

#include <iostream>
#include <string>

// Includes

#include "psb/broadcast/broadcast.hpp"

namespace
{
    // Using

    using namespace psb;

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
        broadcast <std :: string> :: batchset alice;

        alice.add({.hash = std :: string("cat"), .size = 4});
        alice.add({.hash = std :: string("dog"), .size = 6});

        if(alice.size() != 2)
            throw "Wrong size after unlocked add.";

        if(alice.buffer().size() != 0)
            throw "Buffer is not empty after unlocked add.";

        alice.lock();
        alice.add({.hash = std :: string("mouse"), .size = 4});

        if(alice.size() != 3)
            throw "Wrong size after locked add.";

        if(alice.buffer().size() != 1 && alice.buffer().back().hash != hash(std :: string("mouse")))
            throw "Wrong buffer after locked add.";

        alice.lock();
        alice.add({.hash = std :: string("hamster"), .size = 4});

        if(alice.size() != 4)
            throw "Wrong size after locked add.";

        if(alice.buffer().size() != 2 && alice.buffer().back().hash != hash(std :: string("hamster")))
            throw "Wrong buffer after locked add.";

        alice.unlock();
        alice.add({.hash = std :: string("lizard"), .size = 4});

        if(alice.size() != 5)
            throw "Wrong size after locked add.";

        if(alice.buffer().size() != 3 && alice.buffer().back().hash != hash(std :: string("lizard")))
            throw "Wrong buffer after locked add.";

        alice.unlock();

        if(alice.size() != 5)
            throw "Wrong size after unlocking.";

        if(alice.buffer().size())
            throw "Buffer not flushed after unlocking.";
    });
};
