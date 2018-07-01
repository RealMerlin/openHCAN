#ifndef ZUSTANDSMELDUNG_H
#define ZUSTANDSMELDUNG_H

#include <canix/eds.h>
#include <inttypes.h>
#include <eds-structs.h>

typedef struct
{
	uint8_t type;
	eds_zustandsmeldung_block_t config;
	eds_block_p it; // EDS Block Pointer, noetig fuer EDS Schreib-Ops
	uint16_t pwm_counter;
	uint16_t pwm_width;
	uint16_t pwm_end;
	uint8_t frequenz;
	uint8_t modus;
	uint8_t frequenz_counter;
} device_data_zustandsmeldung;

void zustandsmeldung_init(device_data_zustandsmeldung *p, eds_block_p it);
void zustandsmeldung_can_callback(device_data_zustandsmeldung *p, const canix_frame *frame);
inline void zustandsmeldung_timer_handler(device_data_zustandsmeldung *p, uint8_t zyklus);

#endif
