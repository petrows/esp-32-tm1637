/**
 * @file app_main.c
 * @brief Example application for the TM1637 LED segment display
 */

#include <stdio.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <sys/time.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include "sdkconfig.h"
#include "tm1637.h"

#define TAG "app"

const gpio_num_t LED_CLK = CONFIG_TM1637_CLK_PIN;
const gpio_num_t LED_DTA = CONFIG_TM1637_DIO_PIN;

void lcd_tm1637_task(void * arg)
{
	tm1637_led_t * lcd = tm1637_init(LED_CLK, LED_DTA);

	setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
	tzset();

	while (true)
	{
		// Test segment control
		uint8_t seg_data[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20};
		for (uint8_t x=0; x<32; ++x)
		{
			uint8_t v_seg_data = seg_data[x%6];
			tm1637_set_segment_raw(lcd, 0, v_seg_data);
			tm1637_set_segment_raw(lcd, 1, v_seg_data);
			tm1637_set_segment_raw(lcd, 2, v_seg_data);
			tm1637_set_segment_raw(lcd, 3, v_seg_data);
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}

		// Test brightness
		for (int x=0; x<7; x++) {
			tm1637_set_brightness(lcd, x);
			tm1637_set_number(lcd, 8888);
			vTaskDelay(300 / portTICK_RATE_MS);
		}

		for (uint8_t x=0; x<3; ++x)
		{
			// Set random system time
			struct timeval tm_test = {1517769863 + (x*3456), 0};
			settimeofday(&tm_test, NULL);

			// Get current system time
			time_t now = 0;
			struct tm timeinfo = { 0 };
			time(&now);
			localtime_r(&now, &timeinfo);
			int time_number = 100 * timeinfo.tm_hour + timeinfo.tm_min;

			// Display time with blinking dots
			for (int z=0; z<5; ++z) {
				tm1637_set_number_lead_dot(lcd, time_number, true, z%2 ? 0xFF : 0x00);
				vTaskDelay(500 / portTICK_RATE_MS);
			}
		}

		// Test display numbers
		for (int x=0; x<16; ++x) {
			bool show_dot = x%2; // Show dot every 2nd cycle
			tm1637_set_segment_number(lcd, 0, x, show_dot);
			tm1637_set_segment_number(lcd, 1, x, show_dot); // On my display "dot" (clock symbol ":") connected only here
			tm1637_set_segment_number(lcd, 2, x, show_dot);
			tm1637_set_segment_number(lcd, 3, x, show_dot);
			vTaskDelay(100 / portTICK_RATE_MS);
		}
	}
}

void app_main()
{
	xTaskCreate(&lcd_tm1637_task, "lcd_tm1637_task", 4096, NULL, 5, NULL);
}

