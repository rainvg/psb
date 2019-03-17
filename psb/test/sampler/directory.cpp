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

    $test("directory/server", []
    {
        directory directory(1234);
        sleep(2_h);
    });

    $test("directory/client", []
    {
        auto client = directory :: sample <channels> ({"127.0.0.1", 1234});
        sleep(2_h);
    });
};
