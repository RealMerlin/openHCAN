/*
 * gartenbewaesserung.h
 *
 * Created: 29.08.2014 11:58:29
 *  Author: Martin
 */ 

#ifndef GARTENBEWAESSERUNG_H_
#define GARTENBEWAESSERUNG_H_

#include <canix/eds.h>
#include <inttypes.h>
#include <eds-structs.h>

#define MAX_BEWAESSERUNGS_GROUPS 4

#define BEWAESSERUNG_STATE_OFF 0
#define BEWAESSERUNG_STATE_ON 1

typedef struct
{
	uint8_t type;
	eds_gartenbewaesserung_block_t config;
	
	/**
	 * besagt, ob der Bewaesserungsstrang an oder aus ist
	 */
	uint8_t state; // 0 = off, 1 = on
	
	/**
	 * Zeit-Zaehler fuer Bewaesserungssteuerung
	 */
	uint16_t timer_counter;
	
} device_data_gartenbewaesserung;

void gartenbewaesserung_init(device_data_gartenbewaesserung *p, eds_block_p it);

/**
 * Liefert != 0, falls die Bewaesserung in der genannten Gruppe ist
 */
uint8_t bewaesserung_is_in_group(const device_data_gartenbewaesserung *p, uint8_t group);

void gartenbewaesserung_can_callback(device_data_gartenbewaesserung *p, const canix_frame *frame);

inline void gartenbewaesserung_timer_handler(device_data_gartenbewaesserung *p, uint8_t zyklus);

#endif /* GARTENBEWAESSERUNG_H_ */