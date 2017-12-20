/*
 * bewegungsmelder.h
 *
 * Created: 06.11.2014 10:04:15
 *  Author: Martin
 */ 


#ifndef BEWEGUNGSMELDER_H_
#define BEWEGUNGSMELDER_H_

#include <canix/canix.h>
#include <canix/rtc.h>
#include <eds_types.h>
#include <inttypes.h>
#include <inttypes.h>
#include <eds-structs.h>

typedef struct
{
	uint8_t type;
	eds_bewegungsmelder_block_t config;
	//uint8_t pressed; // fuer Entprellung
	uint8_t oldState;
	canix_rtc_clock_t lastdetect;
} device_data_bewegungsmelder;

void bewegungsmelder_init(device_data_bewegungsmelder *p, eds_block_p it);
inline bewegungsmelder_timer_handler(device_data_bewegungsmelder *p, uint8_t zyklus);
void bewegungsmelder_can_callback(device_data_bewegungsmelder *p, const canix_frame *frame);

#endif /* BEWEGUNGSMELDER_H_ */