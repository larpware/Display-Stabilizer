#pragma once

#include <string>
#include <vector>

namespace DisplayStabilizer {

struct VgaCheckItem {
    int id;
    std::wstring title;
    std::wstring category;
    std::wstring description;
    std::wstring physicalAction;
    std::wstring diagnosticInsight;
    bool isCompleted = false;
};

class VgaWorkflow {
public:
    VgaWorkflow();

    const std::vector<VgaCheckItem>& getItems() const { return m_items; }
    std::vector<VgaCheckItem>& getItemsMut() { return m_items; }
    const VgaCheckItem* getItem(int id) const;
    void setCompleted(int id, bool completed);

    int getCompletedCount() const;
    int getTotalCount() const { return static_cast<int>(m_items.size()); }

    static std::wstring getClockPhaseExplanation();
    static std::wstring getVgaColorPinoutExplanation();

private:
    std::vector<VgaCheckItem> m_items;
};

} // namespace DisplayStabilizer
