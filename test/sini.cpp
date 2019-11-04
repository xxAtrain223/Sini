#include "sini.hpp"

#include <gtest/gtest.h>

namespace sini
{
    namespace test
    {
        TEST(sini, Normalization)
        {
            Sini sini;
            sini.parse(
                "c=\t42\n"
                " a = b \n"
                "\n"
                "[section1]\n"
                "  e='  asdf  '\n"
                "g  =\"as123df\"\n"
                "\n"
            );
            EXPECT_EQ(sini.toString(),
                "a=b\n"
                "c=42\n"
                "\n"
                "[section1]\n"
                "e=\"  asdf  \"\n"
                "g=as123df\n"
                "\n"
            );
        }

        TEST(sini, RoundTrip)
        {
            Sini sini;
            sini[""]["a"] = 42;
            sini[""]["b"] = "asdf";
            sini["A"]["c"] = 4.5;

            auto str = sini.toString();

            Sini sini2;
            sini2.parse(str);

            EXPECT_EQ(sini2[""]["a"].as<int>(), 42);
            EXPECT_EQ(sini2[""]["b"].as<std::string>(), "asdf");
            EXPECT_EQ(sini2["A"]["c"].as<double>(), 4.5);
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

        TEST(sini, SingleQuotes)
        {
            Sini sini;
            sini.parse(R"(
                foo = '  horse  '
            )");

            EXPECT_EQ(sini[""]["foo"].as<std::string>(), "  horse  ");
        }

        TEST(sini, DoubleQuotes)
        {
            Sini sini;
            sini.parse(R"(
                foo = "  horse  "
            )");

            EXPECT_EQ(sini[""]["foo"].as<std::string>(), "  horse  ");
        }

        TEST(sini, OutputQuotes)
        {
            Sini sini;

            sini[""]["foo"] = "  horse  ";

            EXPECT_EQ(sini.toString(), "foo=\"  horse  \"\n\n");
        }

        TEST(sini, AtSectionError)
        {
            Sini sini;

            EXPECT_THROW(sini.at(""), std::out_of_range);
        }

        TEST(sini_proxy, ProxyAssignment)
        {
            Sini sini;
            sini[""]["test"] = 42;

            EXPECT_EQ(sini[""]["test"].as<std::string>(), "42");
        }

        TEST(sini_proxy, ProxyReassignment)
        {
            Sini sini;

            sini[""]["test"] = 42;
            EXPECT_EQ(sini[""]["test"].as<std::string>(), "42");

            sini[""]["test"] = 99;
            EXPECT_EQ(sini[""]["test"].as<std::string>(), "99");
        }

        TEST(sini_proxy, ProxyConversions)
        {
            Sini sini;
            sini[""]["test"] = 42;

            EXPECT_EQ(sini[""]["test"].as<std::string>(), "42");
            EXPECT_EQ(sini[""]["test"].as<int>(), 42);
            EXPECT_EQ(sini[""]["test"].as<double>(), 42.0);
        }

        TEST(sini_proxy, ConversionProxyError)
        {
            Sini sini;

            auto proxy = sini[""]["test"];

            EXPECT_THROW(((int)proxy), sini::ProxyError);
        }

        TEST(sini_proxy, ExplicitConversionProxyError)
        {
            Sini sini;

            auto proxy = sini[""]["test"];

            EXPECT_THROW(proxy.as<int>(), sini::ProxyError);
        }

        TEST(sini_proxy, AtProxy)
        {
            Sini sini;

            auto& section = sini[""];
            section["test"] = 42;

            EXPECT_EQ(section.at("test").as<std::string>(), "42");
        }

        TEST(sini_proxy, AtProxyError)
        {
            Sini sini;

            auto& section = sini[""];

            EXPECT_THROW(section.at("test"), sini::ProxyError);
        }

        TEST(sini_proxy, ConstAtProxy)
        {
            Sini sini;

            auto& section = sini[""];
            section["test"] = 42;

            const auto& const_section = section;

            EXPECT_EQ(const_section.at("test").as<std::string>(), "42");
        }

        TEST(sini_proxy, ConstAtProxyError)
        {
            Sini sini;

            const auto& section = sini[""];

            EXPECT_THROW(section.at("test"), sini::ProxyError);
        }

        TEST(sini_parse, Int)
        {
            Sini sini;

            EXPECT_NO_THROW(sini.parse(
                "int=493\n"
                "hex=0x01ED\n"
                "oct=0755\n"
                "bin=0b0000000111101101\n"
                "neg=-493\n"
            ));

            EXPECT_EQ(sini[""]["int"].as<int>(), 493);
            EXPECT_EQ(sini[""]["hex"].as<int>(), 0x01ED);
            EXPECT_EQ(sini[""]["oct"].as<int>(), 0755);
            EXPECT_EQ(sini[""]["bin"].as<int>(), 0b0000000111101101);
            EXPECT_EQ(sini[""]["neg"].as<int>(), -493);
        }

        TEST(sini_parse, FloatingPoint)
        {
            Sini sini;

            EXPECT_NO_THROW(sini.parse(
                "float=1.f\n"
                "double=2.0\n"
                "Scientific Notation 1 =+26.84365E+13\n"
                "Scientific Notation 2=   324.90154e8\n"
                "Scientific Notation 3 = -91.66217E-9 \n"
            ));

            EXPECT_FLOAT_EQ(sini[""]["float"].as<float>(), 1.f);
            EXPECT_DOUBLE_EQ(sini[""]["double"].as<double>(), 2.0);
            EXPECT_DOUBLE_EQ(sini[""]["Scientific Notation 1"].as<double>(), +26.84365E+13);
            EXPECT_DOUBLE_EQ(sini[""]["Scientific Notation 2"].as<double>(), 324.90154e8);
            EXPECT_DOUBLE_EQ(sini[""]["Scientific Notation 3"].as<double>(), -91.66217E-9);
        }

        TEST(sini_parse, Boolean)
        {
            Sini sini;

            EXPECT_NO_THROW(sini.parse(
                "zero=0\n"
                "f=f\n"
                "n=n\n"
                "off=off\n"
                "no=no\n"
                "false=false\n"
                "one=1\n"
                "t=t\n"
                "y=y\n"
                "on=on\n"
                "yes=yes\n"
                "true=true\n"
            ));

            EXPECT_EQ(sini[""]["zero"].as<bool>(), false);
            EXPECT_EQ(sini[""]["f"].as<bool>(), false);
            EXPECT_EQ(sini[""]["n"].as<bool>(), false);
            EXPECT_EQ(sini[""]["off"].as<bool>(), false);
            EXPECT_EQ(sini[""]["no"].as<bool>(), false);
            EXPECT_EQ(sini[""]["false"].as<bool>(), false);
            EXPECT_EQ(sini[""]["one"].as<bool>(), true);
            EXPECT_EQ(sini[""]["t"].as<bool>(), true);
            EXPECT_EQ(sini[""]["y"].as<bool>(), true);
            EXPECT_EQ(sini[""]["on"].as<bool>(), true);
            EXPECT_EQ(sini[""]["yes"].as<bool>(), true);
            EXPECT_EQ(sini[""]["true"].as<bool>(), true);
        }
    }
}