// Forward declarations

namespace test
{
    // Tags

    class IPv4;
    class IPv6;

    // Classes

    class instance;
};

#if !defined(__test__framework__instance__h)
#define __test__framework__instance__h

// Libraries

#include <string>
#include <utility>
#include <vector>

namespace test {
    class instance
    {
        // Static members

        static int _id;
        static std :: vector <std :: pair <std :: string, std :: string>> _peers;

    public:

        // Static methods

        static void load(const int &);
        static void load(const int &, const char *);

        static int id();
        template <typename> static const char * get(const int &);
    };
};

#endif
