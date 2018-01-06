/*
 * gartenbewaesserung.c
 *
 * Created: 29.08.2014 12:10:28
 *  Author: Martin
 */

#include "../../controllerboard/devices/gartenbewaesserung.h"
#include <hcan.h>

#include <canix/canix.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "../../controllerboard/darlingtonoutput.h"

void gartenbewaesserung_init(device_data_gartenbewaesserung *p, eds_block_p it) {}

uint8_t bewaesserung_is_in_group(const device_data_gartenbewaesserung *p, uint8_t group)
{
	uint8_t i;

	// die 255 ist der Ersatzwert und wird ignoriert!
	if (group == 255)
	return 0;
	
	for (i = 0; i < MAX_BEWAESSERUNGS_GROUPS; i++)
	{
		uint8_t *gruppen = (uint8_t *) &(p->config.gruppe0);
		if (gruppen[i] == group)
		{
			return 1;
		}
	}

	return 0;
}

void gartenbewaesserung_can_callback(device_data_gartenbewaesserung *p, const canix_frame *frame)
{
	canix_frame answer;

	answer.src = canix_selfaddr();
	answer.dst = frame->src;
	answer.proto = HCAN_PROTO_SFP;
	answer.data[0] = HCAN_SRV_HES;

	if (bewaesserung_is_in_group(p, frame->data[2]))
	{
		switch (frame->data[1])
		{
			case HCAN_HES_GARTENBEWAESSERUNG_SET_TIMER :
			{
				p->timer_counter = (frame->data[3] << 8) | frame->data[4];
			}
			break;
			
			case HCAN_HES_GARTENBEWAESSERUNG_STATE_REQUEST :
			{
				answer.data[1] = HCAN_HES_GARTENBEWAESSERUNG_STATE_REPLAY;
				answer.data[2] = frame->data[2];
				answer.data[3] = (p->timer_counter >> 8);
				answer.data[4] = p->timer_counter;
				answer.size = 5;
				canix_frame_send_with_prio(&answer, HCAN_PRIO_HI);
			}
			break;
		}

	}

}

// wird alle 1 sec aufgerufen, fuer jeden Bewaesserungsstrang einmal
inline void gartenbewaesserung_timer_handler(device_data_gartenbewaesserung *p, uint8_t zyklus)
{
	if (zyklus != 1) return; // 10tel-Sekunden-Zyklus verwendet
	
	// falls der Timer deaktiviert ist und noch nicht abgelaufen ist:
	if (p->state == BEWAESSERUNG_STATE_OFF && p->timer_counter > 0)
	{
		p->state = BEWAESSERUNG_STATE_ON;
		darlingtonoutput_setpin(p->config.port_power, 1);	// auf VCC
	}
	// wenn er abgelaufen ist:
	else if (p->state == BEWAESSERUNG_STATE_ON && p->timer_counter == 0)
	{
		// Timer ist abgelaufen; Licht ausschalten
		p->state = BEWAESSERUNG_STATE_OFF;
		darlingtonoutput_setpin(p->config.port_power, 0);	// auf GND
	}
	else if (p->state == BEWAESSERUNG_STATE_ON && p->timer_counter > 0)
	{
		// Timer dekrementieren
		p->timer_counter--;
	}
}
