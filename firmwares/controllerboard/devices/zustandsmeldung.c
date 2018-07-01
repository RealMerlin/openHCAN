#include "../../controllerboard/devices/zustandsmeldung.h"

#include <canix/canix.h>
#include <canix/led.h>
#include <canix/tools.h>
#include <canix/rtc.h>
#include <canix/syslog.h>
#include <hcan.h>

#include <avr/wdt.h>
#include <avr/eeprom.h>

#include "../../controllerboard/darlingtonoutput.h"

void zustandsmeldung_init(device_data_zustandsmeldung *p, eds_block_p it)
{
	p->pwm_counter = 0;
	p->pwm_width = 0;
	p->pwm_end = 2;

	p->frequenz_counter = 0;
	p->frequenz = 0;
	p->modus = 0;
}

void zustandsmeldung_set_pwm(device_data_zustandsmeldung *p, uint8_t modus)
{
	p->modus = modus;

	if (modus == 0) // aus
	{
		p->frequenz = 0;
	}
	else if (modus == 1) // dauer an
	{
		p->frequenz = 1;
	}
	else if (modus == 2) // normal an
	{
		p->frequenz = 150;
	}
	else if (modus == 3) // kurz bevor aus
	{
		p->frequenz = 10;
	}
	else if (modus == 4) // extOn
	{
		p->frequenz = 255;
	}
//	p->pwm_end = pwm_periode;
	p->pwm_width = (int32_t)p->pwm_end * 100 / 100;
}

/**uint8_t heizung_get_pwm(device_data_heizung *p)
{
	uint8_t rate = 0;

	if (p->mode != HEIZUNG_MODE_OFF)
	{
		rate = (uint32_t)p->pwm_width * 100 / (uint32_t)p->pwm_end;
	}

	return rate;
}*/

/**
 * this one cares about the PWM (pulse width modulation) for
 * achieving heating rates between off and on
 */
void zustandsmeldung_handle_pwm(device_data_zustandsmeldung *p)
{
	if(p->frequenz == 0)
	{
		darlingtonoutput_setpin(p->config.port, 0);
	}
	else if(++p->frequenz_counter >= (p->frequenz / 2))
	{
		if (++p->pwm_counter >= (p->pwm_end))
		{
			p->pwm_counter = 0;
			darlingtonoutput_setpin(p->config.port, 1);
		}

		if (p->pwm_counter >= p->pwm_width)
		{
			darlingtonoutput_setpin(p->config.port, 0);
		}
		if(p->frequenz_counter >= p->frequenz)
		{
			p->frequenz_counter = 0;
		}
	}
	else
	{
		darlingtonoutput_setpin(p->config.port, 0);
	}
}

/**
 * this timer handler is called every second
 */
inline void zustandsmeldung_timer_handler(device_data_zustandsmeldung *p, uint8_t zyklus)
{
	if (zyklus != 100) return; // 100tel-Sekunden-Zyklus verwendet

	zustandsmeldung_handle_pwm(p);
}

void zustandsmeldung_can_callback(device_data_zustandsmeldung *p, const canix_frame *frame)
{
	switch (frame->data[1])
	{
		case HCAN_HES_ZUSTANDSMELDUNG :
			if (frame->data[2] == p->config.gruppe)
			{
//										   modus 1, 2, 3
				zustandsmeldung_set_pwm(p, frame->data[3]);
			}
			break;
		case HCAN_HES_POWER_GROUP_STATE_INFO :
			if (frame->data[2] == p->config.gruppe && frame->data[3] == 1)
			{
				zustandsmeldung_set_pwm(p, 2);
			}
			else if (frame->data[2] == p->config.gruppe && frame->data[3] == 0)
			{
				zustandsmeldung_set_pwm(p, 1);
			}
			break;
	}
}
