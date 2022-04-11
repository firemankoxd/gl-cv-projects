/*
 * PJR_projekt.c
 *
 * Created: 3. 1. 2021 19:11:27
 * Author : kalus
 */ 

// Implementácia kniníc
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "rtc.h"
#include "ff.h"
#include "diskio.h"

// Definícia makier
#define FLOAT_TO_INT(x) ((int)(x))
#define INT_TO_FLOAT(x) ((float)(x))	

// Deklarácia premennıch
bool zobrazenie = 0;
int Vo, counter = 0;
float logR2, T, R1, R2, teplota = 25, c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
volatile uint16_t ADval = 0;
uint8_t jednotky, desiatky, desatiny, stotiny, zobrazene_cislo = 0, pocitadlo = 0;
uint8_t cislice[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
struct tm* cas;

// Deklarácia premennıch pre zápis na SD kartu
FATFS fs;
FIL fd;
UINT bytesWritten;
FRESULT mountResult, openStatus;
volatile UINT Timer;

// Vytvorenie novej štruktúry (objektu) spotrebiè
struct spotrebic
{
	uint8_t id;		// ID spotrebièa
	uint8_t pin;	// Pin spotrebièa
	float teplotaS;	// Teplota pre spínanie/rozpínanie
	bool reverse;	// false-spotrebiè sa pri danej teplote zopne / true-spotrebiè sa pri danej teplote rozopne
	bool stav;		// aktuálny stav spotrebièa (false-rozopnutı / true-zopnutı)
	bool vystup;	// povolenie zmeny vystupu (false-zakázaná / true-povolená)
};

// Spotrebiè 1 - zopínanie pri 30.0°C - vıstup povolenı
struct spotrebic SP1 = {
	1,
	(1 << PORTC3),
	26.0,
	false,
	false,
	false
};

// Spotrebiè 2 - rozopínanie pri 30.5°C - vıstup zakázanı
struct spotrebic SP2 = {
	2,
	(1 << PORTC4),
	30.0,
	false,
	false,
	false
};

// Spotrebiè 3 - rozopínanie pri 27.0°C - vıstup zakázanı
struct spotrebic SP3 = {
	3,
	(1 << PORTC5),
	32.0,
	false,
	false,
	false
};

// Spotrebiè 2 - zopínanie pri 28.5°C - vıstup zakázanı
struct spotrebic SP4 = {
	4,
	(1 << PORTC6),
	33.0,
	false,
	false,
	false
};

// Funkcia pre vrátenie èasu (potrebná pri zápise súborov na SD kartu) - vyuíva údaj z RTC
DWORD get_fattime(void) {
	return ((DWORD)(cas->year + 20) << 25)
	| ((DWORD)cas->mon << 21)
	| ((DWORD)cas->mday << 16)
	| ((DWORD)cas->hour << 11)
	| ((DWORD)cas->min << 5)
	| ((DWORD)cas->sec >> 1);
}

// Funkcia na resetovanie zobrazovaného segmentu
void segmentsOff(void)
{
	PORTB |= 0x03;	//0b00000011
	PORTC |= 0x06;	//0b00000110
}

// Funkcia na vypísanie zmeny stavu spotrebièa na SD kartu do logu
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
				f_printf(&fd, "Zopnutı;%d °C\n", (int)teplota);
			} else {
				f_printf(&fd, "Rozopnutı;%d °C\n", (int)teplota);
			}
			f_close(&fd);
		}
	}
}

// Funkcia pre nastavenie stavov spotrebièov pod¾a ich priradenej teploty
void checkTemp(struct spotrebic *sp)
{
	// Rozopínanie pri nastavenej teplote
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
	// Zopínanie pri nastavenej teplote
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

// Prerušenie AD prevodníka
ISR(ADC_vect)
{
	// Získanie analógovej hodnoty
	ADval = ADC;
}

// Prerušenie èasovaèa 0 (Preteèenie) - pre vıpis èíslic na displej
ISR(TIMER0_OVF_vect)
{
	// 256 * (255 - 67) / (16 * 10^6) -> 3.008 ms
	TCNT0 = 67;
	zobrazene_cislo++;
	if (zobrazene_cislo >= 4) zobrazene_cislo = 0;
	counter++;
	// 5000 * 0.003 -> cca 15 s
	if(counter >= 5000)
	{
		//zobrazenie = !zobrazenie;
		counter = 0;
	}
	segmentsOff();
	if(!zobrazenie)
	{
		// Zobrazenie teploty
		desiatky = teplota / 10;
		jednotky = FLOAT_TO_INT(teplota) % 10;
		desatiny = (FLOAT_TO_INT(teplota*100) % 100) / 10;
		stotiny = FLOAT_TO_INT(teplota*100) % 10;
	} else {
		// Zobrazenie èasu
		desiatky = cas->hour / 10;
		jednotky = cas->hour % 10;
		desatiny = cas->min / 10;
		stotiny = cas->min % 10;
	}
	switch(zobrazene_cislo)
	{
		case 0:
			PORTC &= ~(1 << PORTC1);	// PC1 (A1) = 0
			PORTD = ((desiatky == 0) ? 0b00000000 : cislice[desiatky]) | ((SP1.stav) ? 0x80 : 0x00); // Ak je teplota menšia ako 10 -> prvy segment sa nezobrazuje
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

// Prerušenie èasovaèa 1 (Porovnanie) - pre funkciu disk_timerproc() a sledovanie stavov spotrebièov
ISR(TIMER1_COMPA_vect)
{
	Timer++;
	disk_timerproc();
	pocitadlo++;
	// 10 * 10ms -> cca 100 ms
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
	// Nastavenie hodnoty deliaceho rezistora R1 pre meranie teploty
	R1 = 10000.0;				// R = 10 kOhm
	
	// Nastavenie smerovıch registrov -> vstup 0 / vıstup 1
	DDRC |= 0x0E;				// 0b00001110 (A1..A3)
	DDRB |= 0x03;				// 0b00000011 (D8..D9)
	DDRD |= 0xFF;				// 0b11111111 (D0..D7)
	
	// Nastavenie registrov pre èasovaè T0
	TCCR0B |= (1 << CS02);		// Preddelièka /1024
	TIMSK0 |= (1 << TOIE0);		// Povolenie prerušenia pri preteèení
	
	// Nastavenie registrov pre èasovaè T1
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << WGM12) |	// CTC Mód
			  (1 << CS11);		// preddelièka /8
	TIMSK1 |= (1 << OCIE1A);	// Output Compare A Match Interrupt Enable
	OCR1A = 0x4E20;				// 16-bitovı register (OCR1AH & OCR1AL) - pre porovnávaciu hodnotu
	
	// Nastavenie registrov pre AD prevodník
	ADMUX |= (1 << REFS0);		//MUX3..0 nenastavujem -> pouívam vstup ADC0
	ADCSRA |= (1 << ADEN) |		// Povolenie ADC
			  (1 << ADSC) |		// Zaèatie konverzie
			  (1 << ADATE) |	// Auto trigger enable
			  (1 << ADIE) |		// Interruput enable
			  (1 << ADPS2) |	// Preddelièka (0b111->128)
			  (1 << ADPS1) |	//
			  (1 << ADPS0);		//
	DIDR0 |= (1 << ADC0D);		// Digital Input Disable pre ADC0
	
	// Globálne povolenie prerušení
	sei();

	// Nastavenie èasu
	cas->sec = 0;				// Sekundy
	cas->min = 0;				// Minúty
	cas->hour = 10;				// Hodiny
	cas->mday = 19;				// Deò
	cas->mon = 1;				// Mesiac
	cas->year = 1921;			// Rok - 100
	cas->wday = 2;				// Deò v tıdni
	twi_init_master();			// Inicializácia I2C zbernice
	rtc_init();					// Inicializácia RTC modulu
	rtc_set_time(cas);			// Nastavenie èasu do modulu

	// Test zápisu do súboru na SD karte
 	mountResult = f_mount(&fs, "0:/", 1);	
}

void loop(void)
{
	// Steinhart-Hartova rovnica pre vıpoèet teploty
	Vo = ADval;
	R2 = R1 * (1023.0 / INT_TO_FLOAT(Vo) - 1.0);
	logR2 = log(R2);
	T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
	T = T - 273.15;

	// Zníenie citlivosti pri vypisovaní teploty na displej
	teplota = (T + (1023 * teplota)) / 1024;
	
	// Cyklická obnova èasu pre vypisovanie na displej
	cas = rtc_get_time();
	
}

// Hlavná funkcia main volajúca funkcie setup a loop -> podobnos s arduinom, preh¾adnejší program
int main(void)
{
	setup();
	while (1)
	{
		loop();
	}
}
