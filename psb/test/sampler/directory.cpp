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
        directory directory({"127.0.0.1", 1235});

        auto alice = directory :: sample <channels> ({"127.0.0.1", 1235});

        sleep(5_s);

        auto bob = directory :: sample <channels> ({"127.0.0.1", 1235});

        sleep(1_h);
    });
};
