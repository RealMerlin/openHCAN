#include <avr/io.h>
#include <canix/syslog.h>
#include <darlingtonoutput.h>
#include "devices/ports.h"

void darlingtonoutput_init(void)
{
#if defined (__AVR_ATmega328P__)
	// Darlington Ports auf Output setzen
	DDRD |= (1 << DDD7) | (1 << DDD6) | (1 << DDD5) | (1 << DDD4) | (1 << DDD3) | (1 << DDD2);

	// Darlington Ports ausschalten
	PORTD &= ~( (1 << PORTD7) | (1 << PORTD6) | (1 << PORTD5) | (1 << PORTD4) | (1 << PORTD3) | (1 << PORTD2) );
#else
	// Darlington Ports auf Output setzen
	DDRD |= 0xff;
	DDRB |= 0x0f;

	// Darlington Ports ausschalten
	PORTD = 0;
	PORTB &= ~ (0x0f);
#endif
}

uint8_t darlingtonoutput_getpin(uint8_t n)
{
#if defined (__AVR_ATmega328P__)
	if (n >= 2 && n <= 7)
	{
		return PORTD & (1<< n);
	}
#else
	if (n < 8)
	{
		return PORTD & (1<< n);
	}
	else if (n < 12)
	{
		n = n - 8;
		return PORTB & (1<< n);
	}
#endif
	else
	{
		return ports_getOutput(n);
	}
}

void darlingtonoutput_setpin(uint8_t n, uint8_t state)
{
#if defined (__AVR_ATmega328P__)
	if (n >= 2 && n <= 7)
	{
		// Output Modus setzen
		DDRD |= (1<< n);

		if (state)
			PORTD |= (1<< n);
		else
			PORTD &= ~ (1<< n);
	}
#else
	if (n < 8)
	{
		// Output Modus setzen
		DDRD |= (1<< n);

		if (state)
			PORTD |= (1<< n);
		else
			PORTD &= ~ (1<< n);
	}
	else if (n < 12)
	{
		n = n - 8;

		// Output Modus setzen
		DDRB |= (1<< n);

		if (state)
			PORTB |= (1<< n);
		else
			PORTB &= ~ (1<< n);
	}
#endif
	else
	{
		//canix_syslog_P(SYSLOG_PRIO_ERROR, PSTR("chgShOutPinStat,n=%d,stat=%d"), n, state);
		ports_setOutput(n, state);
	}
}
