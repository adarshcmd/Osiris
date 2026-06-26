#pragma once

#include <Features/Hud/Watermark/WatermarkConfigVariables.h>
#include <GameClient/Panorama/PanoramaDropDown.h>
#include <Platform/Macros/FunctionAttributes.h>

template <typename HookContext>
class HudTab {
public:
    explicit HudTab(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void init(auto&& guiPanel) const
    {
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, BombTimerEnabled>>(guiPanel, "bomb_timer");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, DefusingAlertEnabled>>(guiPanel, "defusing_alert");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, KillfeedPreserverEnabled>>(guiPanel, "preserve_killfeed");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, PostRoundTimerEnabled>>(guiPanel, "postround_timer");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, BombPlantAlertEnabled>>(guiPanel, "bomb_plant_alert");
        initDropDown<OnOffDropdownSelectionChangeHandler<HookContext, HudWatermarkEnabled>>(guiPanel, "hud_watermark");
    }

    void updateFromConfig(auto&& mainMenu) const noexcept
    {
        setDropDownSelectedIndex(mainMenu, "bomb_timer", !GET_CONFIG_VAR(BombTimerEnabled));
        setDropDownSelectedIndex(mainMenu, "defusing_alert", !GET_CONFIG_VAR(DefusingAlertEnabled));
        setDropDownSelectedIndex(mainMenu, "preserve_killfeed", !GET_CONFIG_VAR(KillfeedPreserverEnabled));
        setDropDownSelectedIndex(mainMenu, "postround_timer", !GET_CONFIG_VAR(PostRoundTimerEnabled));
        setDropDownSelectedIndex(mainMenu, "bomb_plant_alert", !GET_CONFIG_VAR(BombPlantAlertEnabled));
        setDropDownSelectedIndex(mainMenu, "hud_watermark", !GET_CONFIG_VAR(HudWatermarkEnabled));
    }

private:
    [[NOINLINE]] void setDropDownSelectedIndex(auto&& mainMenu, const char* dropDownId, int selectedIndex) const noexcept
    {
        auto&& panel = mainMenu.findChildInLayoutFile(dropDownId);
        if (panel)
            panel.clientPanel().template as<PanoramaDropDown>().setSelectedIndex(selectedIndex);
    }

    template <typename Handler>
    void initDropDown(auto&& guiPanel, const char* panelId) const
    {
        auto&& panel = guiPanel.findChildInLayoutFile(panelId);
        if (!panel)
            return;

        auto&& dropDown = panel.clientPanel().template as<PanoramaDropDown>();
        dropDown.registerSelectionChangedHandler(&GuiEntryPoints<HookContext>::template dropDownSelectionChanged<Handler>);
    }

    HookContext& hookContext;
};