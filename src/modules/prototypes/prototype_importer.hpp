#pragma once

#include "../../prototypes/entity/object/object.hpp"

#include <memory>
#include <string>

struct ImportedPrototypeData
{
    std::string type;
    std::string idText;
    ObjectPrototype objectPrototype;
};

class PrototypeImporter
{
public:
    static std::unique_ptr<ImportedPrototypeData> ImportFromJson(
        const std::string& sourceText,
        const std::string& assetId);
};
