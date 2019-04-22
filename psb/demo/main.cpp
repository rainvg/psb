// Libraries

#include <iostream>
#include <string>
#include <fstream>

#include <drop/network/connection.hpp>
#include <drop/network/tcp.hpp>

// std :: cout mutex

std :: mutex cmtx;

// Includes

#include "psb/broadcast/consistent.hpp"
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



int main()
{
    auto sampler = directory :: sample <channels> ({"127.0.0.1", 1234});
    consistent <uint64_t> myconsistent(sampler, 0);

    broadcast <uint64_t> :: batch batch;
    signer signer;
    hash :: state hasher;

    std :: vector <broadcast <uint64_t> :: message> messages;
    messages.reserve(2048);

    for(size_t block = 0; block < 4; block++)
    {
        for(uint32_t sequence = block * 2048; sequence < (block + 1) * 2048; sequence++)
            messages.push_back({signer.publickey(), sequence, sequence, signer.sign(sequence, static_cast <uint64_t> (sequence))});

        messages.back() = {signer.publickey(), 0, block, signer.sign(uint32_t(0), static_cast <uint64_t> (block))};
        batch.blocks.push_back(messages);

        hasher.update(messages);
        messages.clear();
    }

    batch.info = {.hash = hasher.finalize(), .size = 4};

    myconsistent.dispatch(myconsistent._arc, batch);

    sleep(1_h);
}

// Functions

/*
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
    std :: string filename = "app-logs/" + std :: to_string(id) + ".txt";

    log.open(filename, std :: ios :: out);

    guard <simple> fileguard;

    mybroadcast.on <broadcast <uint64_t> :: batch> ([&](const auto & batch)
    {
        fileguard([&](){
            log << (uint64_t) now() << " " << batch.info.hash << ":" << batch.info.size << std :: endl;
        });
    });

    if(id < sources)
    {
        signer signer;
        for(uint64_t sequence = 0; sequence < broadcasts; sequence++)
        {
            mybroadcast.publish(signer.publickey(), sequence, sequence, signer.sign(sequence, static_cast <uint64_t> (sequence)));
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
*/
