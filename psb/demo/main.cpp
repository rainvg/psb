// Libraries

#include <iostream>
#include <string>
#include <fstream>

#include <drop/network/connection.hpp>
#include <drop/network/tcp.hpp>

// std :: cout mutex

std :: mutex cmtx;

// Includes

#include "psb/broadcast/secure.hpp"
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
    broadcast <timestamp> :: configuration :: sponge :: capacity = batchsize;
    broadcast <timestamp> :: configuration :: sponge :: timeout = timeout;

    std :: cout << "Starting sampler." << std :: endl;

    auto sampler = directory :: sample <channels> ({directory, settings :: directory :: port});

    sleep(setuptime);

    std :: cout << "Starting broadcast." << std :: endl;

    secure <timestamp> mysecure(sampler, id);

    sleep(10_s);

    std :: ofstream log;
    std :: string filename = "app-logs/" + std :: to_string(id) + ".txt";

    log.open(filename, std :: ios :: out);

    guard <simple> fileguard;

    mysecure.on <deliver> ([&](const auto & batch)
    {
        timestamp now = drop :: now();
        interval sum = 0;
        size_t messages = 0;

        for(const auto & block : batch.blocks)
        {
            messages += block.size();
            for (const auto & message : block)
                sum = sum + (now - message.payload);
        }

        interval delay = sum / messages;

        fileguard([&](){
            log << (uint64_t) now << " " << (uint64_t) delay << " " << batch.info.hash << " " << messages << std :: endl;
        });
    });

    if(id < sources)
    {
        signer signer;
        for(uint64_t sequence = 0; sequence < broadcasts; sequence++)
        {
            timestamp now = drop :: now();
            mysecure.publish(signer.publickey(), sequence, now, signer.sign(sequence, now));
            sleep(period);
        }
    }
    else
        sleep(period * broadcasts);

    sleep(5_m);
}

int main(int argc, const char ** args)
{
    signal(SIGPIPE, SIG_IGN);

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
