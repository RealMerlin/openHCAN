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
#include <avr/pgmspace.h>

#include "../controllerboard/darlingtonoutput.h"
#include "../controllerboard/devices.h"
#include "../controllerboard/onewire.h"
#include "../controllerboard/timer.h"
#include "../controllerboard/timeservice.h"


void controllerboard_callback(const canix_frame *frame)
{
	// Dieser Handler wird fuer alle Destination Adressen ausgefuehrt
	// daher muss gefiltert werden, was uns betrifft und was nicht:
	if ( (frame->data[0] != HCAN_SRV_HES))
		// Diese Message ist nicht interessant, daher ignorieren
		return;

	if ((frame->dst != canix_selfaddr()) && (
				(frame->dst != HCAN_MULTICAST_CONTROL) &&
				(frame->dst != HCAN_MULTICAST_INFO)))
		// Diese Message ist nicht interessant, daher ignorieren
		return;

	switch (frame->data[1])
	{
		case HCAN_HES_CONFIG_RELOAD :
			devices_load_config();
			return;
		case HCAN_HES_CONFIG_RAM_USAGE_REQUEST :
			{
				canix_frame answer;

				answer.src = canix_selfaddr();
				answer.dst = frame->src;
				answer.proto = HCAN_PROTO_SFP;
				answer.data[0] = HCAN_SRV_HES;
				answer.data[1] = HCAN_HES_CONFIG_RAM_USAGE_REPLAY;
				answer.data[2] = device_data_ram_usage >> 8;
				answer.data[3] = device_data_ram_usage;
				answer.size = 4;

				canix_frame_send(&answer);
			}
			return;
	}
}

int main(void)
{
	canix_init();

	// Haus-Elektrik Service Handler installieren
	canix_reg_frame_callback(hauselektrik_callback, -1,
			HCAN_PROTO_SFP, HCAN_SRV_HES);

	canix_reg_frame_callback(controllerboard_callback, -1,
			HCAN_PROTO_SFP, HCAN_SRV_HES);


	// setup timeservice can frame handler
	canix_reg_frame_callback(timeservice_can_callback, HCAN_MULTICAST_INFO,
			HCAN_PROTO_SFP, HCAN_SRV_RTS);

	devices_load_config();

	canix_reg_rtc_callback(timer_handler);
	canix_reg_idle_callback(idle_handler);

	canix_mainloop();
	return 0;
}

