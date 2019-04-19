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

void peer(const int & id, const class address :: ip & directory, const interval & setuptime, const int & sources, const int & broadcasts, const interval & period, const int & batchsize, const interval & timeout)
{
    broadcast <uint64_t> :: configuration :: sponge :: capacity = batchsize;
    broadcast <uint64_t> :: configuration :: sponge :: timeout = timeout;

    std :: cout << "Starting sampler." << std :: endl;

    auto sampler = directory :: sample <channels> ({directory, settings :: directory :: port});

    sleep(setuptime);

    std :: cout << "Starting broadcast." << std :: endl;

    broadcast <uint64_t> mybroadcast(sampler, id);

    std :: ofstream log;
    std :: string filename = "logs/" + std :: to_string(id) + ".txt";

    log.open(filename, std :: ios :: out);

    guard <simple> fileguard;

    mybroadcast.on <broadcast <uint64_t> :: batch> ([&](const auto & batch)
    {
        fileguard([&](){
            log << batch.info.hash << ":" << batch.info.size << std :: endl;
        });
    });

    if(id < sources)
    {
        signer signer;
        for(uint64_t sequence = 0; sequence < broadcasts; sequence++)
        {
            mybroadcast.publish(signer.publickey(), sequence, sequence, signer.sign(sequence));
            sleep(period);
        }
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
        std :: cout << "\t./demo.out peer [rendezvous ip] [setup time] [sources] [broadcasts] [broadcasts/second] [batch size] [release timeout] [peer id]" << std :: endl;
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
        if(argc < 10)
            return usage();

        try
        {
            class address :: ip directory(args[2]);
            double setuptime = std :: stof(args[3]);
            int sources = std :: stoi(args[4]);
            int broadcasts = std :: stoi(args[5]);
            double frequency = std :: stof(args[6]);
            int batchsize = std :: stoi(args[7]);
            double timeout = std :: stof(args[8]);
            int id = std :: stoi(args[9]);

            :: peer(id, directory, 1_s * setuptime, sources, broadcasts, 1_s / frequency, batchsize, 1_s * timeout);
        }
        catch(...)
        {
            return usage();
        }
    }

}
