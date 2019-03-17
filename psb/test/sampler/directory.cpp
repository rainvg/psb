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

    $test("directory/develop", []
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
};
