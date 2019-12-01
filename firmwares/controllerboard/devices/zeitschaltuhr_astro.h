#ifndef ZEITSCHALTUHR_ASTRO_H
#define ZEITSCHALTUHR_ASTRO_H

#include <canix/eds.h>
#include <inttypes.h>
#include <eds-structs.h>

#define ZEITSCHALTUHR_UPDATE_INTERVAL 173


#define ZEITSCHALTUHR_ASTRO_FEATURE_EINSCHALT_ZEITZONE   3
#define ZEITSCHALTUHR_ASTRO_FEATURE_AUSSCHALT_ZEITZONE   4

typedef struct
{
	uint8_t type;
	eds_zeitschaltuhr_astro_block_t config;

	/** hier wird der Zustand der Power-Gruppe gespeichert */
	uint8_t state;

	/** 
	 * In regelmaessigen Abstaenden wird der Zustand wiederholt an die
	 * Power Gruppe gesendet, so dass verlorene Frames oder rebootete 
	 * Geraete keine Probleme machen.
	 */
	uint16_t update_counter;
	uint8_t automatikEin; // Automatik aktiviert oder nicht
} device_data_zeitschaltuhr_astro;

void zeitschaltuhr_astro_init(device_data_zeitschaltuhr_astro *p, eds_block_p it);
extern void zeitschaltuhr_astro_timer_handler(device_data_zeitschaltuhr_astro *p, uint8_t zyklus);
void zeitschaltuhr_astro_can_callback(device_data_zeitschaltuhr_astro *p, const canix_frame *frame);

#endif
