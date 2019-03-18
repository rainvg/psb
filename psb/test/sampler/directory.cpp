#include "framework/test.hpp"

// Libraries

#include <iostream>
#include <set>
#include <unordered_set>

// Includes

#include "psb/sampler/directory.hpp"

namespace
{
    // Using

    using namespace psb;

    // Enums

    enum channels {alpha, beta};

    // Tests

    $test("directory/shutdown", []
    {
        {
            std :: cout << "Creating directory" << std :: endl;
            directory directory(1234);

            {
                sleep(1_s);
                std :: cout << "Creating alice" << std :: endl;
                auto alice = directory :: sample <channels> ({"127.0.0.1", 1234});

                {
                    sleep(3_s);
                    std :: cout << "Creating bob" << std :: endl;
                    auto bob = directory :: sample <channels> ({"127.0.0.1", 1234});
                    sleep(15_s);
                    std :: cout << "Deleting bob" << std :: endl;
                }

                sleep(45_s);
                std :: cout << "Deleting alice" << std :: endl;
            }

            sleep(35_s);
            std :: cout << "Deleting directory" << std :: endl;
        }

        sleep(40_s);
        std :: cout << "Everything successful, shutting down." << std :: endl;
    });

    $test("directory/develop", []
    {
        std :: vector <sampler <channels>> samplers;

        for(size_t i = 0; i < 16; i++)
        {
            samplers.push_back(directory :: sample <channels> ({"127.0.0.1", 1234}));

            [&]() -> promise <void>
            {
                struct {size_t i;} local{.i = i};

                auto connection = co_await samplers.back().accept <alpha> ();
                std :: cout << "Connection received on " << local.i << "!" << std :: endl;

                co_await connection.send <std :: string> ("Here is the server... can you hear me?");
                std :: cout << co_await connection.receive <std :: string> () << std :: endl;
                co_await wait(1_s);
            }();
        }

        [&]() -> promise <void>
        {
            std :: cout << "Attempting connection.." << std :: endl;
            auto connection = co_await samplers.front().connect <alpha> ();
            std :: cout << "Connection established!" << std :: endl;

            co_await connection.send <std :: string> ("Here is the client... can you hear me?");
            std :: cout << co_await connection.receive <std :: string> () << std :: endl;
            co_await wait(1_s);
        }();

        sleep(10_s);

        directory directory(1234);

        sleep(1_h);
    });
};
