#include "../../controllerboard/devices/zeitschaltuhr_astro.h"

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

#include "../../controllerboard/darlingtonoutput.h"
#include "../../controllerboard/devices.h"

static inline void sendMessage(device_data_zeitschaltuhr_astro *p, uint8_t zustandToSend);

void zeitschaltuhr_astro_init(device_data_zeitschaltuhr_astro *p, eds_block_p it)
{
	p->state = 0;
	p->update_counter = 1 + (p->config.power_gruppe & 0x0f);
	
	/* Wir gehen davon aus, das die Automatik aktiviert ist. Ist ein Schalter konfiguriert,  
	 * so wird er auch seinen Zustand melden, wenn irgendein Controllerboard einen Reboot durchfuehrt. */
	p->automatikEin = true;
}

inline uint8_t sundown_sunset_matches()
{
	return time_matches(canix_rtc_clock.sundown_hour, canix_rtc_clock.sundown_minute, canix_rtc_clock.sunset_hour, canix_rtc_clock.sunset_minute, 254);
}

inline void zeitschaltuhr_astro_timer_handler(device_data_zeitschaltuhr_astro *p, uint8_t zyklus)
{
	if (zyklus != 1) return; // 1s-Zyklus verwendet

	if(!p->automatikEin) return; // kein Automatikbetrieb
	
	if (p->state == 0)
	{
		//Einschalten nach Astro
		if (sundown_sunset_matches())
		{ //ist aus
			// waehrend der Zeitzone wird nicht eingeschaltet
			if (p->config.zeitzone_id != 255 && zeitzone_matches(p->config.zeitzone_id)) return;
			
			p->state = 1;
			sendMessage(p, HCAN_HES_POWER_GROUP_ON); // Power Gruppe einschalten
			return;
		}
	}
	else
	{ //ist an
		// Zwischen Sonnenunter- und aufgang oder
		// waehrend der Zeitzone wird ausgeschaltet
		if (!sundown_sunset_matches() ||
				(sundown_sunset_matches() &&
				p->config.zeitzone_id != 255 && zeitzone_matches(p->config.zeitzone_id)))
		{
			p->state = 0;
			sendMessage(p, HCAN_HES_POWER_GROUP_OFF);
			return;
		}
	}

	// die regelmaessigen Update-Infos versenden (Achtung: Mutlicast
	// ist trotzdem CONTROL !)
	
	if (p->update_counter == 0)
	{
		p->update_counter = ZEITSCHALTUHR_UPDATE_INTERVAL;

		if (p->state) 	sendMessage(p, HCAN_HES_POWER_GROUP_ON);
		else 		sendMessage(p, HCAN_HES_POWER_GROUP_OFF);
	}

	p->update_counter--;
}

void zeitschaltuhr_astro_can_callback(device_data_zeitschaltuhr_astro *p, const canix_frame *frame)
{
	switch (frame->data[1])
	{
		case HCAN_HES_SCHALTER_ON :
			if (frame->data[2] == p->config.automatikEin_schalter_gruppe)
			{
				p->automatikEin = true;
			}
			break;

		case HCAN_HES_SCHALTER_OFF :
			if (frame->data[2] == p->config.automatikEin_schalter_gruppe)
			{
				p->automatikEin = false;
				sendMessage(p, HCAN_HES_POWER_GROUP_OFF); // Automatik deaktiviert -> sofort ausschalten
			}
			break;
	}
}

static inline void sendMessage(device_data_zeitschaltuhr_astro *p, uint8_t zustandToSend)
{
	canix_frame message;
	
		message.src = canix_selfaddr();
		message.dst = HCAN_MULTICAST_CONTROL;
		message.proto = HCAN_PROTO_SFP;
		message.data[0] = HCAN_SRV_HES;
		message.data[1] = zustandToSend;
		message.data[2] = p->config.power_gruppe;
		message.size    = 3;
		canix_frame_send(&message);
}
