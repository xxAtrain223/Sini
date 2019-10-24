#ifndef SINI_HPP
#define SINI_HPP

#define BOOST_SPIRIT_X3_NO_FILESYSTEM
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

#include <map>
#include <string>
#include <sstream>
#include <regex>

namespace sini
{
    class ParseError : public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
    };

    class ProxyError : public std::runtime_error
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

        template <typename T>
        std::string stringify(const T& t) {
            std::ostringstream oss;
            oss << t;
            return oss.str();
        }

        std::string stringify(const std::string& t) {
            return t;
        }

        template <typename T>
        T destringify(const std::string& str) {
            std::istringstream iss {str};
            T t;
            iss >> t;
            return t;
        }

        template <>
        std::string destringify<std::string>(const std::string& str) {
            return str;
        }

        template <>
        bool destringify<bool>(const std::string& str) {
            static std::regex reFalse("^(?:0|f|n|off|no|false)$", std::regex_constants::icase);
            static std::regex reTrue("^(?:1|t|y|on|yes|true)$", std::regex_constants::icase);

            if (std::regex_match(str, reFalse))
            {
                return false;
            }
            else if (std::regex_match(str, reTrue))
            {
                return true;
            }
            else
            {
                throw ParseError("Could not parse \"" + str + "\" as a bool");
            }
        }

        template <>
        int destringify<int>(const std::string& str) {
            static std::regex reHex("^0x.+", std::regex_constants::icase);
            static std::regex reBin("^0b.+", std::regex_constants::icase);
            static std::regex reOct("^0.+", std::regex_constants::icase);

            std::istringstream iss{ str };
            int t;

            if (std::regex_match(str, reHex))
            {
                iss >> std::hex >> t;
            }
            else if (std::regex_match(str, reBin))
            {
                return std::bitset<sizeof(int) * 8>(str, 2).to_ulong();
            }
            else if (std::regex_match(str, reOct))
            {
                iss >> std::oct >> t;
            }
            else
            {
                iss >> t;
            }

            return t;
        }

        struct throw_on_construct_tag {};
    }

    class Section
    {
        using PropMap = std::map<std::string, std::string>;

        PropMap m_properties;

        /**
         * Base class for Proxy types, essentially ConstProxy.
         */
        template <typename S, typename I>
        class ProxyBase
        {
        protected:
            S* section; // The section the proxy originated from
            I iter; // Either an iterator to the property entry, or end(m_properties)
            std::string key; // Name of property

            // Constructs the Proxy by getting an iterator to the property for the key, if it exists.
            ProxyBase(S& section, std::string key) :
                section(&section),
                iter(section.m_properties.find(key)),
                key(std::move(key))
            {}

            // Constructs the proxy as usual, but throws a ProxyError if the property does not exist.
            ProxyBase(S& section, std::string key, _detail::throw_on_construct_tag) :
                ProxyBase(section, std::move(key))
            {
                throw_if_invalid();
            }

            // Used by methods that require the property to be valid.
            void throw_if_invalid() const {
                if (!valid()) {
                    throw ProxyError("Property '" + key + "' does not exist");
                }
            }

        public:
            ProxyBase() = delete;
            ProxyBase(const ProxyBase&) = default;
            ProxyBase(ProxyBase&&) = default;
            ProxyBase& operator=(const ProxyBase&) = default;
            ProxyBase& operator=(ProxyBase&&) = default;

            // Checks if the proxy points to a property.
            // Note: the property could have been created after construction by another proxy.
            bool valid() const { return iter != end(section->m_properties); }

            // Explicity converts the property value to the type T.
            template <typename T>
            T as() const {
                throw_if_invalid();
                return _detail::destringify<T>(iter->second);
            }

            // Implicit conversion.
            template <typename T>
            operator T() const {
                return as<T>();
            }
        };

        // A mutable Proxy, allows assignment.
        class Proxy : public ProxyBase<Section, PropMap::iterator>
        {
            friend class Section;
            using ProxyBase::ProxyBase;
        public:
            // Sets the value of the property.
            template <typename T>
            void operator=(const T& val) {
                if (valid()) {
                    iter->second = _detail::stringify(val);
                } else {
                    iter = section->m_properties.insert({key, _detail::stringify(val)}).first;
                }
            }
        };

        // A const Proxy, no special features, points to a const Section.
        class ConstProxy : public ProxyBase<const Section, PropMap::const_iterator>
        {
            friend class Section;
            using ProxyBase::ProxyBase;
        };

    public:
        // Returns a Proxy without checking if the property exists, similar to std::map::operator[].
        Proxy operator[](const std::string& propertyName)
        {
            return {*this, propertyName};
        }

        // Returns a Proxy to a property, throws ProxyError if the property doesn't exist.
        Proxy at(const std::string& propertyName)
        {
            return {*this, propertyName, _detail::throw_on_construct_tag{}};
        }

        // Returns a ConstProxy to a property, throws ProxyError if the property doesn't exist.
        ConstProxy at(const std::string& propertyName) const
        {
            return {*this, propertyName, _detail::throw_on_construct_tag{}};
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

    public:
        Section& operator[](const std::string& sectionName)
        {
            return m_sections[sectionName];
        }

        Section& at(const std::string& sectionName)
        {
            return m_sections.at(sectionName);
        }

        const Section& at(const std::string& sectionName) const
        {
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
            auto curSection = &(*this)[""];

            // semantic action: begins a new section
            auto startSection = [&](auto& ctx){ curSection = &(*this)[_attr(ctx)]; };

            // semantic action: sets a property on the current section
            auto setProp = [&](auto& ctx){ (*curSection)[at_c<0>(_attr(ctx))] = at_c<1>(_attr(ctx)); };

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