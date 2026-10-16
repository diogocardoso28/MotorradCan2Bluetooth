#ifndef CAN_DRIVER_H__
#define CAN_DRIVER_H__

#define CAN_TAG "CAN"
#define LONG_PRESS_TIME_SECONDS 1

void configure_can(void (*handle_menu_buttons_callback)(int));

// Data Structures Declaration

enum ButtonActions
{
    // Menu Buttons
    UP,
    LONG_UP,
    DOWN,
    LONG_DOWN,
    // Wonder Wheel
    WHEEL_UP,
    WHEEL_DOWN,
    WHEEL_LEFT,
    LONG_WHEEL_LEFT,
    WHEEL_RIGHT,
    LONG_WHEEL_RIGHT,
    // Blinker Button
    BLINKER_CANCEL,
    LONG_BLINKER_CANCEL,
    // MISC
    NONE,
};

#endif /* CAN_DRIVER_H__ */
