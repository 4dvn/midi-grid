#pragma once

#include "system/System.hpp"
#include "hardware/grid/GridDriver.h"

#include "io/grid/GridContainer.h"
#include "io/additional_buttons/AdditionalButtons.h"
#include "io/rotary_controls/RotaryControls.h"

#include "hardware/lcd/Driver.h"
#include "LcdContainer.h"

#include "application/internal_menu/InternalMenu.hpp"
#include "application/launchpad/Launchpad.hpp"
#include "application/startup/Startup.hpp"
#include "application/grid_test/GridTest.hpp"
#include "application/snake/Snake.hpp"

#include "system/GlobalInterrupts.hpp"
#include "io/usb/UsbMidi.hpp"

#include <freertos/thread.hpp>

class Main
{
public:
    static inline Main& getInstance()
    {
        static Main instance;
        return instance;
    }

    void run();
private:
    Main();

    mcu::System system_;
    mcu::GlobalInterrupts globalInterrupts_;
    hardware::grid::GridDriver gridDriver_;
    grid::GridContainer gridContainer_;
    additional_buttons::AdditionalButtons additionalButtons_;
    rotary_controls::RotaryControls rotaryControls_;
    midi::UsbMidi usbMidi_;
    hardware::lcd::Driver lcdDriver_;
    lcd::LcdContainer lcdContainer_;
    application::ApplicationController applicationController_;
    application::Startup startup_;
    application::GridTest gridTest_;
    application::InternalMenu internalMenu_;
    application::launchpad::Launchpad launchpad_;
    application::Snake snake_;
};
