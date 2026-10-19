#ifndef CORE_LIBS_PRX_LIBSCEVIDEOOUT_PADINPUT_HPP
#define CORE_LIBS_PRX_LIBSCEVIDEOOUT_PADINPUT_HPP

#include "SDL_events.h"
#include "SDL_gamecontroller.h"
#include "prx/libScePad/include/KeyboardMouse.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include <array>
#include <chrono>

class DisplayWindow;

class PadInput {
public:
    PadInput();
    ~PadInput();
    void HandleEvent(const SDL_Event& event, DisplayWindow& window);
    void Update();
    bool MouseModeEnabled() const { return keyboardMouse.MouseMode(); }

private:
    struct ControllerSlot {
        SDL_GameController* controller = nullptr;
        SDL_JoystickID instanceId = -1;
        PadInputState state{};
        std::uint32_t outputSequence = 0;
        PadOutputState outputState{};
        bool outputPending = false;
        std::chrono::steady_clock::time_point nextRumbleRefresh{};
    };

    explicit PadInput(Pad::InputConfiguration configuration);
    void publish();
    void publish(int slot);
    void setMouseMode(bool enabled);
    void openAvailableControllers();
    void openController(int deviceIndex);
    void closeController(int slot);
    PadInputState sampleController(int slot) const;
    void applyOutput();
    void enableSensors(int slot);

    Pad::KeyboardMouse keyboardMouse;
    std::array<ControllerSlot, PAD_MAX_CONTROLLERS> slots{};
};

#endif
