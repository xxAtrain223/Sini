#ifndef SINI_HPP
#define SINI_HPP

#define BOOST_SPIRIT_X3_NO_FILESYSTEM
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

#include <map>
#include <string>
#include <sstream>

namespace sini
{
    class ParseError : public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };

    namespace _detail
    {
        namespace x3 = boost::spirit::x3;

        struct error_handler
        {
            template <typename I, typename E, typename C>
            x3::error_handler_result on_error(I& first, const I& last, const E& x, const C& context)
            {
                auto& error_handler = x3::get<x3::error_handler_tag>(context).get();
                std::string message = "Error! Expecting: " + x.which() + " here:";
                error_handler(x.where(), message);
                return x3::error_handler_result::fail;
            }
        };
    }

    class Section
    {
        std::map<std::string, std::string> m_properties;

    public:
        inline std::string& operator[](const std::string& propertyName)
        {
            return m_properties.at(propertyName);
        }

        template <typename T>
        T get(const std::string& propertyName) const
        {
            T value;
            std::istringstream iss;
            iss.str(m_properties.at(propertyName));
            iss >> value;
            return value;
        }

        template <typename T>
        void set(const std::string& propertyName, const T& value)
        {
            std::ostringstream oss;
            oss << value;
            m_properties.emplace(propertyName, oss.str());
        }

        inline std::string toString() const
        {
            std::string rv;

            for (const auto& prop : m_properties)
            {
                rv += prop.first + "=" + prop.second + "\n";
            }

            return rv;
        }
    };

    class Sini
    {
        std::map<std::string, Section> m_sections;

    public :
        inline Section& operator[](const std::string& sectionName)
        {
            return m_sections.at(sectionName);
        }

        inline Section& addSection(const std::string& sectionName)
        {
            m_sections.emplace(sectionName, Section());
            return m_sections.at(sectionName);
        }

        inline void parse(const std::string& ini)
        {
            namespace x3 = boost::spirit::x3;

            using x3::rule;
            using x3::char_;
            using x3::space;
            using x3::eps;
            using x3::eol;
            using x3::eoi;
            using x3::lexeme;
            using x3::omit;

            using boost::fusion::at_c;

            auto curSection = &addSection("");

            auto concat = [](auto& ab){ return at_c<0>(ab) + at_c<1>(ab); };

            auto startSection = [&](auto& ctx){ curSection = &addSection(_attr(ctx)); };
            auto setProp = [&](auto& ctx){ curSection->set(at_c<0>(_attr(ctx)), at_c<1>(_attr(ctx))); };

            auto ws = char_(" \t");
            auto wsn = ws | eol;

            auto singleQuotedValue = '\'' >> *char_ > '\'';
            auto doubleQuotedValue = '"' >> *char_ > '"';

            auto textChunk = +(char_ - wsn);
            auto rawValue = textChunk >> *(+ws >> textChunk);

            auto propKey
                = rule<struct propKey, std::string>{"propKey"}
                = lexeme[ char_("a-zA-Z.$:") > *char_("a-zA-Z0-9._~\\-$: ") ];

            auto propValue
                = rule<struct propValue, std::string>{"propValue"}
                = lexeme[ singleQuotedValue | doubleQuotedValue | rawValue | eps ];

            auto sectionHeader = '[' > +(char_ - ']') > ']';
            auto property = propKey > '=' > propValue;
            auto comment = ';' > *(char_ - eol);

            auto line = -(sectionHeader[startSection] | property[setProp]) >> -comment;

            auto iniFile
                = rule<_detail::error_handler>{"iniFile"}
                = line % eol;

            auto iter = begin(ini);
            auto enditer = end(ini);
            auto err = std::ostringstream{};

            using x3::phrase_parse;
            using x3::with;
            using x3::error_handler_tag;
            using error_handler_type = x3::error_handler<std::string::const_iterator>;

            auto error_handler = error_handler_type(iter, enditer, err);

            auto parser = with<error_handler_tag>(std::ref(error_handler))[ iniFile ];

            auto r = phrase_parse(iter, enditer, parser, ws);

            if (!r || iter != enditer)
            {
                throw ParseError(err.str());
            }
        }

        inline std::string toString() const
        {
            std::string rv;

            for (const auto& section : m_sections)
            {
                if (section.first != "")
                {
                    rv += "[" + section.first + "]\n";
                }
                rv += section.second.toString() + "\n";
            }

            return rv;
        }
    };
}

#endif // SINI_HPP