#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Profile.h"

namespace DisplayStabilizer {

class ProfileManager {
public:
    ProfileManager();

    void resetToDefaults();

    const std::vector<Profile>& getAllProfiles() const { return m_profiles; }
    const Profile* getProfileById(const std::wstring& id) const;
    Profile* getProfileByIdMut(const std::wstring& id);

    const Profile& getActiveProfile() const;
    bool setActiveProfileId(const std::wstring& id);

    bool updateProfile(const Profile& updated);
    bool duplicateProfile(const std::wstring& sourceId, const std::wstring& newName, std::wstring& outNewId);
    bool deleteProfile(const std::wstring& id);

    bool exportProfileToJsonFile(const std::wstring& id, const std::wstring& path, std::wstring& outErr) const;
    bool importProfileFromJsonFile(const std::wstring& path, std::wstring& outNewId, std::wstring& outErr);

    JsonValue customProfilesToJson() const;
    void loadCustomProfilesFromJson(const JsonValue& jv);

private:
    std::vector<Profile> m_profiles;
    std::wstring m_activeProfileId;
    Profile m_fallbackNeutral;
};

} // namespace DisplayStabilizer
