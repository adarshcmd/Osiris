#pragma once

#include <CS2/Panorama/PanelHandle.h>

struct WatermarkState {
    cs2::PanelHandle containerPanelHandle;
    cs2::PanelHandle fpsTextPanelHandle;
    cs2::PanelHandle pingTextPanelHandle;
    cs2::PanelHandle timeTextPanelHandle;
};