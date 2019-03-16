#include <iostream>

#include "framework/test.h"

int main(int argc, char * argv[])
{
    auto usage = []
    {
        std :: cout << "usage: test enumerate" << std :: endl;
        std :: cout << "       test configuration [<test_name>]" << std :: endl;
        std :: cout << "       test run <test_name> [<instance_id>] [<membership_file]" << std :: endl;

        return -1;
    };

    if(argc < 2)
        return usage();

    if(strcmp(argv[1], "enumerate") && strcmp(argv[1], "run") && strcmp(argv[1], "configuration"))
        return usage();

    if(!strcmp(argv[1], "enumerate"))
    {
        std :: cout << "[";

        bool first = true;
        for(const auto & name : :: test :: test :: enumerate())
            std :: cout << (first ? "" : ", ") << "\"" << name << "\"", first = false;

        std :: cout << "]" << std :: endl;
        return 0;
    }

    if(!strcmp(argv[1], "run"))
    {
        if(argc < 3 || argc > 5)
            return usage();

        if(argc == 4)
            :: test :: instance :: load(atoi(argv[3]));
        else if(argc == 5)
            :: test :: instance :: load(atoi(argv[3]), argv[4]);

        try
        {
            :: test :: test :: run(argv[2]);
            return 0;
        }
        catch(const std :: exception & exception)
        {
            std :: cerr << "Test failed: " << exception.what() << std :: endl;
        }
        catch(const char * exception)
        {
            std :: cerr << "Test failed: " << exception << std :: endl;
        }
        catch(...)
        {
            std :: cerr << "Test failed: (unknown exception)" << std :: endl;
        }

        return -1;
    }

    if(!strcmp(argv[1], "configuration"))
    {
        if(argc < 3)
        {
            std :: cout << "{";

            bool first = true;
            for(const auto & name : :: test :: test :: enumerate())
            {
                std :: cout << (first ? "" : ", ") << "\"" << name << "\": ";
                first = false;
                std :: cout << :: test :: test :: configuration(name);
            }

            std :: cout << "}" << std :: endl;
        }
        else
        {
            try
            {
                std :: cout << :: test :: test :: configuration(argv[2]) << std :: endl;
            }
            catch(const std :: exception & exception)
            {
                std :: cerr << "Configuration unavailable: " << exception.what() << std :: endl;
                return -1;
            }
        }

        return 0;
    }
}
