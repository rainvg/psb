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

    $test("directory/develop", {.instances = 3}, []
    {
        if(:: test :: instance :: id() == 0)
        {
            directory directory(1234);
            sleep(2_m);
        }
        else if(:: test :: instance :: id() == 1)
        {
            sleep(1_s);

            auto alice = directory :: sample <channels> ({:: test :: instance :: get <:: test :: IPv4> (0), 1234});
            sleep(2_m);
        }
        else {
            sleep(1_s);

            auto bob = directory :: sample <channels> ({:: test :: instance :: get <:: test :: IPv4> (0), 1234});
            sleep(5_s);
        }
    });
};
