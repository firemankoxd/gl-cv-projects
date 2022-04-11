/*
 * PJR_projekt.c
 *
 * Created: 3. 1. 2021 19:11:27
 * Author : kalus
 */ 

// Libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "rtc.h"
#include "ff.h"
#include "diskio.h"

// Macros
#define FLOAT_TO_INT(x) ((int)(x))
#define INT_TO_FLOAT(x) ((float)(x))	

// Variables
bool zobrazenie = 0;
int Vo, counter = 0;
float logR2, T, R1, R2, teplota = 25, c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
volatile uint16_t ADval = 0;
uint8_t jednotky, desiatky, desatiny, stotiny, zobrazene_cislo = 0, pocitadlo = 0;
uint8_t cislice[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
struct tm* cas;

// SD card variables
FATFS fs;
FIL fd;
UINT bytesWritten;
FRESULT mountResult, openStatus;
volatile UINT Timer;

// Struct for appliance
struct spotrebic
{
	uint8_t id;		// Appliance ID
	uint8_t pin;	// Appliance pin
	float teplotaS;	// Limit temperature
	bool reverse;	// false-appliance will connect to power at given temperature / true-appliance will disconnect from power at given temperature
	bool stav;		// Current appliance state
	bool vystup;	// Allowing state changes on physical output (pin): true-enabled, false-disabled
};

// Appliance No. 1 - connects to power at temperature=26deg celsius
struct spotrebic SP1 = {
	1,
	(1 << PORTC3),
	26.0,
	false,
	false,
	false
};

// Appliance No. 2 - connects to power at temperature=30deg celsius
struct spotrebic SP2 = {
	2,
	(1 << PORTC4),
	30.0,
	false,
	false,
	false
};

// Appliance No. 3 - connects to power at temperature=32deg celsius
struct spotrebic SP3 = {
	3,
	(1 << PORTC5),
	32.0,
	false,
	false,
	false
};

// Appliance No. 4 - disconnects from power at temperature=33deg celsius
struct spotrebic SP4 = {
	4,
	(1 << PORTC6),
	33.0,
	true,
	false,
	false
};

// Getting current time from RTC into FatFS-friendly format
DWORD get_fattime(void) {
	return ((DWORD)(cas->year + 20) << 25)
	| ((DWORD)cas->mon << 21)
	| ((DWORD)cas->mday << 16)
	| ((DWORD)cas->hour << 11)
	| ((DWORD)cas->min << 5)
	| ((DWORD)cas->sec >> 1);
}

// Reseting segments
void segmentsOff(void)
{
	PORTB |= 0x03;	//0b00000011
	PORTC |= 0x06;	//0b00000110
}

// SD card logging
void printState(uint8_t sid, uint8_t spin, bool sstav)
{
	if (mountResult == FR_OK)
	{
		openStatus = f_open(&fd, "0:/teplota.csv", FA_OPEN_APPEND | FA_WRITE);
		if (openStatus == FR_OK)
		{
			f_printf(&fd, "%02d/%02d/%04d;%02d:%02d:%02d;", cas->mday, cas->mon, cas->year+100, cas->hour, cas->min, (cas->sec)%60);
			f_printf(&fd, "%d;0b%08b;%s;", sid, spin, (spin==(1<<PORTC3))?"A3":(spin==(1<<PORTC4))?"A4":(spin==(1<<PORTC5))?"A5":(spin==(1<<PORTC6))?"A6":"---");
			if(!sstav)
			{
				f_printf(&fd, "Zopnut�;%d �C\n", (int)teplota);
			} else {
				f_printf(&fd, "Rozopnut�;%d �C\n", (int)teplota);
			}
			f_close(&fd);
		}
	}
}

// Changing appliance states based on current temperature
void checkTemp(struct spotrebic *sp)
{
	// Disconnecting from power at given temperature
	if(sp->reverse)
	{
		if(teplota >= sp->teplotaS)
		{
			if(sp->stav)
			{
				printState(sp->id, sp->pin, sp->stav);
			}
			sp->stav = false;
			if(sp->vystup) PORTC |= sp->pin;
		}
		else if(teplota < sp->teplotaS - 0.5)
		{
			if(sp->stav == false)
			{
				printState(sp->id, sp->pin, sp->stav);
			}
			sp->stav = true;
			if(sp->vystup) PORTC &= ~(sp->pin);
		}
	}
	// Connecting to power at given temperature
	else
	{
		if(teplota >= sp->teplotaS)
		{
			if(sp->stav == false)
			{
				printState(sp->id, sp->pin, sp->stav);
			}
			sp->stav = true;
			if(sp->vystup) PORTC &= ~(sp->pin);
		}
		else if(teplota < sp->teplotaS - 0.5)
		{
			if(sp->stav)
			{
				printState(sp->id, sp->pin, sp->stav);
			}
			sp->stav = false;
			if(sp->vystup) PORTC |= sp->pin;
		}
	}
}

// Analog-digital converted interrupt
ISR(ADC_vect)
{
	// Getting analog value
	ADval = ADC;
}

// Timer0 interrupt - displaying digits on 7segment display
ISR(TIMER0_OVF_vect)
{
	// 256 * (255 - 67) / (16 * 10^6) -> 3.008 ms
	TCNT0 = 67;
	zobrazene_cislo++;
	if (zobrazene_cislo >= 4) zobrazene_cislo = 0;
	counter++;
	// 5000 * 0.003 -> approx. 15 s
	if(counter >= 5000)
	{
		// Toggling displayed value between temperature and current time
		zobrazenie = !zobrazenie;
		counter = 0;
	}
	segmentsOff();
	if(!zobrazenie)
	{
		// Displaying temperature
		desiatky = teplota / 10;
		jednotky = FLOAT_TO_INT(teplota) % 10;
		desatiny = (FLOAT_TO_INT(teplota*100) % 100) / 10;
		stotiny = FLOAT_TO_INT(teplota*100) % 10;
	} else {
		// Displaying time
		desiatky = cas->hour / 10;
		jednotky = cas->hour % 10;
		desatiny = cas->min / 10;
		stotiny = cas->min % 10;
	}
	switch(zobrazene_cislo)
	{
		case 0:
			PORTC &= ~(1 << PORTC1);	// PC1 (A1) = 0
			PORTD = ((desiatky == 0) ? 0b00000000 : cislice[desiatky]) | ((SP1.stav) ? 0x80 : 0x00); // Do not show first segment if the temp is smaller than 10
			break;
		case 1:
			PORTC &= ~(1 << PORTC2);	// PC2 (A2) = 0
			PORTD = cislice[jednotky] | ((SP2.stav) ? 0x80 : 0x00);	
			break;
		case 2:
			PORTB &= ~(1 << PORTB1);	// PB1 (D9) = 0
			PORTD = cislice[desatiny] | ((SP3.stav) ? 0x80 : 0x00);
			break;
		case 3:
			PORTB &= ~(1 << PORTB0);	// PB0 (D8) = 0
			PORTD = cislice[stotiny] | ((SP4.stav) ? 0x80 : 0x00);
			break;
		default:
			segmentsOff();
			break;
	}
}

// Timer1 interrupt - for FatFS function disk_timerproc() and monitoring states of appliances
ISR(TIMER1_COMPA_vect)
{
	Timer++;
	disk_timerproc();
	pocitadlo++;
	// 10 * 10ms -> approx. 100 ms
	if(pocitadlo >= 10)
	{
		checkTemp(&SP1);
		checkTemp(&SP2);
		checkTemp(&SP3);
		checkTemp(&SP4);
		pocitadlo = 0;
	}
}

void setup(void)
{
	// Resistor used for calculating temperature
	R1 = 10000.0;				// R = 10 kOhm
	
	// I/O registers settings
	DDRC |= 0x0E;				// 0b00001110 (A1..A3)
	DDRB |= 0x03;				// 0b00000011 (D8..D9)
	DDRD |= 0xFF;				// 0b11111111 (D0..D7)
	
	// Timer0 registers settings
	TCCR0B |= (1 << CS02);		// Prescaling /1024
	TIMSK0 |= (1 << TOIE0);		// Overflow interrupt enable
	
	// Timer1 registers settings
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << WGM12) |	// CTC Mode
			  (1 << CS11);		// Prescaler /8
	TIMSK1 |= (1 << OCIE1A);	// Output Compare A Match Interrupt Enable
	OCR1A = 0x4E20;				// 16-bit register (OCR1AH & OCR1AL) - for comparing
	
	// Analog-Digital Converter registers settings
	ADMUX |= (1 << REFS0);		// MUX3..0 setting to use input ADC0
	ADCSRA |= (1 << ADEN) |		// ADC enable
			  (1 << ADSC) |		// Conversion start
			  (1 << ADATE) |	// Auto trigger enable
			  (1 << ADIE) |		// Interruput enable
			  (1 << ADPS2) |	// Prescaler (0b111->128)
			  (1 << ADPS1) |	//
			  (1 << ADPS0);		//
	DIDR0 |= (1 << ADC0D);		// Digital Input Disable for ADC0
	
	// Interrupts global enable
	sei();

	// Setting the right time
	cas->sec = 0;				// Seconds
	cas->min = 0;				// Minutes
	cas->hour = 10;				// Hours
	cas->mday = 19;				// Day
	cas->mon = 1;				// Month
	cas->year = 1921;			// Year - 100
	cas->wday = 2;				// Day in week
	twi_init_master();			// I2C init
	rtc_init();					// RTC module init
	rtc_set_time(cas);			// Setting actual date to RTC module

	// File system init
 	mountResult = f_mount(&fs, "0:/", 1);	
}

void loop(void)
{
	// Steinhart-Hart equation to calculate the temperature
	Vo = ADval;
	R2 = R1 * (1023.0 / INT_TO_FLOAT(Vo) - 1.0);
	logR2 = log(R2);
	T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
	T = T - 273.15;

	// Smaller sensitivity to temperature changes
	teplota = (T + (1023 * teplota)) / 1024;
	
	// Loading current time from RTC module
	cas = rtc_get_time();
	
}

int main(void)
{
	setup();
	while (1)
	{
		loop();
	}
}
