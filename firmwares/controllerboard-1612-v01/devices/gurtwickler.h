/*
 * gurtwickler.h
 *
 * Created: 04.01.2015 22:49:20
 *  Author: Martin
 */ 

#ifndef GURTWICKLER_H_
#define GURTWICKLER_H_

#include <canix/eds.h>
#include <inttypes.h>
#include <eds-structs.h>

#define MAX_GURTWICKLER_GROUPS 4

#define GURTWICKLER_READY							0
#define GURTWICKLER_COMMAND_STOP					1
#define GURTWICKLER_COMMAND_AUF_TasteDown			2
#define GURTWICKLER_COMMAND_AUF_TasteUp				3
#define GURTWICKLER_COMMAND_AB_TasteDown			4
#define GURTWICKLER_COMMAND_AB_TasteUp				5

#define GURTWICKLER_STATE_STOP						1
#define GURTWICKLER_STATE_AUF						2
#define GURTWICKLER_STATE_AUF_TasteDown				3
#define GURTWICKLER_STATE_AB						4
#define GURTWICKLER_STATE_AB_TasteDown				5


typedef struct
{
	uint8_t type;
	eds_gurtwickler_block_t config;

	/**
	 * Command Source Address: Gibt an, von welcher HCAN Adresse der letzte
	 * Befehl kam. Im Falle des Tasters ist es 0.
	 *
	 * Wird verwendet fuer das HCAN_HES_ROLLADEN_POSITION_CHANGED_INFO Frame
	 */
	uint16_t cmdsrc;

	/**
	 * ROLLADEN_STATE* Konstante
	 */
	uint8_t lowlevel_state;
	uint8_t lowlevel_command;
	uint8_t lowlevel_wait_counter;

	int8_t hilevel_up_event_counter;

} device_data_gurtwickler;

/**
 * initialisiert den Gurtwickler
 */
void gurtwickler_init(device_data_gurtwickler *p, eds_block_p it);

/**
 * Liefert 1, falls der Rolladen in der gegebenen Gruppe ist
 */
uint8_t gurtwickler_is_in_group(const device_data_gurtwickler *p, uint8_t group);

inline void gurtwickler_timer_handler(device_data_gurtwickler *p, uint8_t zyklus);

void gurtwickler_can_callback(device_data_gurtwickler *p, const canix_frame *frame);


/* Lowlevel Steuerung */

/**
 * Liefert den Stand (bzw. den Sollzustand, sofern dieser noch nicht
 * aufgrund einer Wartezeit erreicht ist
 */
uint8_t gurtwickler_lowlevel_get_state(const device_data_gurtwickler *p);

/**
 * der Timer Handler der Lowlevel Steuerung. Er wird vom gurtwickler_timer_handler
 * aufgerufen und erledigt die Lowlevel Timer Dinge.
 */
void gurtwickler_lowlevel_timer_handler(device_data_gurtwickler *p);

#endif /* GURTWICKLER_H_ */