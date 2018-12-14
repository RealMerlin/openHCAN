/*
 *  This file is part of openHCAN.
 *
 *  openHCAN is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openHCAN is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openHCAN.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Based on Original Sources:
 *
 *  light weight WS2812 lib
 *  Controls WS2811/WS2812/WS2812B RGB-LEDs
 *  Author: Tim (cpldcpu@gmail.com)
 *  https://github.com/cpldcpu/light_ws2812
 *
 */

#ifndef WS2812B_H
#define WS2812B_H

#include <eds-structs.h>
#include <canix/eds.h>
#include <inttypes.h>
#include <eds-structs.h>

typedef struct
{
	uint8_t type;
	eds_ws2812b_block_t config;
	uint8_t status;
	uint8_t mute;            // Aktiv: ws2812b kann Kommandos entgegennehmen; oder mute=1

} device_data_ws2812b;

#define MAX_WS2812B_GROUPS 1

#define maxLEDs		30 //150
#define ws2812_port D      // Data port

/*
 * Internal defines
 */

#define CONCAT(a, b)            a ## b
#define CONCAT_EXP(a, b)   CONCAT(a, b)

#define ws2812_PORTREG  CONCAT_EXP(PORT,ws2812_port)

/*
 *  Structure of the LED array
 */

struct cRGB { uint8_t g; uint8_t r; uint8_t b; };

void ws2812_setleds    (struct cRGB *ledarray, uint16_t number_of_leds, uint8_t pin);
void ws2812_setleds_pin(struct cRGB *ledarray, uint16_t number_of_leds,uint8_t pinmask);

/*
 * Old interface / Internal functions
 *
 * The functions take a byte-array and send to the data output as WS2812 bitstream.
 * The length is the number of bytes to send - three per LED.
 */

void ws2812_sendarray_mask(uint8_t *array,uint16_t length, uint8_t pinmask);
void setAll(device_data_ws2812b *p, uint8_t intensityR, uint8_t intensityG, uint8_t intensityB, uint8_t useLEDs, uint8_t unusedLEDs);

void ws2812b_init(device_data_ws2812b *p, eds_block_p it);
extern void ws2812b_timer_handler(device_data_ws2812b *p, uint8_t zyklus);
void ws2812b_toggle(device_data_ws2812b *p);
void ws2812b_can_callback(device_data_ws2812b *p, const canix_frame *frame);

#endif
