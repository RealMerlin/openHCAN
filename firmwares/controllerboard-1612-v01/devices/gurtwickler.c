/*
 * gurtwickler.c
 *
 * Created: 04.01.2015 22:53:10
 *  Author: Martin
 */

#include "gurtwickler.h"
#include "darlingtonoutput.h"
#include "tasterinput.h"

#include <hcan.h>

#include <canix/canix.h>
#include <canix/tools.h>
#include <canix/rtc.h>
#include <canix/syslog.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

#include <canix/led.h>

//#include <log.h>

/***************************************************************************


*/

void gurtwickler_init(device_data_gurtwickler *p, eds_block_p it)
{
	p->cmdsrc            = 0;
	p->lowlevel_command  = GURTWICKLER_READY;
	p->lowlevel_state    = GURTWICKLER_STATE_STOP;
	// Zur Sicherheit initial warten:
	p->lowlevel_wait_counter = 6;
	p->hilevel_up_event_counter = -1;
	canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_Init"), p->config.gruppe0);
}

uint8_t gurtwickler_is_in_group(const device_data_gurtwickler *p, uint8_t group)
{
	uint8_t i;
	uint8_t *gruppen;
	
	gruppen = (uint8_t *) &(p->config.gruppe0);

	for (i = 0; i < MAX_GURTWICKLER_GROUPS; i++)
	{
		if (gruppen[i] == group)
		return 1;
	}

	return 0;
}

void gurtwickler_timer_handler(device_data_gurtwickler *p, uint8_t zyklus)
{
	if (zyklus != 10) return; // 10tel-Sekunden-Zyklus verwendet
	
	// Läuft der Gurtwickler?
	uint8_t status_up = tasterport_read(p->config.pin_motor_up); //High-Active
	uint8_t status_down = tasterport_read(p->config.pin_motor_down); //High-Active
	if (status_up == 1 && status_down == 0)
	{
		p->lowlevel_state = GURTWICKLER_STATE_AUF;
		canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_State_Auf"), p->config.gruppe0);
	}
	else if(status_up == 0 && status_down == 1)
	{
		p->lowlevel_state = GURTWICKLER_STATE_AB;
		canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_State_Ab"), p->config.gruppe0);
	}
	else if(status_up == 0 && status_down == 0)
	{
		p->lowlevel_state = GURTWICKLER_STATE_STOP;
		canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_State_Stop"), p->config.gruppe0);
		//p->lowlevel_wait_counter = 5;
	}
	else
	{
		// error?
	}	
		
	// Die Lowlevel Steuerung:
	gurtwickler_lowlevel_timer_handler(p);
}

void gurtwickler_send_changed_info(device_data_gurtwickler *p)
{
	canix_frame message;

	message.src = canix_selfaddr();
	message.dst = HCAN_MULTICAST_INFO;
	message.proto = HCAN_PROTO_SFP;
	message.data[0] = HCAN_SRV_HES;
	message.data[1] = HCAN_HES_ROLLADEN_POSITION_CHANGED_INFO;
	message.data[2] = p->config.gruppe0;
	//message.data[3] = gurtwickler_lowlevel_get_position(p);
	message.data[4] = p->cmdsrc >> 8;
	message.data[5] = p->cmdsrc & 0xff;
	message.size = 6;

	canix_frame_send_with_prio(&message, HCAN_PRIO_HI);
}

void gurtwickler_can_callback(device_data_gurtwickler *p, const canix_frame *frame)
{
	//canix_frame answer;

	//answer.src = canix_selfaddr();
	//answer.dst = frame->src;
	//answer.proto = HCAN_PROTO_SFP;
	//answer.data[0] = HCAN_SRV_HES;

	switch (frame->data[1])
	{
		case HCAN_HES_ROLLADEN_POSITION_SET :
		if (gurtwickler_is_in_group(p,frame->data[2]))
		{
			canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_POSITION_SET"), p->config.gruppe0);
			// der Position Set Befehl wird nur ausgefuehrt, wenn
			// der Rolladen nicht laeuft!
			if (p->lowlevel_state == GURTWICKLER_STATE_STOP)
			{
				canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_STATE_IS_STOP"), p->config.gruppe0);
				p->cmdsrc = frame->src; // Absender des Befehls vormerken

				// Nun den Rolladen in die richtige Richtung starten:
				if (frame->data[3] == 100)
				{
					p->lowlevel_command = GURTWICKLER_COMMAND_AUF_TasteDown;
				}
				else if (frame->data[3] == 0)
				{
					p->lowlevel_command = GURTWICKLER_COMMAND_AB_TasteDown;
				}
				else
				{
					// freie Position lässt sich noch nicht anfahren :(
				}
			}
			else
			{
				canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_STATE_IS:%d"), p->config.gruppe0, p->lowlevel_state);
			}
		}
		break;
	}
}

/*
* Lowlevel Steuerung - Details zur Implementierung:
*
* Fuer die oben beschriebenen Wartezeiten wird der dir_blocking_counter
* und der power_blocking_counter verwendet. Beide werden alle 100msec
* im timer handler aktualisiert: Wenn sie auf 0 stehen, sind sie abgelaufen
* und die Aktion ist dann nicht mehr gesperrt.
*/

uint8_t gurtwickler_lowlevel_get_state(const device_data_gurtwickler *p)
{
	return p->lowlevel_state;
}

/*
* Die Statemachine, die die Steuerung implementiert, ist in
* ./rolladen-statemachine.png dokumentiert.
*/

void gurtwickler_lowlevel_timer_handler(device_data_gurtwickler *p)
{
	// Wait Counter decrementieren:
	if (p->lowlevel_wait_counter)
	p->lowlevel_wait_counter--;

	if (p->lowlevel_command == GURTWICKLER_COMMAND_AUF_TasteUp && p->lowlevel_wait_counter == 0)
	{
		// Taste loslassen
		canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_Taste_up_loslassen"), p->config.gruppe0);
		p->lowlevel_command = GURTWICKLER_READY;
		darlingtonoutput_setpin(p->config.port_up, 0);	// auf GND
		p->lowlevel_wait_counter = 3;
	}
	if (p->lowlevel_command == GURTWICKLER_COMMAND_AB_TasteUp && p->lowlevel_wait_counter == 0)
	{
		// Taste loslassen
		canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_Taste_Down_loslassen"), p->config.gruppe0);
		p->lowlevel_command = GURTWICKLER_READY;
		darlingtonoutput_setpin(p->config.port_down, 0);	// auf GND
		p->lowlevel_wait_counter = 3;
	}
	
	if (p->lowlevel_state == GURTWICKLER_STATE_STOP && p->lowlevel_wait_counter == 0)
	{
		switch (p->lowlevel_command)
		{
			case GURTWICKLER_COMMAND_AUF_TasteDown :
			// Taste drücken
			canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_Taste_Down_druecken"), p->config.gruppe0);
			p->lowlevel_command = GURTWICKLER_COMMAND_AUF_TasteUp;
			darlingtonoutput_setpin(p->config.port_up, 1);	// auf VCC
			p->lowlevel_wait_counter = 3;
			break;
		
			case GURTWICKLER_COMMAND_AB_TasteDown :
			// Taste drücken
			canix_syslog_P(SYSLOG_PRIO_DEBUG, PSTR("Rolladen:%d_Taste_Up_druecken"), p->config.gruppe0);
			p->lowlevel_command = GURTWICKLER_COMMAND_AB_TasteUp;
			darlingtonoutput_setpin(p->config.port_down, 1);	// auf VCC
			p->lowlevel_wait_counter = 3;
			break;
		}
	}
}
