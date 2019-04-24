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

    struct synchronizer
    {
        static constexpr uint16_t port = 1235;
    };
};

// Using

using namespace psb;
using namespace drop;

// Functions

void rendezvous(const int & expected)
{
    directory directory(settings :: directory :: port);

    auto listener = tcp :: listen(settings :: synchronizer :: port);

    while(true)
    {
        std :: vector <connection> peers;
        peers.reserve(expected);

        for(int peer = 0; peer < expected; peer++)
        {
            std :: cout << "Waiting for peer " << peer << " / " << expected << std :: endl;
            peers.push_back(listener.acceptsync());
        }

        std :: cout << "All peers connected." << std :: endl;

        for(const auto & connection : peers)
            connection.sendsync(true);

        sleep(10_s);
    }
}

void peer(const int & id, const class address :: ip & directory, const int & sources, const int & broadcasts, const interval & period, const int & batchsize, const interval & timeout)
{
    broadcast <timestamp> :: configuration :: sponge :: capacity = batchsize;
    broadcast <timestamp> :: configuration :: sponge :: timeout = timeout;

    std :: cout << "Starting sampler." << std :: endl;

    auto sampler = directory :: sample <channels> ({directory, settings :: directory :: port});

    tcp :: connectsync({directory, settings :: synchronizer :: port}).receivesync <bool> ();

    sleep(20_s);

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

    tcp :: connectsync({directory, settings :: synchronizer :: port}).receivesync <bool> ();

    sleep(20_s);
}

int main(int argc, const char ** args)
{
    signal(SIGPIPE, SIG_IGN);

    auto usage = []()
    {
        std :: cout << "Usage: " << std :: endl;
        std :: cout << "\t./demo.out rendezvous [expected]" << std :: endl;
        std :: cout << "\t./demo.out peer [rendezvous ip] [sources] [broadcasts] [broadcasts/second] [batch size] [release timeout] [peer id]" << std :: endl;
        return -1;
    };

    if(argc < 3)
        return usage();

    if(strcmp(args[1], "rendezvous") && strcmp(args[1], "peer"))
        return usage();

    if(!strcmp(args[1], "rendezvous"))
        rendezvous(std :: stoi(args[2]));
    else
    {
        if(argc < 9)
            return usage();

        try
        {
            class address :: ip directory(args[2]);
            int sources = std :: stoi(args[3]);
            int broadcasts = std :: stoi(args[4]);
            double frequency = std :: stof(args[5]);
            int batchsize = std :: stoi(args[6]);
            double timeout = std :: stof(args[7]);
            int id = std :: stoi(args[8]);

            :: peer(id, directory, sources, broadcasts, 1_s / frequency, batchsize, 1_s * timeout);
        }
        catch(...)
        {
            return usage();
        }
    }

}
