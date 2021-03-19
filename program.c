//definicje wyprowadzeń i portów AT89S51 (biblioteka programu Keil uVision)
#include <regx51.h> 

//diody LED
sbit LEDG = P0^5;
sbit LEDR = P0^1;

//przycisk START
sbit button = P0^3;

//wyświetlacz LCD
sfr display_port = 0xA0;
sbit rs = P3^1;
sbit e = P3^0;

//moduł HC-SR04
sbit echo = P3^5;
sbit trig = P3^4;

//prototypy funkcji opóźnienia i mierzenia czasu
void delay(unsigned int);
void timer_start();
void timer_ISR();
void timer_stop();

//prototypy funkcji obsługi LCD
void LCD_cmd(unsigned char);
void LCD_init();
void LCD_char(unsigned char);
void LCD_string(unsigned char*);
void LCD_number(float);

void main(void)
{
	float res = 0;  //zmierzona i obliczona odległość
	button = 1;     //ustawienie wyprowadzenia jako input (przycisk START)
	LEDG = 1;	    //LED red off
	LEDR = 0;	    //LED green  on
	echo = 1;       //ustawienie wyprowadzenia jako input (odpowiedź modułu HC-SR04)
	trig = 0;       //ustawienie niskiego stanu na pinie aktywującym pomiar
	LCD_init();
	TMOD = 0x11;    //przygotowanie Timer0 i Timer1
	while(1) 
	{
		LCD_string("Gotowe do \n pomiaru");

		while(button);  //oczekiwanie na wciśnięcie przycisku
		LEDR = 1;	    //LED red off
		trig = 1;	    //wysłanie długiego sygnału inicjującego pomiar
		delay(1);       //opóźnienie min 10us
		trig = 0;       //zakończenie impulsu inicjującego
		
		while(!echo);   //oczekiwane na stan wysoki na wyprowadzenie ECHO
		timer_start();	//początek pomiaru czasu
		while(echo);	//oczekiwanie na koniec stanu wysokiego na wyprowadzeniu ECHO
		timer_stop();	//zakończenie pomiaru czasu
		LEDG = 0;	    //LED green on
		res = (TL1 | (TH1<<8)) * 0.01860775; //obliczenie odległości
		LCD_string("wynik pomiaru: \n");
		LCD_number(res);

		while(button);  //oczekiwanie na wciśnięcie przycisku
		LEDG = 1;	    //LED green off
		delay(2000);
		LEDR = 0;	    //LED red on
	}
}

//funkcje opóźniająca ukłąd o podaną ilość ms - Timer0
void delay(unsigned int time)
{
	while(time!=0) //timer liczy po 1 ms
	{
		TL0 = 0x66;	    //ustawienie wartości LOW timera
		TH0 = 0xFC;     //ustawienie wartości HIGH timera
		TR0 = 1;	    //włączenie Timer0
		while(!TF0);    //oczekiwanie na overflow
		time--;	        //zaktualizowanie czasu
		TF0 = 0;	    //wyczyszczenie flagi overflow
	}
	TR0 = 0;	//wyłączenie Timera0
}

//wysyłanie komend do LCD
void LCD_cmd(unsigned char command)
{
	display_port = command; //ustawienie komendy na porcie LCD
	rs = 0;     //wysłanmie sygnału komendy	
	e = 1;      //włączenie sygnału przetwarzania
	delay(2);
	e = 0;      //wyłączenie sygnału przetwarzania
}

//inicjalizacja LCD
void LCD_init()
{
	delay(50);      //odczekanie min 30ms od włączenia
	LCD_cmd(0x3C);	//ustawienie funcji wyświetlacza na wyświetlanie w 2 liniach
	delay(1);		//odczekanie >39us
	LCD_cmd(0x0F);	//włączenie kolejnych funkcji wyświetlacza - kursora i migania kursora
	delay(1);		//odczekanie >39us
	LCD_cmd(0x01);	//wyczyszczenie wyświetlacza
	delay(2);		//odczekanie >1,53ms
	LCD_cmd(0x06);	//ustawienie trybu wprowadzania
	delay(1);		//odczekanie >39us
}

//wysłanie znaku do wyświetlenia na LCD
void LCD_char(unsigned char letter)
{
	display_port = letter; //ustawienie znaku na porcie LCD
	rs = 1;     //wysłanmie sygnału danych
	e = 1;      //włączenie sygnału przetwarzania
	delay(2);
	e = 0;      //wyłączenie sygnału przetwarzania
}

//wysłanie tekstu do wyświetlenia na LCD
void LCD_string(unsigned char* text)
{
	int i = 0;
	LCD_cmd(0x01);  //wyczyszczenie wyświetlacza
	delay(2);		//odczekanie >1,53ms
	while(text[i] != '\0')  //czytanie do znaku końca ciągu
	{
		if(text[i] == '\n') //przejście do kolejnej linii po wykryciu znaku
        {
			LCD_cmd(0xC0);
			delay(1);
		}
		else                //wysłanie znaku do wysłania
        {
			LCD_char(text[i]);
		}
		i++;                //odczytanie kolejnego znaku w łańcuchu
	}
}

//wysłanie obliczonej odległości do wyświetlenia na LCD
//wykorzystanie rozkładu liczby na czynniki
void LCD_number(float number)
{
	int digit = 0;  //cyfra do wyświetlenia
	int div = 100;  //dzielnik liczby do rozłożenia wyniku na cyfry
	int i;
	for(i=0; i<5; i++)  //wyświetlenie pięciu cyfr
	{		
		if (div == 0)   //ustawienie wartości do obliczenia części dziesiętnej wyniku
        {
			LCD_char(',');
			div = 10;
			number = number * 100;
		}
		digit = number / div;   //wyznaczenie cyfry - kolejnego czynnika
		LCD_char(digit + 48);   //wysłanie kodu ASCII znaku do wyświetlania
		number = number - digit * div;  //obliczenie liczby pozostałej do wyświetlenia (po odjęciu wyświetlonego współczynnika)
		div = div / 10;         //zaktualizowanie dzielnika
	}

    //wyświetlenie jednostki
	LCD_char(' ');
	LCD_char('c');
	LCD_char('m');
}

//włączenie Timera1
void timer_start()
{
	EA = 1;	    //włączenie przerwań globalnych
	ET1 = 1;	//włączenie przerwań Timera1
	//timer może liczyć do 65535
	TL1 = 0x00;	
	TH1 = 0x00; 
	TR1 = 1;	//włączenie Timera1
	
}

//procedura obsługi przerwania Timera1
void timer_ISR() interrupt 3
{
	while(1) //miganie czerwoną diodą
	{
		LEDR = 1;	//LED off
		delay(500);
		LEDR = 0;	//LED on
		delay(500);
	}
}

//wyłączenie Timera1
void timer_stop()
{
	TR1 = 0;	//wyłączenie Timera1
	EA = 0;	    //włączenie przerwań globalnych
	ET1 = 0;	//włączenie przerwań Timera1
}