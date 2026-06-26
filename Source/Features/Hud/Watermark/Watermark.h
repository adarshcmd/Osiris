#pragma once

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <ctime>

#include <CS2/Classes/Color.h>
#include <CS2/Classes/Entities/C_BaseEntity.h>
#include <CS2/Panorama/CUILength.h>
#include <CS2/Panorama/StyleEnums.h>
#include <GameClient/Panorama/PanelAlignmentParams.h>
#include <GameClient/Panorama/PanelFontParams.h>
#include <GameClient/Panorama/PanelMarginParams.h>
#include <GameClient/Panorama/PanelShadowParams.h>
#include <GameClient/Panorama/PanoramaLabel.h>
#include <GameClient/Panorama/PanoramaUiEngine.h>
#include <GameClient/Panorama/PanelHandle.h>

#include "WatermarkConfigVariables.h"
#include "WatermarkState.h"

template <typename HookContext>
class Watermark {
public:
    explicit Watermark(HookContext& hookContext) noexcept
        : hookContext{hookContext}
    {
    }

    void run() const noexcept
    {
        auto&& panel = containerPanel();
        if (!panel)
            return;

        if (!GET_CONFIG_VAR(HudWatermarkEnabled)) {
            panel.hide();
            return;
        }

        panel.show();
        updateFps();
        updatePing();
        updateTime();
    }

    void onUnload() const noexcept
    {
        hookContext.template make<PanoramaUiEngine>().deletePanelByHandle(state().containerPanelHandle);
    }

private:
    [[nodiscard]] decltype(auto) containerPanel() const noexcept
    {
        return hookContext.template make<PanelHandle>(state().containerPanelHandle).getOrInit([this] {
            auto&& panel = hookContext.panelFactory().createPanel(hookContext.hud().scoreAndTimeAndBomb(), "OsirisWatermark").uiPanel();
            setupContainerPanel(panel);
            createTextPanel(panel, "Osiris", cs2::Color{0, 102, 255});
            createSeparatorPanel(panel);
            state().fpsTextPanelHandle = createTextPanel(panel, "0 FPS", cs2::Color{235, 235, 235}).getHandle();
            createSeparatorPanel(panel);
            state().pingTextPanelHandle = createTextPanel(panel, "0 ms", cs2::Color{235, 235, 235}).getHandle();
            createSeparatorPanel(panel);
            state().timeTextPanelHandle = createTextPanel(panel, "00:00", cs2::Color{235, 235, 235}).getHandle();
            return panel;
        });
    }

    void setupContainerPanel(auto&& panel) const noexcept
    {
        panel.setPosition(cs2::CUILength::pixels(20), cs2::CUILength::pixels(18));
        panel.setWidth(cs2::CUILength::pixels(360));
        panel.setHeight(cs2::CUILength::pixels(24));
        panel.setZIndex(1000.0f);
        panel.setFlowChildren(cs2::k_EFlowRight);
        panel.setAlign({
            .horizontalAlignment = cs2::k_EHorizontalAlignmentLeft,
            .verticalAlignment = cs2::k_EVerticalAlignmentTop
        });
    }

    [[nodiscard]] decltype(auto) createTextPanel(auto&& parentPanel, const char* text, cs2::Color color) const noexcept
    {
        auto&& label = hookContext.panelFactory().createLabelPanel(parentPanel).uiPanel();
        label.setHeight(cs2::CUILength::pixels(24));
        label.setFont({
            .fontFamily = "Stratum2, 'Arial Unicode MS'",
            .fontSize = 18,
            .fontWeight = cs2::k_EFontWeightMedium
        });
        label.setColor(color);
        label.setTextShadow({
            .horizontalOffset = cs2::CUILength::pixels(1),
            .verticalOffset = cs2::CUILength::pixels(1),
            .blurRadius = cs2::CUILength::pixels(2),
            .strength = 2.0f,
            .color = cs2::Color{0, 0, 0, 180}
        });
        label.clientPanel().template as<PanoramaLabel>().setText(text);
        return label;
    }

    void createSeparatorPanel(auto&& parentPanel) const noexcept
    {
        auto&& label = createTextPanel(parentPanel, "|", cs2::Color{0, 102, 255, 150});
        label.setMargin({
            .marginLeft = cs2::CUILength::pixels(13),
            .marginRight = cs2::CUILength::pixels(13)
        });
    }

    void updateFps() const noexcept
    {
        const auto frameTime = hookContext.globalVars().frametime().valueOr(0.0f);
        const auto fps = frameTime > 0.0f ? static_cast<int>(1.0f / frameTime + 0.5f) : 0;

        char text[16];
        std::snprintf(text, sizeof(text), "%d FPS", fps);
        setLabelText(state().fpsTextPanelHandle, text);
    }

    void updatePing() const noexcept
    {
        constexpr auto kControllerPingOffset = 0x828;
        auto&& localController = hookContext.localPlayerController();
        const auto controllerEntity = static_cast<cs2::C_BaseEntity*>(localController.baseEntity());
        if (!controllerEntity) {
            setLabelText(state().pingTextPanelHandle, "0 ms");
            return;
        }

        const auto ping = *reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::byte*>(controllerEntity) + kControllerPingOffset);
        char text[16];
        std::snprintf(text, sizeof(text), "%u ms", ping <= 999 ? ping : 0);
        setLabelText(state().pingTextPanelHandle, text);
    }

    void updateTime() const noexcept
    {
        char text[8] = "00:00";
        const auto now = std::time(nullptr);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif
        std::snprintf(text, sizeof(text), "%02d:%02d", localTime.tm_hour, localTime.tm_min);
        setLabelText(state().timeTextPanelHandle, text);
    }

    void setLabelText(cs2::PanelHandle panelHandle, const char* text) const noexcept
    {
        if (auto&& panel = hookContext.template make<PanoramaUiEngine>().getPanelFromHandle(panelHandle))
            panel.clientPanel().template as<PanoramaLabel>().setText(text);
    }

    [[nodiscard]] auto& state() const noexcept
    {
        return hookContext.featuresStates().hudFeaturesStates.watermarkState;
    }

    HookContext& hookContext;
};