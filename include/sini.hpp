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
        std::string& operator[](const std::string& propertyName)
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

        std::string toString() const
        {
            std::string rv;

            for (const auto& prop : m_properties)
            {
                const auto& k = prop.first;
                const auto& v = prop.second;
                auto quote =
                    !v.empty() && (std::isspace(v.front()) || std::isspace(v.back()))
                        ? "\""
                        : "";

                rv += k + "=" + quote + v + quote + "\n";
            }

            return rv;
        }
    };

    class Sini
    {
        std::map<std::string, Section> m_sections;

    public :
        Section& operator[](const std::string& sectionName)
        {
            return m_sections.at(sectionName);
        }

        Section& addSection(const std::string& sectionName)
        {
            m_sections.emplace(sectionName, Section());
            return m_sections.at(sectionName);
        }

        void parse(const std::string& ini)
        {
            namespace x3 = boost::spirit::x3;

            using x3::rule;

            // X3 built-in parsers
            using x3::char_;
            using x3::space;
            using x3::eps;
            using x3::eol;
            using x3::eoi;
            using x3::lexeme;
            using x3::omit;

            using boost::fusion::at_c;

            // pointer to current section (used in the setProp semantic action)
            auto curSection = &addSection("");

            // semantic action: begins a new section
            auto startSection = [&](auto& ctx){ curSection = &addSection(_attr(ctx)); };

            // semantic action: sets a property on the current section
            auto setProp = [&](auto& ctx){ curSection->set(at_c<0>(_attr(ctx)), at_c<1>(_attr(ctx))); };

            // whitepsace
            auto ws = char_(" \t");
            // whitespace including newline
            auto wsn = ws | eol;

            // string enclosed in single quotes, value doesn't include the quotes
            auto singleQuotedValue = '\'' >> *((char_ - '\'') | "\\'") > '\'';
            // string enclosed in double quotes, value doesn't include the quotes
            auto doubleQuotedValue = '"' >> *((char_ - '"') | "\\\"") > '"';

            // a chunk of text which doesn't contain whitespace
            auto textChunk = +(char_ - wsn);

            // a sequence of textChunks separated by whitespace
            auto rawValue = textChunk >> *(+ws >> textChunk);

            // allowed first characters of property key
            auto propLeader = char_("a-zA-Z.$:");

            // other property key characters
            auto propChars = char_("a-zA-Z0-9._~\\-$:");

            // property key name
            auto propKey
                = rule<struct propKey, std::string>{"propKey"}
                = lexeme[ propLeader >> *propChars >> *(+char_(" ") >> +propChars) ];

            // property value
            auto propValue
                = rule<struct propValue, std::string>{"propValue"}
                = lexeme[ singleQuotedValue | doubleQuotedValue | rawValue | eps ];

            // section header, semantic value doesn't include the brackets
            auto sectionHeader = '[' > +(char_ - ']') > ']';

            // property definition
            auto property = propKey > '=' > propValue;

            // comment
            auto comment = ';' > *(char_ - eol);

            // a line containing either a section header or property definition (or nothing), and an optional comment
            auto line = -(sectionHeader[startSection] | property[setProp]) >> -comment;

            // whole INI file
            auto iniFile
                = rule<_detail::error_handler>{"iniFile"}
                = line % eol;

            // current iterator into ini, should be equal to enditer when done
            auto iter = begin(ini);
            // end of ini file
            const auto enditer = end(ini);
            // output stream for errors, used to form exception messages
            auto err = std::ostringstream{};

            using x3::phrase_parse;
            using x3::with;
            using x3::error_handler_tag;
            using error_handler_type = x3::error_handler<std::string::const_iterator>;

            // default error handler
            auto error_handler = error_handler_type(iter, enditer, err);

            // top-level parser, wraps iniFile with error handling
            auto parser = with<error_handler_tag>(std::ref(error_handler))[ iniFile ];

            // parse the input
            auto r = phrase_parse(iter, enditer, parser, ws);

            // if the parsing failed, or we somehow didn't reach the end, throw an exception
            if (!r || iter != enditer)
            {
                throw ParseError(err.str());
            }
        }

        std::string toString() const
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