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

        TEST(sini, AtSectionError)
        {
            Sini sini;

            EXPECT_THROW(sini.at(""), std::out_of_range);
        }

        TEST(sini, ProxyAssignment)
        {
            Sini sini;
            sini[""]["test"] = 42;

            EXPECT_EQ(sini[""]["test"].as<std::string>(), "42");
        }

        TEST(sini, ProxyReassignment)
        {
            Sini sini;

            sini[""]["test"] = 42;
            EXPECT_EQ(sini[""]["test"].as<std::string>(), "42");

            sini[""]["test"] = 99;
            EXPECT_EQ(sini[""]["test"].as<std::string>(), "99");
        }

        TEST(sini, ProxyConversions)
        {
            Sini sini;
            sini[""]["test"] = 42;

            EXPECT_EQ(sini[""]["test"].as<std::string>(), "42");
            EXPECT_EQ(sini[""]["test"].as<int>(), 42);
            EXPECT_EQ(sini[""]["test"].as<double>(), 42.0);
        }

        TEST(sini, ConversionProxyError)
        {
            Sini sini;

            auto proxy = sini[""]["test"];

            EXPECT_THROW(((int)proxy), sini::ProxyError);
        }

        TEST(sini, ExplicitConversionProxyError)
        {
            Sini sini;

            auto proxy = sini[""]["test"];

            EXPECT_THROW(proxy.as<int>(), sini::ProxyError);
        }

        TEST(sini, AtProxy)
        {
            Sini sini;

            auto& section = sini[""];
            section["test"] = 42;

            EXPECT_EQ(section.at("test").as<std::string>(), "42");
        }

        TEST(sini, AtProxyError)
        {
            Sini sini;

            auto& section = sini[""];

            EXPECT_THROW(section.at("test"), sini::ProxyError);
        }

        TEST(sini, ConstAtProxy)
        {
            Sini sini;

            auto& section = sini[""];
            section["test"] = 42;

            const auto& const_section = section;

            EXPECT_EQ(const_section.at("test").as<std::string>(), "42");
        }

        TEST(sini, ConstAtProxyError)
        {
            Sini sini;

            const auto& section = sini[""];

            EXPECT_THROW(section.at("test"), sini::ProxyError);
        }
    }
}