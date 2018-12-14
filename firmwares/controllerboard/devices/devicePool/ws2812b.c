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

#include "ws2812b.h"

#include <canix/syslog.h>
#include <hcan.h>

#include <canix/canix.h>
#include <canix/led.h>
#include <canix/tools.h>
#include <canix/rtc.h>
#include <canix/syslog.h>
#include <hcan.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>

struct cRGB led[maxLEDs];

void inline ws2812_setleds(struct cRGB *ledarray, uint16_t leds, uint8_t pin)
{
	ws2812_setleds_pin(ledarray,leds, _BV(pin));
}

void inline ws2812_setleds_pin(struct cRGB *ledarray, uint16_t leds, uint8_t pinmask)
{
	DDRD |= pinmask; // Enable DDR
	ws2812_sendarray_mask((uint8_t*)ledarray,leds+leds+leds,pinmask);
	_delay_us(50);
}

/*
  This routine writes an array of bytes with RGB values to the Dataout pin
  using the fast 800kHz clockless WS2811/2812 protocol.
*/

// Timing in ns
#define w_zeropulse   350
#define w_onepulse    900
#define w_totalperiod 1250

// Fixed cycles used by the inner loop
#define w_fixedlow    2
#define w_fixedhigh   4
#define w_fixedtotal  8

// Insert NOPs to match the timing, if possible
#define w_zerocycles    (((F_CPU/1000)*w_zeropulse          )/1000000)
#define w_onecycles     (((F_CPU/1000)*w_onepulse    +500000)/1000000)
#define w_totalcycles   (((F_CPU/1000)*w_totalperiod +500000)/1000000)

// w1 - nops between rising edge and falling edge - low
#define w1 (w_zerocycles-w_fixedlow)
// w2   nops between fe low and fe high
#define w2 (w_onecycles-w_fixedhigh-w1)
// w3   nops to complete loop
#define w3 (w_totalcycles-w_fixedtotal-w1-w2)

#if w1>0
  #define w1_nops w1
#else
  #define w1_nops  0
#endif

// The only critical timing parameter is the minimum pulse length of the "0"
// Warn or throw error if this timing can not be met with current F_CPU settings.
#define w_lowtime ((w1_nops+w_fixedlow)*1000000)/(F_CPU/1000)
#if w_lowtime>550
   #error "Light_ws2812: Sorry, the clock speed is too low. Did you set F_CPU correctly?"
//#elif w_lowtime>450
//   #warning "Light_ws2812: The timing is critical and may only work on WS2812B, not on WS2812(S)."
//   #warning "Please consider a higher clockspeed, if possible"
#endif

#if w2>0
#define w2_nops w2
#else
#define w2_nops  0
#endif

#if w3>0
#define w3_nops w3
#else
#define w3_nops  0
#endif

#define w_nop1  "nop      \n\t"
#define w_nop2  "rjmp .+0 \n\t"
#define w_nop4  w_nop2 w_nop2
#define w_nop8  w_nop4 w_nop4
#define w_nop16 w_nop8 w_nop8

void inline ws2812_sendarray_mask(uint8_t *data,uint16_t datlen,uint8_t maskhi)
{
  uint8_t curbyte,ctr,masklo;
  uint8_t sreg_prev;

  masklo	=~maskhi&PORTD;
  maskhi |=        PORTD;
  sreg_prev=SREG;
  cli();

  while (datlen--) {
    curbyte=*data++;

    asm volatile(
    "       ldi   %0,8  \n\t"
    "loop%=:            \n\t"
    "       out   %2,%3 \n\t"    //  '1' [01] '0' [01] - re
#if (w1_nops&1)
w_nop1
#endif
#if (w1_nops&2)
w_nop2
#endif
#if (w1_nops&4)
w_nop4
#endif
#if (w1_nops&8)
w_nop8
#endif
#if (w1_nops&16)
w_nop16
#endif
    "       sbrs  %1,7  \n\t"    //  '1' [03] '0' [02]
    "       out   %2,%4 \n\t"    //  '1' [--] '0' [03] - fe-low
    "       lsl   %1    \n\t"    //  '1' [04] '0' [04]
#if (w2_nops&1)
  w_nop1
#endif
#if (w2_nops&2)
  w_nop2
#endif
#if (w2_nops&4)
  w_nop4
#endif
#if (w2_nops&8)
  w_nop8
#endif
#if (w2_nops&16)
  w_nop16
#endif
    "       out   %2,%4 \n\t"    //  '1' [+1] '0' [+1] - fe-high
#if (w3_nops&1)
w_nop1
#endif
#if (w3_nops&2)
w_nop2
#endif
#if (w3_nops&4)
w_nop4
#endif
#if (w3_nops&8)
w_nop8
#endif
#if (w3_nops&16)
w_nop16
#endif

    "       dec   %0    \n\t"    //  '1' [+2] '0' [+2]
    "       brne  loop%=\n\t"    //  '1' [+3] '0' [+4]
    :	"=&d" (ctr)
    :	"r" (curbyte), "I" (_SFR_IO_ADDR(PORTD)), "r" (maskhi), "r" (masklo)
    );
  }

  SREG=sreg_prev;
}

void ws2812b_init(device_data_ws2812b *p, eds_block_p it)
{
	setAll(p, 0, 0, 0, 1, 0);
	p->mute = 0;
}

inline void ws2812b_timer_handler(device_data_ws2812b *p, uint8_t zyklus)
{
	if (zyklus != 1) return; // 1s-Zyklus verwendet
}

void setAll(device_data_ws2812b *p, uint8_t intensityR, uint8_t intensityG, uint8_t intensityB, uint8_t useLEDs, uint8_t unusedLEDs)
{
	if(useLEDs == 0) return;
	uint8_t i = 0;
	while(i < p->config.anzLEDs)
	{
		if(i % useLEDs != 0)
		{
			if(unusedLEDs == 0)
			{
				led[i].r=0;
				led[i].g=0;
				led[i].b=0;
			}
		}
		else
		{
			led[i].r=intensityR;
			led[i].g=intensityG;
			led[i].b=intensityB;
		}
		i++;
	}

	// Status setzen, damit man per TASTER_DOWN aus bzw. einschalten kann.
	if(intensityR == 0 && intensityG == 0 && intensityB == 0)
	{
		p->status = 0;
	}
	else
	{
		p->status = 1;
	}
	ws2812_setleds(led, p->config.anzLEDs, p->config.port);
}

void ws2812b_toggle(device_data_ws2812b *p)
{
        if (p->status) // ws218b ist aktiv
        {
			setAll(p, 0, 0, 0, 1, 0);
        }
        else
        {
			setAll(p, 255, 255, 255, 1, 0);
        }
}

static uint8_t is_in_group(const device_data_ws2812b *p, uint8_t group)
{
        uint8_t i;
        uint8_t *gruppen;
        uint8_t maxDeviceGruppen = MAX_WS2812B_GROUPS;

        // die 255 ist der Ersatzwert und wird ignoriert!
        if (group == 255)
                return 0;

        gruppen = (uint8_t *) &(p->config.gruppe);

        for (i = 0; i < maxDeviceGruppen; i++)
        {
                if (gruppen[i] == group)
                        return 1;
        }

        return 0;
}

void ws2812b_can_callback(device_data_ws2812b *p, const canix_frame *frame)
{
	canix_frame answer;

	answer.src = canix_selfaddr();
	answer.dst = frame->src;
	answer.proto = HCAN_PROTO_SFP;
	answer.data[0] = HCAN_SRV_HES;

	if (is_in_group(p, frame->data[2]))
	{
		switch (frame->data[1])
		{
			case HCAN_HES_WS2812B_SEND :
				if (frame->data[3] == 1) //alle LEDs auf eine Farbe setzen. Nicht gewählte LEDs aus
				{
					if (!p->mute) setAll(p, frame->data[4], frame->data[5], frame->data[6], frame->data[7], 0);
				}
				else if (frame->data[3] == 2) //alle LEDs auf eine Farbe setzen. Nicht gewählte LEDs bleiben wie sie sind
				{
					if (!p->mute) setAll(p, frame->data[4], frame->data[5], frame->data[6], frame->data[7], 1);
				}
				break;
			case HCAN_HES_TASTER_DOWN :
                                if (!p->mute) ws2812b_toggle(p);
                                break;
                        case HCAN_HES_POWER_GROUP_ON :
                        case HCAN_HES_SCHALTER_ON :
                                if (!p->mute) setAll(p, 255, 255, 255, 1, 0); //es soll eingeschaltet werden
                                break;
                        case HCAN_HES_POWER_GROUP_OFF :
                        case HCAN_HES_SCHALTER_OFF :
                                if (!p->mute) setAll(p, 0, 0, 0, 1, 0); //es soll abgeschaltet werden
                                break;
                        case HCAN_HES_POWER_GROUP_STATE_QUERY :
                                {
                                        answer.data[1] =
                                                HCAN_HES_POWER_GROUP_STATE_REPLAY;
                                        answer.data[2] = frame->data[2];
                                        answer.data[3] = p->status;
                                        answer.data[4] = 0;
                                        answer.size = 5;
                                        canix_frame_send_with_prio(&answer,HCAN_PRIO_HI);
                                }
                                break;
		}
	}
	else if (HCAN_HES_DEVICE_STATES_REQUEST == frame->data[1])
	{
		if(p->config.gruppe != 255)
		{
			wdt_reset();
			canix_sleep_100th(10); // 100msec Pause

			answer.data[1] = HCAN_HES_POWER_GROUP_STATE_REPLAY;
			answer.data[2] = p->config.gruppe; // wird in main.c fuer jedes Device einmal aufgerufen
			answer.data[3] = p->status;
			answer.data[4] = 0;
			answer.size = 5;
			canix_frame_send_with_prio(&answer,HCAN_PRIO_HI);
		}
	}

	if (p->config.mute == frame->data[2])
	{
		if (HCAN_HES_MUTE_OFF == frame->data[1])
			p->mute = 0; // ws2812b aktiv
		else if (HCAN_HES_MUTE_ON == frame->data[1])
			p->mute = 1; // ws2812b passiv (per Taster nicht aenderbar)
	}
}