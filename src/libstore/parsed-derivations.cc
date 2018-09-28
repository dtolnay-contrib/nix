#include "parsed-derivations.hh"

namespace nix {

ParsedDerivation::ParsedDerivation(const Path & drvPath, BasicDerivation & drv)
    : drvPath(drvPath), drv(drv)
{
    /* Parse the __json attribute, if any. */
    auto jsonAttr = drv.env.find("__json");
    if (jsonAttr != drv.env.end()) {
        try {
            structuredAttrs = nlohmann::json::parse(jsonAttr->second);
        } catch (std::exception & e) {
            throw Error("cannot process __json attribute of '%s': %s", drvPath, e.what());
        }
    }
}

std::experimental::optional<std::string> ParsedDerivation::getStringAttr(const std::string & name) const
{
    if (structuredAttrs) {
        auto i = structuredAttrs->find(name);
        if (i == structuredAttrs->end())
            return {};
        else {
            if (!i->is_string())
                throw Error("attribute '%s' of derivation '%s' must be a string", name, drvPath);
            return i->get<std::string>();
        }
    } else {
        auto i = drv.env.find(name);
        if (i == drv.env.end())
            return {};
        else
            return i->second;
    }
}

bool ParsedDerivation::getBoolAttr(const std::string & name, bool def) const
{
    if (structuredAttrs) {
        auto i = structuredAttrs->find(name);
        if (i == structuredAttrs->end())
            return def;
        else {
            if (!i->is_boolean())
                throw Error("attribute '%s' of derivation '%s' must be a Boolean", name, drvPath);
            return i->get<bool>();
        }
    } else {
        auto i = drv.env.find(name);
        if (i == drv.env.end())
            return def;
        else
            return i->second == "1";
    }
}

std::experimental::optional<Strings> ParsedDerivation::getStringsAttr(const std::string & name) const
{
    if (structuredAttrs) {
        auto i = structuredAttrs->find(name);
        if (i == structuredAttrs->end())
            return {};
        else {
            if (!i->is_array())
                throw Error("attribute '%s' of derivation '%s' must be a list of strings", name, drvPath);
            Strings res;
            for (auto j = i->begin(); j != i->end(); ++j) {
                if (!j->is_string())
                    throw Error("attribute '%s' of derivation '%s' must be a list of strings", name, drvPath);
                res.push_back(j->get<std::string>());
            }
            return res;
        }
    } else {
        auto i = drv.env.find(name);
        if (i == drv.env.end())
            return {};
        else
            return tokenizeString<Strings>(i->second);
    }
}

bool ParsedDerivation::canBuildLocally() const
{
    return drv.platform == settings.thisSystem
        || settings.extraPlatforms.get().count(drv.platform) > 0
        || drv.isBuiltin();
}

bool ParsedDerivation::willBuildLocally() const
{
    return getBoolAttr("preferLocalBuild") && canBuildLocally();
}

}
