#pragma once

#include "runtime/vector_icon.hpp"

#include <string>
#include <vector>

namespace studio_runtime
{
    class VectorIconLibrary
    {
    public:
        bool LoadFromFile(const std::string& path, std::string& outError);
        bool LoadFromJson(const std::string& json, std::string& outError);

        const VectorIcon* FindById(const std::string& id) const;
        const std::vector<VectorIcon>& GetIcons() const;

    private:
        std::vector<VectorIcon> m_icons;
    };
}

