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


promise <void> yo(sampler <channels> sampler)
{
    [=]() mutable -> promise <void>
    {
        co_await wait(12_s);

        while(true)
        {
            auto connection = co_await sampler.connect <gossip> ();

            co_await connection.send <std :: string> ("Hello!");

            auto response = co_await connection.receive <std :: string> ();
            std :: cout << response << std :: endl;

            co_await wait(2_s);
        }
    }();

    [=]() mutable -> promise <void>
    {
        while(true)
        {
            auto connection = co_await sampler.accept <gossip> ();

            auto request = co_await connection.receive <std :: string> ();
            std :: cout << request << std :: endl;

            co_await connection.send <std :: string> ("Ciao!");

            co_await wait(0.1_s);
        }
    }();

    co_await wait(1_h);
}

int main()
{
    directory directory(1234);

    for(size_t i = 0; i < 4; i++)
    {
        std :: cout << "Starting sampler " << i << std :: endl;
        auto sampler = directory :: sample <channels> ({"127.0.0.1", 1234});
        yo(sampler);
    }

    sleep(1_h);
}

/*
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
    std :: string filename = "app-logs/" + std :: to_string(id) + ".txt";

    log.open(filename, std :: ios :: out);

    guard <simple> fileguard;

    mybroadcast.on <deliver> ([&](const auto & batch)
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
