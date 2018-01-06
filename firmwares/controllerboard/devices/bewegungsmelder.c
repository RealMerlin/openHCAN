/*
 * bewegungsmelder.c
 *
 * Created: 06.11.2014 10:03:58
 *  Author: Martin
 */

#include "../../controllerboard/devices/bewegungsmelder.h"
#include <hcan.h>
#include <canix/canix.h>
#include <canix/tools.h>
#include <canix/rtc.h>
#include <canix/led.h>
#include "powerport.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include "../../controllerboard/input.h"
/*
IDEE:
Bewegunsgmelder sendet "HCAN_HES_TASTER_DOWN" nach dem ersten auslösen
Danach wird sekündlich "HCAN_HES_TASTER_DOWN" gesendet bis der Melder nicht mehr auslöst
Dadurch wird sofort nach der Bewegung das Licht eingeschaltet.
Und der Timer der Lichtzone immer wieder zurückgesetzt.
-> nach der letzte Bewegung bleibt der Melder ca. 2 Sek im Aktiven Zustand + der Timer der Lichtzone ergibt die Nachleuchtzeit
*/

void bewegungsmelder_init(device_data_bewegungsmelder *p, eds_block_p it)
{
	//p->lastdetect = canix_rtc_clock;
	//p->lastdetect.day_of_month = 0;
	//p->lastdetect.month_of_year = 0;
	//p->lastdetect.year = 0;
	//p->lastdetect.hour = 0;
	//p->lastdetect.minute = 0;
	//p->lastdetect.second = 0;
}

// wird alle 10msec aufgerufen, fuer jede Bewegungsmelder-Instanz einmal
// 10msec weil mit dem Bewegungsmelder auch Licht eingeschaltet werden soll!!
inline void bewegungsmelder_timer_handler(device_data_bewegungsmelder *p, uint8_t zyklus)
{
	if (zyklus != 10) return; // 10tel-Sekunden-Zyklus verwendet
	
	canix_frame message;
	//uint8_t time;

	// Message schon mal vorbereiten:
	message.src = canix_selfaddr();
	message.dst = HCAN_MULTICAST_CONTROL;
	message.proto = HCAN_PROTO_SFP;
	message.data[0] = HCAN_SRV_HES;
	// message.data[1] wird unten ausgefuellt
	message.data[2] = p->config.gruppe;
	message.data[3] = p->config.port;
	message.size = 4;

	// Wenn der Bewegunsmelder ausgelöst hat, dann ist der Pin 0, ansonsten 1
	uint8_t status = ! inputport_read(1, p->config.port); //Low-Active
	
	if (status != p->oldState)
	{
		// bewegungsmelder hat angeschlagen oder hat Timeout
		if (status == 1 && p->oldState == 0)
		{
			// bewegungsmelder hat angeschlagen
			message.data[1] = HCAN_HES_TASTER_DOWN;
			canix_frame_send_with_prio(&message, HCAN_PRIO_HI);
			p->lastdetect = canix_rtc_clock;
		}
		else if(status == 0 && p->oldState == 1)
		{
			// bewegungsmelder hat timeout
			message.data[1] = HCAN_HES_TASTER_DOWN;
			canix_frame_send_with_prio(&message, HCAN_PRIO_HI);
		}
		p->oldState = status;
	}

	//	if (status) // gedrueckt
	//	{
	//		message.data[1] = HCAN_HES_TASTER_DOWN;
	//		canix_frame_send_with_prio(&message, HCAN_PRIO_HI);
	//	}
	//	else
	//	{
	// Wenn Schalter-Down schon gesendet wurde,
	// dann ein Schalter-Up Event senden:
	//			message.data[1] = HCAN_HES_TASTER_UP;
	//			canix_frame_send_with_prio(&message, HCAN_PRIO_HI);
	//	}
}

void bewegungsmelder_can_callback(device_data_bewegungsmelder *p, const canix_frame *frame)
{
	canix_frame answer;

	answer.src = canix_selfaddr();
	answer.dst = frame->src;
	answer.proto = HCAN_PROTO_SFP;
	answer.data[0] = HCAN_SRV_HES;

	if (p->config.gruppe == frame->data[2])
	{
		switch (frame->data[1])
		{
			case HCAN_HES_BEWEGUNGSMELDER_LASTMOVMENT_REQUEST :
			{
				// Achtung: Diese Anfrage beantworten wir nur, wenn sie
				// an unsere Unicast-Adresse ging
				answer.data[1] = HCAN_HES_BEWEGUNGSMELDER_LASTMOVMENT_REPLAY;
				answer.data[2] = p->config.gruppe;
				answer.data[3] = p->lastdetect.day_of_month;
				answer.data[4] = p->lastdetect.month_of_year;
				answer.data[5] = p->lastdetect.year;
				answer.data[6] = p->lastdetect.hour;
				answer.data[7] = p->lastdetect.minute;
				answer.size = 8;
				canix_frame_send(&answer);
			}
		}
	}
}
