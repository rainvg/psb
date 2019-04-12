#include "framework/test.hpp"

// Libraries

#include <iostream>
#include <string>

// Includes

#include "psb/broadcast/broadcast.hpp"

namespace
{
    // Using

    using namespace psb;

    $test("broadcast/develop", []
    {
        broadcast <std :: string> x;
    });
};
