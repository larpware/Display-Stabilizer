#include "ProfileManager.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <chrono>

namespace DisplayStabilizer {

ProfileManager::ProfileManager() {
    m_fallbackNeutral = Profile::createNeutral();
    resetToDefaults();
}

void ProfileManager::resetToDefaults() {
    m_profiles.clear();
    m_profiles.push_back(Profile::createNeutral());
    m_profiles.push_back(Profile::createNaturalWhites());
    m_profiles.push_back(Profile::createGreenCastTest());
    m_profiles.push_back(Profile::createPinkCastTest());
    m_profiles.push_back(Profile::createLowContrast());
    m_profiles.push_back(Profile::createDiagnostic());

    m_activeProfileId = m_profiles[0].id;
}

const Profile* ProfileManager::getProfileById(const std::wstring& id) const {
    for (const auto& p : m_profiles) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

Profile* ProfileManager::getProfileByIdMut(const std::wstring& id) {
    for (auto& p : m_profiles) {
        if (p.id == id) return &p;
    }
    return nullptr;
}

const Profile& ProfileManager::getActiveProfile() const {
    const Profile* p = getProfileById(m_activeProfileId);
    if (p) return *p;
    if (!m_profiles.empty()) return m_profiles[0];
    return m_fallbackNeutral;
}

bool ProfileManager::setActiveProfileId(const std::wstring& id) {
    if (getProfileById(id)) {
        m_activeProfileId = id;
        return true;
    }
    return false;
}

bool ProfileManager::updateProfile(const Profile& updated) {
    Profile* p = getProfileByIdMut(updated.id);
    if (!p) return false;

    // Preserve built-in immutable flags and ID
    bool wasBuiltIn = p->isBuiltIn;
    std::wstring origId = p->id;

    *p = updated;
    p->id = origId;
    p->isBuiltIn = wasBuiltIn;
    p->clamp();
    return true;
}

bool ProfileManager::duplicateProfile(const std::wstring& sourceId, const std::wstring& newName, std::wstring& outNewId) {
    const Profile* src = getProfileById(sourceId);
    if (!src) return false;

    Profile dup = *src;
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    dup.id = L"custom_" + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    dup.name = newName.empty() ? (src->name + L" (Copy)") : newName;
    dup.isBuiltIn = false;
    dup.clamp();

    m_profiles.push_back(dup);
    outNewId = dup.id;
    return true;
}

bool ProfileManager::deleteProfile(const std::wstring& id) {
    for (auto it = m_profiles.begin(); it != m_profiles.end(); ++it) {
        if (it->id == id) {
            if (it->isBuiltIn) {
                // Cannot delete built-in presets
                return false;
            }
            if (m_activeProfileId == id) {
                m_activeProfileId = m_profiles[0].id;
            }
            m_profiles.erase(it);
            return true;
        }
    }
    return false;
}

bool ProfileManager::exportProfileToJsonFile(const std::wstring& id, const std::wstring& path, std::wstring& outErr) const {
    const Profile* p = getProfileById(id);
    if (!p) {
        outErr = L"Profile not found: " + id;
        return false;
    }

    JsonValue root(JsonType::Object);
    root[L"formatVersion"] = JsonValue(1);
    root[L"appName"] = JsonValue(L"DisplayStabilizer");
    root[L"exportedTimestamp"] = JsonValue(static_cast<double>(GetTickCount64()));
    root[L"profile"] = p->toJson();

    std::string jsonUtf8 = root.serializeUtf8(true);
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        outErr = L"Failed to open export file for writing: " + path;
        return false;
    }
    ofs.write(jsonUtf8.data(), jsonUtf8.size());
    ofs.close();
    return true;
}

bool ProfileManager::importProfileFromJsonFile(const std::wstring& path, std::wstring& outNewId, std::wstring& outErr) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        outErr = L"Failed to open import file: " + path;
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    JsonValue root;
    if (!JsonValue::parseUtf8(content, root, outErr)) {
        return false;
    }

    if (!root.isObject()) {
        outErr = L"Invalid JSON root: expected object";
        return false;
    }

    JsonValue pVal;
    if (root.hasKey(L"profile")) {
        pVal = root[L"profile"];
    } else {
        pVal = root;
    }

    if (!pVal.isObject()) {
        outErr = L"No valid profile object found in JSON";
        return false;
    }

    Profile imported = Profile::fromJson(pVal);
    // Always assign new custom ID and mark not built-in to prevent replacing built-in presets
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    imported.id = L"imported_" + std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    imported.isBuiltIn = false;
    if (imported.name.empty()) {
        imported.name = L"Imported Profile";
    }
    imported.clamp();

    m_profiles.push_back(imported);
    outNewId = imported.id;
    return true;
}

JsonValue ProfileManager::customProfilesToJson() const {
    JsonValue arr(JsonType::Array);
    for (const auto& p : m_profiles) {
        if (!p.isBuiltIn) {
            arr.arrayValue.push_back(p.toJson());
        }
    }
    return arr;
}

void ProfileManager::loadCustomProfilesFromJson(const JsonValue& jv) {
    if (!jv.isArray()) return;

    for (const auto& item : jv.arrayValue) {
        if (item.isObject()) {
            Profile p = Profile::fromJson(item);
            p.isBuiltIn = false;
            // Check if already exists
            if (!getProfileById(p.id)) {
                m_profiles.push_back(p);
            }
        }
    }
}

} // namespace DisplayStabilizer
