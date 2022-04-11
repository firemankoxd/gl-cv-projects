/*
 * PJR_projekt.c
 *
 * Created: 3. 1. 2021 19:11:27
 * Author : kalus
 */ 

// Implement�cia kni�n�c
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "rtc.h"
#include "ff.h"
#include "diskio.h"

// Defin�cia makier
#define FLOAT_TO_INT(x) ((int)(x))
#define INT_TO_FLOAT(x) ((float)(x))	

// Deklar�cia premenn�ch
bool zobrazenie = 0;
int Vo, counter = 0;
float logR2, T, R1, R2, teplota = 25, c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
volatile uint16_t ADval = 0;
uint8_t jednotky, desiatky, desatiny, stotiny, zobrazene_cislo = 0, pocitadlo = 0;
uint8_t cislice[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
struct tm* cas;

// Deklar�cia premenn�ch pre z�pis na SD kartu
FATFS fs;
FIL fd;
UINT bytesWritten;
FRESULT mountResult, openStatus;
volatile UINT Timer;

// Vytvorenie novej �trukt�ry (objektu) spotrebi�
struct spotrebic
{
	uint8_t id;		// ID spotrebi�a
	uint8_t pin;	// Pin spotrebi�a
	float teplotaS;	// Teplota pre sp�nanie/rozp�nanie
	bool reverse;	// false-spotrebi� sa pri danej teplote zopne / true-spotrebi� sa pri danej teplote rozopne
	bool stav;		// aktu�lny stav spotrebi�a (false-rozopnut� / true-zopnut�)
	bool vystup;	// povolenie zmeny vystupu (false-zak�zan� / true-povolen�)
};

// Spotrebi� 1 - zop�nanie pri 30.0�C - v�stup povolen�
struct spotrebic SP1 = {
	1,
	(1 << PORTC3),
	26.0,
	false,
	false,
	false
};

// Spotrebi� 2 - rozop�nanie pri 30.5�C - v�stup zak�zan�
struct spotrebic SP2 = {
	2,
	(1 << PORTC4),
	30.0,
	false,
	false,
	false
};

// Spotrebi� 3 - rozop�nanie pri 27.0�C - v�stup zak�zan�
struct spotrebic SP3 = {
	3,
	(1 << PORTC5),
	32.0,
	false,
	false,
	false
};

// Spotrebi� 2 - zop�nanie pri 28.5�C - v�stup zak�zan�
struct spotrebic SP4 = {
	4,
	(1 << PORTC6),
	33.0,
	false,
	false,
	false
};

// Funkcia pre vr�tenie �asu (potrebn� pri z�pise s�borov na SD kartu) - vyu��va �daj z RTC
DWORD get_fattime(void) {
	return ((DWORD)(cas->year + 20) << 25)
	| ((DWORD)cas->mon << 21)
	| ((DWORD)cas->mday << 16)
	| ((DWORD)cas->hour << 11)
	| ((DWORD)cas->min << 5)
	| ((DWORD)cas->sec >> 1);
}

// Funkcia na resetovanie zobrazovan�ho segmentu
void segmentsOff(void)
{
	PORTB |= 0x03;	//0b00000011
	PORTC |= 0x06;	//0b00000110
}

// Funkcia na vyp�sanie zmeny stavu spotrebi�a na SD kartu do logu
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

// Funkcia pre nastavenie stavov spotrebi�ov pod�a ich priradenej teploty
void checkTemp(struct spotrebic *sp)
{
	// Rozop�nanie pri nastavenej teplote
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
	// Zop�nanie pri nastavenej teplote
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

// Preru�enie AD prevodn�ka
ISR(ADC_vect)
{
	// Z�skanie anal�govej hodnoty
	ADval = ADC;
}

// Preru�enie �asova�a 0 (Prete�enie) - pre v�pis ��slic na displej
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
		// Zobrazenie �asu
		desiatky = cas->hour / 10;
		jednotky = cas->hour % 10;
		desatiny = cas->min / 10;
		stotiny = cas->min % 10;
	}
	switch(zobrazene_cislo)
	{
		case 0:
			PORTC &= ~(1 << PORTC1);	// PC1 (A1) = 0
			PORTD = ((desiatky == 0) ? 0b00000000 : cislice[desiatky]) | ((SP1.stav) ? 0x80 : 0x00); // Ak je teplota men�ia ako 10 -> prvy segment sa nezobrazuje
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

// Preru�enie �asova�a 1 (Porovnanie) - pre funkciu disk_timerproc() a sledovanie stavov spotrebi�ov
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
	
	// Nastavenie smerov�ch registrov -> vstup 0 / v�stup 1
	DDRC |= 0x0E;				// 0b00001110 (A1..A3)
	DDRB |= 0x03;				// 0b00000011 (D8..D9)
	DDRD |= 0xFF;				// 0b11111111 (D0..D7)
	
	// Nastavenie registrov pre �asova� T0
	TCCR0B |= (1 << CS02);		// Preddeli�ka /1024
	TIMSK0 |= (1 << TOIE0);		// Povolenie preru�enia pri prete�en�
	
	// Nastavenie registrov pre �asova� T1
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1B |= (1 << WGM12) |	// CTC M�d
			  (1 << CS11);		// preddeli�ka /8
	TIMSK1 |= (1 << OCIE1A);	// Output Compare A Match Interrupt Enable
	OCR1A = 0x4E20;				// 16-bitov� register (OCR1AH & OCR1AL) - pre porovn�vaciu hodnotu
	
	// Nastavenie registrov pre AD prevodn�k
	ADMUX |= (1 << REFS0);		//MUX3..0 nenastavujem -> pou��vam vstup ADC0
	ADCSRA |= (1 << ADEN) |		// Povolenie ADC
			  (1 << ADSC) |		// Za�atie konverzie
			  (1 << ADATE) |	// Auto trigger enable
			  (1 << ADIE) |		// Interruput enable
			  (1 << ADPS2) |	// Preddeli�ka (0b111->128)
			  (1 << ADPS1) |	//
			  (1 << ADPS0);		//
	DIDR0 |= (1 << ADC0D);		// Digital Input Disable pre ADC0
	
	// Glob�lne povolenie preru�en�
	sei();

	// Nastavenie �asu
	cas->sec = 0;				// Sekundy
	cas->min = 0;				// Min�ty
	cas->hour = 10;				// Hodiny
	cas->mday = 19;				// De�
	cas->mon = 1;				// Mesiac
	cas->year = 1921;			// Rok - 100
	cas->wday = 2;				// De� v t��dni
	twi_init_master();			// Inicializ�cia I2C zbernice
	rtc_init();					// Inicializ�cia RTC modulu
	rtc_set_time(cas);			// Nastavenie �asu do modulu

	// Test z�pisu do s�boru na SD karte
 	mountResult = f_mount(&fs, "0:/", 1);	
}

void loop(void)
{
	// Steinhart-Hartova rovnica pre v�po�et teploty
	Vo = ADval;
	R2 = R1 * (1023.0 / INT_TO_FLOAT(Vo) - 1.0);
	logR2 = log(R2);
	T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
	T = T - 273.15;

	// Zn�enie citlivosti pri vypisovan� teploty na displej
	teplota = (T + (1023 * teplota)) / 1024;
	
	// Cyklick� obnova �asu pre vypisovanie na displej
	cas = rtc_get_time();
	
}

// Hlavn� funkcia main volaj�ca funkcie setup a loop -> podobnos� s arduinom, preh�adnej�� program
int main(void)
{
	setup();
	while (1)
	{
		loop();
	}
}
