// Libraries

#include <iostream>
#include <string>
#include <fstream>

#include <drop/network/connection.hpp>
#include <drop/network/tcp.hpp>

// Includes

#include "psb/broadcast/broadcast.hpp"
#include "psb/sampler/directory.hpp"

// Settings

struct settings
{
    struct directory
    {
        static constexpr uint16_t port = 1234;
    };
};

// Using

using namespace psb;
using namespace drop;

// Functions

void rendezvous()
{
    directory directory(settings :: directory :: port);

    while(true)
        sleep(1_h);
}

void peer(const int & id, const class address :: ip & directory, const interval & setuptime, const uint32_t & iterations)
{
    std :: cout << "Starting sampler." << std :: endl;

    auto sampler = directory :: sample <channels> ({directory, settings :: directory :: port});

    std :: cout << "Starting broadcast." << std :: endl;

    sleep(setuptime);

    broadcast <uint64_t> mybroadcast(sampler, getpid());

    std :: ofstream log;
    std :: string filename = "tmp/logs/" + std :: to_string(id) + ".txt";

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

int main(int argc, const char ** args)
{
    auto usage = []()
    {
        std :: cout << "Usage: " << std :: endl;
        std :: cout << "\t./demo.out rendezvous" << std :: endl;
        std :: cout << "\t./demo.out peer [rendezvous ip] [peer id] [configuration file]" << std :: endl;
        return -1;
    };

    if(argc < 2)
        return usage();

    if(strcmp(args[1], "rendezvous") && strcmp(args[1], "peer"))
        return usage();

    if(!strcmp(args[1], "rendezvous"))
        rendezvous();
    else
    {
        if(argc < 5)
            return usage();

        try
        {
            class address :: ip directory(args[2]);
            int id = std :: stoi(args[3]);
        }
        catch(...)
        {
            return usage();
        }
    }

}
