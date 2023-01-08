#include <stdafx.h>

#include "Settings.h"

#include <CET.h>

#include <Utils.h>

Settings::Settings(Options& aOptions, LuaVM& aVm)
    : Widget("Settings")
    , m_options(aOptions)
    , m_vm(aVm)
{
    Load();
}

WidgetResult Settings::OnPopup()
{
    const auto ret = UnsavedChangesPopup(
        "Settings", m_openChangesModal, m_madeChanges, [this] { Save(); }, [this] { Load(); });
    m_madeChanges = ret == TChangedCBResult::CHANGED;
    m_popupResult = ret;

    return m_madeChanges ? WidgetResult::ENABLED : WidgetResult::DISABLED;
}

WidgetResult Settings::OnDisable()
{
    if (m_enabled)
    {
        if (m_popupResult == TChangedCBResult::CANCEL)
        {
            m_popupResult = TChangedCBResult::APPLY;
            return WidgetResult::CANCEL;
        }

        if (m_madeChanges)
        {
            m_drawPopup = true;
            return WidgetResult::ENABLED;
        }

        m_enabled = false;
    }

    return m_enabled ? WidgetResult::ENABLED : WidgetResult::DISABLED;
}

void Settings::OnUpdate()
{
    const auto frameSize = ImVec2(ImGui::GetContentRegionAvail().x, -(ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().FramePadding.y + 2.0f));
    if (ImGui::BeginChild(ImGui::GetID("设置"), frameSize))
    {
        m_madeChanges = false;
        if (ImGui::CollapsingHeader("补丁程序", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TreePush();
            if (ImGui::BeginTable("##SETTINGS_PATCHES", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchSame, ImVec2(-ImGui::GetStyle().IndentSpacing, 0)))
            {
                const auto& patchesSettings = m_options.Patches;
                UpdateAndDrawSetting(
                    "AMD SMT补丁",
                    "适用于在蠢驴更新后未获得性能提升的AMD处理器 "
                    "（需要重启游戏才能生效）",
                    m_patches.AmdSmt, patchesSettings.AmdSmt);
                UpdateAndDrawSetting(
                    "移除行人", "移除大部分行人和交通（需要重启游戏才能生效）", m_patches.RemovePedestrians,
                    patchesSettings.RemovePedestrians);
                UpdateAndDrawSetting(
                    "禁用异步计算",
                    "禁用异步计算，可以提升旧显卡的性能，例如英伟达GTX 10系显卡 "
                    "（需要重启游戏才能生效）",
                    m_patches.AsyncCompute, patchesSettings.AsyncCompute);
                UpdateAndDrawSetting(
                    "禁用抗锯齿", "完全禁用抗锯齿（需要重启游戏才能生效）", m_patches.Antialiasing, patchesSettings.Antialiasing);
                UpdateAndDrawSetting(
                    "跳过开始界面",
                    "跳过开始要你按空格键继续的界面 "
                    "（需要重启游戏才能生效）",
                    m_patches.SkipStartMenu, patchesSettings.SkipStartMenu);
                UpdateAndDrawSetting(
                    "禁用开场动画", "禁用开场动画视频（需要重启游戏才能生效）", m_patches.DisableIntroMovies,
                    patchesSettings.DisableIntroMovies);
                UpdateAndDrawSetting(
                    "禁用晕影", "禁用屏幕四边的渐变黑色晕影（需要重启游戏才能生效）", m_patches.DisableVignette, patchesSettings.DisableVignette);
                UpdateAndDrawSetting(
                    "禁用边界传送", "允许玩家进入边界外面的地方（需要重启游戏才能生效）", m_patches.DisableBoundaryTeleport,
                    patchesSettings.DisableBoundaryTeleport);
                UpdateAndDrawSetting("禁用V-Sync (仅限Win7系统)", "在Win7上禁用V-sync以绕过垂直同步60帧的限制（需要重启游戏才能生效）", m_patches.DisableWin7Vsync, patchesSettings.DisableWin7Vsync);
                UpdateAndDrawSetting(
                    "修复小地图闪烁", "（需要重新启动游戏才能生效）", m_patches.MinimapFlicker,
                    patchesSettings.MinimapFlicker);

                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
        if (ImGui::CollapsingHeader("控制台其他设置", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::TreePush();
            if (ImGui::BeginTable("##SETTINGS_DEV", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchSame, ImVec2(-ImGui::GetStyle().IndentSpacing, 0)))
            {
                const auto& developerSettings = m_options.Developer;
                UpdateAndDrawSetting(
                    "去除无效按键绑定",
                    "去除所有不再有效的按键绑定 "
                    "(在调试模组问题时禁用此项可能会很有用)",
                    m_developer.RemoveDeadBindings, developerSettings.RemoveDeadBindings);
                UpdateAndDrawSetting(
                    "启用ImGui界面诊断窗口",
                    "启用所有ImGui诊断，诊断将被记录到触发诊断人的日志文件中 "
                    "（在调试ImGui界面问题时很有用， "
                    "也应该用于发布模组之前检查模组问题！）",
                    m_developer.EnableImGuiAssertions, developerSettings.EnableImGuiAssertions);
                UpdateAndDrawSetting(
                    "启用Debug版本", "设置内部标签以伪装成Debug版本（需要重新启动游戏才能生效）", m_developer.EnableDebug,
                    developerSettings.EnableDebug);
                UpdateAndDrawSetting(
                    "转储游戏配置", "将所有游戏配置转储到主日志文件中（需要重新启动游戏才能生效）", m_developer.DumpGameOptions,
                    developerSettings.DumpGameOptions);

                ImGui::EndTable();
            }
            ImGui::TreePop();
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    const auto itemWidth = GetAlignedItemWidth(3);
    if (ImGui::Button("加载", ImVec2(itemWidth, 0)))
        Load();
    ImGui::SameLine();
    if (ImGui::Button("保存", ImVec2(itemWidth, 0)))
        Save();
    ImGui::SameLine();
    if (ImGui::Button("默认", ImVec2(itemWidth, 0)))
        ResetToDefaults();
}

void Settings::Load()
{
    m_options.Load();

    m_patches = m_options.Patches;
    m_developer = m_options.Developer;
}

void Settings::Save() const
{
    m_options.Patches = m_patches;
    m_options.Developer = m_developer;

    m_options.Save();
}

void Settings::ResetToDefaults()
{
    m_options.ResetToDefaults();

    m_patches = m_options.Patches;
    m_developer = m_options.Developer;
}

void Settings::UpdateAndDrawSetting(const std::string& acLabel, const std::string& acTooltip, bool& aCurrent, const bool& acSaved)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImVec4 curTextColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    if (aCurrent != acSaved)
        curTextColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

    ImGui::AlignTextToFramePadding();

    ImGui::PushStyleColor(ImGuiCol_Text, curTextColor);

    ImGui::PushID(&acLabel);
    ImGui::TextUnformatted(acLabel.c_str());
    ImGui::PopID();

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !acTooltip.empty())
        ImGui::SetTooltip("%s", acTooltip.c_str());

    ImGui::TableNextColumn();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight()) / 2);
    ImGui::Checkbox(("##" + acLabel).c_str(), &aCurrent);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s", acTooltip.c_str());

    ImGui::PopStyleColor();

    m_madeChanges |= aCurrent != acSaved;
}
