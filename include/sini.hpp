#ifndef SINI_HPP
#define SINI_HPP

#include <regex>
#include <map>
#include <string>
#include <sstream>

namespace sini
{
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
                rv += prop.first + "=" + prop.second + "\n";
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
            std::regex sectionRegex(R"(^(?:\[([^\]]*)\]\r?\n)?([^\[]*))");
            std::regex propertyRegex(R"([ \t]*([a-zA-Z.$:][a-zA-Z0-9_~\-.:$ ]*?)[ \t]*=[ \t]*['"]?([^\n]+)['"]?[ \t]*(?:;[^\n]*)?[ \t]*)");
            std::smatch sectionResults;
            std::smatch propertyResults;

            for (std::string::const_iterator ini_iter = ini.cbegin();
                 std::regex_search(ini_iter, ini.cend(), sectionResults, sectionRegex) && ini_iter < ini.cend();
                 ini_iter = sectionResults[0].second)
            {
                std::string sectionName = sectionResults[1];
                std::string sectionText = sectionResults[2];

                Section& section = addSection(sectionName);

                for (std::string::const_iterator section_iter = sectionText.cbegin();
                     std::regex_search(section_iter, sectionText.cend(), propertyResults, propertyRegex) && section_iter < sectionText.cend();
                     section_iter = propertyResults[0].second)
                {
                    std::string propertyName = propertyResults[1];
                    std::string propertyText = propertyResults[2];

                    section.set(propertyName, propertyText);
                }
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