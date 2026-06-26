#pragma once

#include <cstddef>
#include <cstdint>

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
#include <Utils/Lvalue.h>
#include <Utils/StringBuilder.h>

#include "WatermarkConfigVariables.h"
#include "WatermarkState.h"

struct WatermarkSystemTime {
    unsigned short year;
    unsigned short month;
    unsigned short dayOfWeek;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short milliseconds;
};

extern "C" __declspec(dllimport) void __stdcall GetLocalTime(WatermarkSystemTime* systemTime);

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
            state().fpsTextPanelHandle = createTextPanelAndGetHandle(panel, "0 FPS", cs2::Color{235, 235, 235});
            createSeparatorPanel(panel);
            state().pingTextPanelHandle = createTextPanelAndGetHandle(panel, "0 ms", cs2::Color{235, 235, 235});
            createSeparatorPanel(panel);
            state().timeTextPanelHandle = createTextPanelAndGetHandle(panel, "00:00", cs2::Color{235, 235, 235});
            return utils::lvalue<decltype(panel)>(panel);
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

    void createTextPanel(auto&& parentPanel, const char* text, cs2::Color color) const noexcept
    {
        auto&& label = hookContext.panelFactory().createLabelPanel(parentPanel).uiPanel();
        setupLabelPanel(label, color);
        label.clientPanel().template as<PanoramaLabel>().setText(text);
    }

    [[nodiscard]] cs2::PanelHandle createTextPanelAndGetHandle(auto&& parentPanel, const char* text, cs2::Color color) const noexcept
    {
        auto&& label = hookContext.panelFactory().createLabelPanel(parentPanel).uiPanel();
        setupLabelPanel(label, color);
        label.clientPanel().template as<PanoramaLabel>().setText(text);
        return label.getHandle();
    }

    void setupLabelPanel(auto&& label, cs2::Color color) const noexcept
    {
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
    }

    void createSeparatorPanel(auto&& parentPanel) const noexcept
    {
        auto&& label = hookContext.panelFactory().createLabelPanel(parentPanel).uiPanel();
        setupLabelPanel(label, cs2::Color{0, 102, 255, 150});
        label.setMargin({
            .marginLeft = cs2::CUILength::pixels(13),
            .marginRight = cs2::CUILength::pixels(13)
        });
        label.clientPanel().template as<PanoramaLabel>().setText("|");
    }

    void updateFps() const noexcept
    {
        const auto frameTime = hookContext.globalVars().frametime().valueOr(0.0f);
        const auto fps = frameTime > 0.0f ? static_cast<int>(1.0f / frameTime + 0.5f) : 0;

        StringBuilderStorage<16> storage;
        auto builder = storage.builder();
        builder.put(fps, " FPS");
        setLabelText(state().fpsTextPanelHandle, builder.cstring());
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
        StringBuilderStorage<16> storage;
        auto builder = storage.builder();
        builder.put(ping <= 999 ? ping : 0, " ms");
        setLabelText(state().pingTextPanelHandle, builder.cstring());
    }

    void updateTime() const noexcept
    {
        WatermarkSystemTime localTime{};
        GetLocalTime(&localTime);

        StringBuilderStorage<8> storage;
        auto builder = storage.builder();
        putTwoDigits(builder, localTime.hour);
        builder.put(':');
        putTwoDigits(builder, localTime.minute);
        setLabelText(state().timeTextPanelHandle, builder.cstring());
    }

    static void putTwoDigits(StringBuilder& builder, unsigned int value) noexcept
    {
        builder.put(static_cast<char>('0' + (value / 10) % 10));
        builder.put(static_cast<char>('0' + value % 10));
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