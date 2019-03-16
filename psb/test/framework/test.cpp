// Includes

#include "test.hpp"

namespace test
{
    // test

    // Static members

    std :: unordered_map <std :: string, struct test :: configuration> * test :: singleton = nullptr;

    // Static methods

    void test :: run(const std :: string & name)
    {
        struct configuration test = tests().at(name);
        test.test->run();
    }

    struct test :: configuration test :: configuration(const std :: string & name)
    {
        return tests().at(name);
    }

    std :: vector <std :: string> test :: enumerate()
    {
        std :: vector <std :: string> names;

        for(auto const & [name, test] : tests())
            names.push_back(name);

        std :: sort(names.begin(), names.end());

        return names;
    }

    // interface

    // Destructor

    test :: interface :: ~interface()
    {
    }

    // configuration

    // Ostream integration

    std :: ostream & operator << (std :: ostream & out, const class test :: configuration & configuration)
    {
        out << "{" << "\"instances\": " << configuration.instances << "}";

        return out;
    }
};
