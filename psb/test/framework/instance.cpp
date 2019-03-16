// Libraries

#include <iostream>
#include <fstream>

// Includes

#include "instance.h"

namespace test
{
    // Static members

    int instance :: _id = -1;
    std :: vector <std :: pair <std :: string, std :: string>> instance :: _peers;

    // Static methods

    void instance :: load(const int & id)
    {
        instance :: _id = id;
    }

    void instance :: load(const int & id, const char * path)
    {
        instance :: _id = id;

        std :: ifstream file;
        file.open(path, std :: ifstream :: in);
        std :: string ipv4, ipv6;

        while (file.peek() != EOF)
        {
            file >> ipv4 >> ipv6;

            std :: pair <std :: string, std :: string> adresses(ipv4, ipv6);
            instance :: _peers.push_back(adresses);

        }
    }

    int instance :: id()
    {
        if(instance :: _id == -1)
            throw "No membership file was specified.";

        return instance :: _id;
    }

    template <> const char * instance :: get <IPv4> (const int & id)
    {
        if(id >= instance :: _peers.size())
            throw "Invalid instance `id`.";

        return instance :: _peers[id].first.c_str();
    }

    template <> const char * instance :: get <IPv6> (const int & id)
    {
        if(id >= instance :: _peers.size())
            throw "Invalid instance `id`.";

        return instance :: _peers[id].second.c_str();
    }
};
