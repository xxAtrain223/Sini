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
            try
            {
                sini.parse(
                    "a=b\n"
                    "\n"
                    "[asdf\n" // intentionally forgot the closing bracket
                    "e=f\n"
                    "\n"
                );

                FAIL() << "Expected parsing to throw.";
            }
            catch (const sini::ParseError&)
            {
                // success!
            }
            catch (...)
            {
                FAIL() << "Expected ParseError exception type.";
            }
        }
    }
}