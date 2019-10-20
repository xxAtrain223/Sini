#include "sini.hpp"

#include <gtest/gtest.h>

namespace sini
{
    namespace test
    {
        TEST(sini, temp)
        {
            Sini sini;
            sini.parse(
                "a=b\n"
                "c=d\n"
                "\n"
                "[asdf]\n"
                "e=f\n"
                "g=h\n"
                "\n"
            );
            EXPECT_EQ(sini.toString(),
                "a=b\n"
                "c=d\n"
                "\n"
                "[asdf]\n"
                "e=f\n"
                "g=h\n"
                "\n"
            );
        }

        TEST(sini, ParseError)
        {
            Sini sini;
            EXPECT_THROW((
                sini.parse(
                    "a=b\n"
                    "\n"
                    "[asdf\n" // intentionally forgot the closing bracket
                    "e=f\n"
                    "\n"
                )),
                sini::ParseError);
        }
    }
}