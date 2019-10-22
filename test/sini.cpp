#include "sini.hpp"

#include <gtest/gtest.h>

namespace sini
{
    namespace test
    {
        TEST(sini, Idempotence)
        {
            Sini sini;
            sini.parse(
                "a=b\n"
                "c=42\n"
                "\n"
                "[section1]\n"
                "e='asdf'\n"
                "g=\"as123df\"\n"
                "\n"
            );
            EXPECT_EQ(sini.toString(),
                "a=b\n"
                "c=42\n"
                "\n"
                "[section1]\n"
                "e='asdf'\n"
                "g=\"as123df\"\n"
                "\n"
            );
        }

        TEST(sini, RoundTrip)
        {
            Sini sini;
            sini.addSection("").set("a", 42);
            sini[""].set("b", "asdf");
            sini.addSection("A").set("c", 4.5);

            auto str = sini.toString();

            Sini sini2;
            sini2.parse(str);

            EXPECT_EQ(sini2[""].get<int>("a"), 42);
            EXPECT_EQ(sini2[""].get<std::string>("b"), "asdf");
            EXPECT_EQ(sini2["A"].get<double>("c"), 4.5);
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