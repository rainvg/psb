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

    $test("broadcast/develop", []
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
};
