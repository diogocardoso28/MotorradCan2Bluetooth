#include "can_driver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_check.h"

void (*handle_menu_buttons)(int);

static void twai_receive_task(void *arg)
{
    int previous_state_menu = NONE;
    int prev_time_menu = 0;
    int previous_state_wheel = NONE;
    int prev_time_wheel = 0;
    int previous_state_blinker = NONE;
    int prev_time_blinker = 0;
    ESP_LOGI(CAN_TAG, "Ready to receive messages.");

    int previous_whell_value = 0;

    while (true)
    {
        twai_message_t rx_msg;
        twai_receive(&rx_msg, portMAX_DELAY);
        // led_can_data_rx();
        // TODO: Make this more readable
        switch (rx_msg.identifier)
        {
            // Left Hand Switches
        case 0x2a0:
        {

            switch (rx_msg.data[2]) // D2 Byte
            {
            case 0x00:
                if (previous_state_menu != NONE)
                {
                    handle_menu_buttons(NONE);
                    previous_state_menu = NONE;
                }
                break;
            case 0x20: // Menu Up Button
                if (previous_state_menu == UP || previous_state_menu == LONG_UP)
                {
                    if (esp_timer_get_time() - prev_time_menu > LONG_PRESS_TIME_SECONDS * 1000000 && previous_state_menu != LONG_UP)
                    {
                        ESP_LOGI(CAN_TAG, "LONG UP");
                        handle_menu_buttons(LONG_UP);
                        previous_state_menu = LONG_UP;
                    }
                }
                else
                {
                    previous_state_menu = UP;
                    prev_time_menu = esp_timer_get_time();
                    ESP_LOGI(CAN_TAG, "UP");
                    handle_menu_buttons(UP);
                }

                break;
            case 0x10: // Menu Down Button
                if (previous_state_menu == DOWN || previous_state_menu == LONG_DOWN)
                {
                    if (esp_timer_get_time() - prev_time_menu > LONG_PRESS_TIME_SECONDS * 1000000 && previous_state_menu != LONG_DOWN)
                    {
                        ESP_LOGI(CAN_TAG, "LONG DOWN");
                        handle_menu_buttons(LONG_DOWN);
                        previous_state_menu = LONG_DOWN;
                    }
                }
                else
                {
                    prev_time_menu = esp_timer_get_time();
                    previous_state_menu = DOWN;
                    ESP_LOGI(CAN_TAG, "DOWN");
                    handle_menu_buttons(DOWN);
                }
                break;
            default:
                break;
            }
            switch (rx_msg.data[3]) // D3 Byte WONDER WHEEL LEFT/RIGHT
            {
            case 0xfe:
                if (previous_state_wheel == WHEEL_RIGHT || previous_state_wheel == LONG_WHEEL_RIGHT)
                {
                    if (esp_timer_get_time() - prev_time_wheel > LONG_PRESS_TIME_SECONDS * 1000000 && previous_state_wheel != LONG_WHEEL_RIGHT)
                    {
                        ESP_LOGI(CAN_TAG, "LONG WHEEL_RIGHT");
                        handle_menu_buttons(LONG_WHEEL_RIGHT);
                        previous_state_wheel = LONG_WHEEL_RIGHT;
                    }
                }
                else
                {
                    previous_state_wheel = WHEEL_RIGHT;
                    prev_time_wheel = esp_timer_get_time();
                    ESP_LOGI(CAN_TAG, "WHEEL_RIGHT");
                    handle_menu_buttons(WHEEL_RIGHT);
                }
                break;
            case 0xFD:
                if (previous_state_wheel == WHEEL_LEFT || previous_state_wheel == LONG_WHEEL_LEFT)
                {
                    if (esp_timer_get_time() - prev_time_wheel > LONG_PRESS_TIME_SECONDS * 1000000 && previous_state_wheel != LONG_WHEEL_LEFT)
                    {
                        ESP_LOGI(CAN_TAG, "LONG WHEEL_LEFT");
                        handle_menu_buttons(LONG_WHEEL_LEFT);
                        previous_state_wheel = LONG_WHEEL_LEFT;
                    }
                }
                else
                {
                    previous_state_wheel = WHEEL_LEFT;
                    prev_time_wheel = esp_timer_get_time();
                    ESP_LOGI(CAN_TAG, "WHEEL_LEFT");
                    handle_menu_buttons(WHEEL_LEFT);
                }
                break;
            case 0xfc:
                if (previous_state_wheel != NONE)
                {
                    handle_menu_buttons(NONE);
                    previous_state_wheel = NONE;
                }
                break;
            default:
                break;
            }
            switch (rx_msg.data[1]) // D1 Byte BLINKER
            {
            case 0x40:
                if (previous_state_blinker == BLINKER_CANCEL || previous_state_blinker == LONG_BLINKER_CANCEL)
                {
                    if (esp_timer_get_time() - prev_time_blinker > LONG_PRESS_TIME_SECONDS * 1000000 && previous_state_blinker != LONG_BLINKER_CANCEL)
                    {
                        ESP_LOGI(CAN_TAG, "LONG BLINKER_CANCEL");
                        handle_menu_buttons(LONG_BLINKER_CANCEL);
                        previous_state_blinker = LONG_BLINKER_CANCEL;
                    }
                }
                else
                {
                    previous_state_blinker = BLINKER_CANCEL;
                    prev_time_blinker = esp_timer_get_time();
                    ESP_LOGI(CAN_TAG, "BLINKER_CANCEL");
                    handle_menu_buttons(BLINKER_CANCEL);
                }
                break;

            default:
                if (previous_state_blinker != NONE)
                {
                    handle_menu_buttons(NONE);
                    previous_state_blinker = NONE;
                }
                break;
            }

            // Wheel UP/DOWN
            if (previous_whell_value > rx_msg.data[5])
            {
                handle_menu_buttons(WHEEL_UP);
                handle_menu_buttons(NONE);
            }
            if (previous_whell_value < rx_msg.data[5])
            {
                handle_menu_buttons(WHEEL_DOWN);
                handle_menu_buttons(NONE);
            }
            previous_whell_value = rx_msg.data[5];
            break;
        }

        default:
            break;
        }
    }

    vTaskDelete(NULL);
}

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
// Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config = {.mode = TWAI_MODE_LISTEN_ONLY,
                                               .tx_io = GPIO_NUM_9,
                                               .rx_io = GPIO_NUM_8,
                                               .clkout_io = TWAI_IO_UNUSED,
                                               .bus_off_io = TWAI_IO_UNUSED,
                                               .tx_queue_len = 0,
                                               .rx_queue_len = 5,
                                               .alerts_enabled = TWAI_ALERT_NONE,
                                               .clkout_divider = 0};

void configure_can(void (*handle_menu_buttons_callback)(int))
{
    // Install and start TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(CAN_TAG, "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(CAN_TAG, "Driver started");
    xTaskCreate(twai_receive_task, "TWAI_rx", 4096, NULL, 1, NULL); // Start CAN RX task
    handle_menu_buttons = handle_menu_buttons_callback;
}