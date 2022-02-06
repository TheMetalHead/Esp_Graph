/*

 This reads the value of an analogue pin, and graphs the values on the screen.

 The maximum input voltage is 3.3 volts and has no proper overvoltage protection. It should
 withstand a brief connection to 13.8 volts as it has a 220K ohm series input resistor.

 Lifted from: http://www.arduino.cc/en/Tutorial/TFTGraph

 Created 15 April 2013 by Scott Fitzgerald.

 This example code is in the public domain.

 Modified by TheMetalHead to use a Wemos D1 Mini and a TFT 1.4" shield.



 Usage:
	Pressing the 'buttonFilled' button toggles between displaying draw or filled.

	Pressing the 'buttonTimebase' button:

		Single press = Display the current timebase.
		Double press = Increment the timebase by 10mS up to _D_MAX_TIMEBASE.
		Long press   = Reset the timebase to '_D_DEFAULT_TIMEBASE'.

 */



#include	<Adafruit_GFX.h>			// Core graphics library
#include	<Adafruit_ST7735.h>			// Hardware-specific library

#include	<ClickButton.h>
#include	<elapsedMillis.h>



#ifdef ESP32

	// The ADC input channels have a 12 bit resolution. This means that you can get analogue
	// readings ranging from 0 to 4095, in which 0 corresponds to 0V and 4095 to 3.3V

	// ADC1_CH0 (GPIO 36)
	// ADC1_CH1 (GPIO 37)
	// ADC1_CH2 (GPIO 38)
	// ADC1_CH3 (GPIO 39)
	// ADC1_CH4 (GPIO 32)
	// ADC1_CH5 (GPIO 33)
	// ADC1_CH6 (GPIO 34)
	// ADC1_CH7 (GPIO 35)

	// These are not tested.

	#define	_D_TFT_RST		( 15 )
	#define	_D_TFT_CS		( 14 )
	#define	_D_TFT_DC		( 32 )
	//#define	_D_TFT_LED		( -1 )

	#define	_D_A2D_INPUT		( A0 )
	#define	_D_A2D_MAX_BITS		( 4095 )
	#define	_D_A2D_OFFSET		( 9 )

	#define	_D_MAX_INPUT_VOLTAGE	( 3300 )

#else

	// Wemos D1 Mini
	//
	// https://www.wemos.cc/en/latest/d1/d1_mini_3.1.0.html
	//
	// D0 	IO			GPIO16		Voltage selection, 3v3 on start up
	// D1 	IO, SCL			GPIO5		btn_1
	// D2 	IO, SDA			GPIO4		btn_2
	// D3 	IO, 10k Pull-up		GPIO0		_D_TFT_DC
	// D4 	IO, 10k Pull-up,	GPIO2		PB_GPIO_BUILTIN_LED
	// D5 	IO, SCK			GPIO14		SPI
	// D6 	IO, MISO 		GPIO12		SPI
	// D7 	IO, MOSI	 	GPIO13		SPI
	// D8 	IO, 10k Pull-down, SS	GPIO15		_D_TFT_CS

	// The ADC input channel has a 10 bit resolution. This means that you can get analogue
	// readings ranging from 0 to 1023, in which 0 corresponds to 0V and 1023 to 3.3V

	#define	_D_TFT_RST		( -1 )			// For TFT I2C Connector Shield V1.0.0 and TFT 1.4 Shield V1.0.0   Not used
	#define	_D_TFT_CS		( 15 )			// For TFT I2C Connector Shield V1.0.0 and TFT 1.4 Shield V1.0.0   D8
	#define	_D_TFT_DC		( 0 )			// For TFT I2C Connector Shield V1.0.0 and TFT 1.4 Shield V1.0.0   D3
	#define	_D_TFT_LED		( -1 )			// For TFT I2C Connector Shield V1.0.0 and TFT 1.4 Shield V1.0.0   Not used

	#define	_D_A2D_INPUT		( A0 )
	#define	_D_A2D_MAX_BITS		( 1023 )
	#define	_D_A2D_OFFSET		( 9 )

	#define	_D_MAX_INPUT_VOLTAGE	( 3300 )

#endif



// 1.44” diagonal LCD TFT display.
//
// https://www.wemos.cc/en/latest/d1_mini_shield/tft_1_4.html
//
// 128 x 128 resolution, 18-bit (262,144) colour.
// Driver IC: ST7735S.
//
// D7			13		MOSI
// D5			14		SCK
// NC*(D0/D3/D4/D8)	NC*(16/0/2/15)	_D_TFT_LED
// NC*(D0/D3/D4/D8)	NC*(16/0/2/15)	_D_TFT_RST
//
// ST7735_TFTWIDTH_128
// ST7735_TFTHEIGHT_128

Adafruit_ST7735		TFTscreen = Adafruit_ST7735( _D_TFT_CS, _D_TFT_DC, _D_TFT_RST );



// Available colours.
//
//	ST7735_BLACK
//	ST7735_WHITE
//	ST7735_RED
//	ST7735_GREEN
//	ST7735_BLUE
//	ST7735_CYAN
//	ST7735_MAGENTA
//	ST7735_YELLOW
//	ST7735_ORANGE



/*******************************************************************/
// Configure as required.

// Uncomment to display 'ui16_A2d_Smoothed' and 'ui16_Reading_In_Mv'.
//
//#define	_D_DEBUG_VALUES			( 1 )

// Uncomment if the timebase and filled buttons are used.
//
#define	_D_ENABLE_BUTTONS		( 1 )

// Comment out if display Y axis labels..
//
#define	_D_ENABLE_Y_AXIS_LABELS		( 1 )

/*******************************************************************/



#define	_D_SERIAL_SPEED			( 115200 )

// The oscilloscope timebase in mS.
//
#define	_D_DEFAULT_TIMEBASE		( 40 )			// 40 * ST7735_TFTHEIGHT_128 = 5,120mS
#define	_D_MAX_TIMEBASE			( 70 )			// Maximum timebase. (Multiple of 10mS):
								//   10mS = 1.28 seconds per screen width
								//   20mS = 1.56 seconds
								//   30mS = 3.84 seconds
								//   40mS = 5.12 seconds
								//   50mS = 6.4 seconds
								//   60mS = 7.68 seconds
								//   70mS = 8.96 seconds

// The displayed voltage position.
//
#define	_D_VOLTAGE_X_POS		( 5 )
#define	_D_VOLTAGE_Y_POS		( 7 )

#define	_D_TIMEBASE_X_POS		( 55 )
#define	_D_TIMEBASE_Y_POS		( 7 )

#define	_D_SCREENTIME_X_POS		( 45 + _D_TIMEBASE_X_POS )
#define	_D_SCREENTIME_Y_POS		( _D_TIMEBASE_Y_POS )

// The font size.
//
#define	_D_FONT_HEIGHT			( 7 )
#define	_D_FONT_WIDTH			( 5 )

// The colours.
//
#define	_D_BACKGROUND_COLOUR		( ST7735_BLACK )
#define	_D_FOREGROUND_COLOUR		( ST7735_GREEN )

#define	_D_GRID_COLOUR			( ST7735_BLUE )
#define	_D_OSCILLOSCOPE_COLOUR		( ST7735_YELLOW )



#ifdef	_D_ENABLE_BUTTONS

// The Buttons.
//
const	int	button_Timebase_Pin = 5;			// D1
const	int	button_Filled_Pin = 4;				// D2



ClickButton	buttonTimebase( button_Timebase_Pin, HIGH );
ClickButton	buttonFilled(   button_Filled_Pin,   HIGH );

#endif



#ifdef	_D_DEBUG_VALUES

elapsedMillis	str_Millis;

#endif

elapsedMillis	str_Millis_Sample;
elapsedMillis	str_Millis_Timebase;

bool		b_Draw_Filled;

int16_t		i16_Scope_X_Pos;				// Position of the line on screen
int16_t		i16_Scope_Last_X_Pos;				// Position of the last line on screen

uint8_t		ui8_Timebase;
uint8_t		ui8_Timebase_Display_Count;
uint8_t		ui8_A2d_Sample_Count;

uint16_t	ui16_A2d_Smoothed;
uint16_t	ui16_Last_Draw_Height;
uint16_t	ui16_Max_Height;
uint16_t	ui16_Reading_In_Mv;

uint32_t	ui32_A2d_Total;



static	void	_Write_Char( const char _cc_chr ) {
	// We need an extra blank pixel line around the character being written.

	TFTscreen.drawFastHLine( TFTscreen.getCursorX(), TFTscreen.getCursorY(), 2 + _D_FONT_WIDTH,  _D_BACKGROUND_COLOUR );
	TFTscreen.drawFastVLine( TFTscreen.getCursorX(), TFTscreen.getCursorY(), 2 + _D_FONT_HEIGHT, _D_BACKGROUND_COLOUR );

	// Display the character after the blank pixel line.

	TFTscreen.setCursor( 1 + TFTscreen.getCursorX(), 1 + TFTscreen.getCursorY() );

	TFTscreen.setTextColor( _D_FOREGROUND_COLOUR, _D_BACKGROUND_COLOUR );
	TFTscreen.write( _cc_chr );

	// Calculate and set the new cursor position.

	TFTscreen.setCursor( TFTscreen.getCursorX() - 1, TFTscreen.getCursorY() - 1 );
}



static	void	_Write_Digit( uint8_t _ui8_val ) {
	_Write_Char( ( const char ) ( '0' + _ui8_val ) );
}



static	void	_Write_Decimal_Point() {
	_Write_Char( '.' );
}



static	void	_Write_Null_Character() {
	_Write_Char( '\0' );
}


static	void	_Clear_Screen() {
///	TFTscreen.fillScreen( _D_BACKGROUND_COLOUR );

	// Only clear the lower part of the display.

	TFTscreen.fillRect( 1, ST7735_TFTHEIGHT_128 - 1 - ui16_Max_Height, ST7735_TFTWIDTH_128 - 2, ui16_Max_Height, _D_BACKGROUND_COLOUR );

	// Clear the timebase and screen time if it was displayed and is now not required.

	if ( 0 == ui8_Timebase_Display_Count ) {
		TFTscreen.setCursor( _D_TIMEBASE_X_POS, _D_TIMEBASE_Y_POS );
		_Write_Null_Character();
		_Write_Null_Character();
		_Write_Null_Character();
		_Write_Null_Character();

		TFTscreen.setCursor( _D_SCREENTIME_X_POS, _D_SCREENTIME_Y_POS );
		_Write_Null_Character();
		_Write_Null_Character();
		_Write_Null_Character();
		_Write_Null_Character();
	}

	// Draw the grid every 500mV.

	uint16_t	_ui16_Y_Pos;


	for ( i16_Scope_X_Pos = 0; i16_Scope_X_Pos < _D_MAX_INPUT_VOLTAGE; i16_Scope_X_Pos += 500 ) {
		_ui16_Y_Pos = map( i16_Scope_X_Pos, 0, _D_MAX_INPUT_VOLTAGE + 1, 0, ST7735_TFTHEIGHT_128 );

		TFTscreen.drawFastHLine( 1, ST7735_TFTHEIGHT_128 - _ui16_Y_Pos, ST7735_TFTWIDTH_128 - 2, _D_GRID_COLOUR );

#ifdef	_D_ENABLE_Y_AXIS_LABELS

		TFTscreen.setCursor( 112, ST7735_TFTHEIGHT_128 - 5 - _ui16_Y_Pos );

		if ( 1000 == i16_Scope_X_Pos ) {
			_Write_Char( '1' );
			_Write_Char( 'v' );
		} else if ( 2000 == i16_Scope_X_Pos ) {
			_Write_Char( '2' );
			_Write_Char( 'v' );
		} else if ( 3000 == i16_Scope_X_Pos ) {
			// If the timebase is not being displayed.

			if ( 0 == ui8_Timebase_Display_Count ) {
				_Write_Char( '3' );
				_Write_Char( 'v' );
			}
		}

#endif

	}

	// Reset some variables.

	ui16_Max_Height = 0;

	i16_Scope_X_Pos = 0;
	i16_Scope_Last_X_Pos = 0;
}



static	void	_Write_String( const char *_ccp_str ) {
	while ( *_ccp_str) {
		_Write_Char( *_ccp_str++ );
	}
}



#if 0

static	void	_Write_String_At_XY( uint8_t _ui8_x, uint8_t _ui8_y, const char *_ccp_str ) {
	TFTscreen.setCursor( _ui8_x, _ui8_y );

	while ( *_ccp_str) {
		_Write_Char( *_ccp_str++ );
	}
}

#endif



static	void	_Display_Voltage( uint16_t _ui16_val ) {
	TFTscreen.setCursor( _D_VOLTAGE_X_POS, _D_VOLTAGE_Y_POS );

	_Write_Digit( ( uint8_t ) ( _ui16_val / 1000 ) );	// Write the units digit

	_ui16_val %= 1000;

	_Write_Decimal_Point();

	_Write_Digit( ( uint8_t ) ( _ui16_val / 100 ) );	// Write the tenths digit

	_ui16_val %= 100;

	_Write_Digit( ( uint8_t ) ( _ui16_val / 10 ) );		// Write the hundredths digit

	_ui16_val %= 10;

	_Write_Digit( ( uint8_t ) ( _ui16_val ) );		// Write the thousandths digit

	_Write_Char( 'v' );
}



static	void	_Display_Value( uint16_t _ui16_val ) {
	_Write_Digit( ( uint8_t ) ( _ui16_val / 1000 ) );

	_ui16_val %= 1000;

	_Write_Digit( ( uint8_t ) ( _ui16_val / 100 ) );

	_ui16_val %= 100;

	_Write_Digit( ( uint8_t ) ( _ui16_val / 10 ) );

	_ui16_val %= 10;

	_Write_Digit( ( uint8_t ) ( _ui16_val ) );
}



static	void	_Display_Timebase() {
	TFTscreen.setCursor( _D_TIMEBASE_X_POS, _D_TIMEBASE_Y_POS );

	if ( ui8_Timebase > 9 ) {
		_Write_Digit( ( uint8_t ) ( ui8_Timebase / 10 ) );	// Write the tenths digit
	} else {
		_Write_Null_Character();
	}

	_Write_Digit( ( uint8_t ) ( ui8_Timebase % 10 ) );		// Write the units digit

	_Write_String( "mS" );


#ifndef	_D_DEBUG_VALUES

	uint16_t	_u16_Screen_time;


	_u16_Screen_time = ST7735_TFTHEIGHT_128 * ui8_Timebase;		// Calculate the screen refresh time

	TFTscreen.setCursor( _D_SCREENTIME_X_POS, _D_SCREENTIME_Y_POS );

	_Write_Digit( ( uint8_t ) ( _u16_Screen_time / 1000 ) );	// Write the units digit

	_u16_Screen_time %= 1000;

	_Write_Decimal_Point();

	_Write_Digit( ( uint8_t ) ( _u16_Screen_time / 100 ) );		// Write the tenths digit

	_u16_Screen_time %= 100;

	_Write_Digit( ( uint8_t ) ( _u16_Screen_time / 10 ) );		// Write the hundredths digit

#endif
}



static	void	_Reset_Display_Timebase() {
	ui8_Timebase_Display_Count = 3;
}



static	void	_Enable_Display_Timebase() {
	_Reset_Display_Timebase();
	_Display_Timebase();
}



#ifdef	_D_ENABLE_BUTTONS

static	void	_Button_Display_Timebase() {
	if ( 1 == buttonTimebase.clicks ) {			// If we need to display the timebase
		_Enable_Display_Timebase();
	}
}



static	void	_Button_Change_Timebase() {
	if ( 2 == buttonTimebase.clicks ) {			// If we need to change the timebase
		ui8_Timebase += 10;

		if ( ui8_Timebase > _D_MAX_TIMEBASE ) {
			ui8_Timebase = 10;
		}

		_Enable_Display_Timebase();

		Serial.print( F( "Button 1 pressed. Timebase = " ) );
		Serial.print( ui8_Timebase );
		Serial.println( F( "mS" ) );
	}
}



static	void	_Button_Reset_Timebase() {
	if ( ( char ) -1 == buttonTimebase.clicks ) {		// If we need to reset the timebase to its default
		ui8_Timebase = _D_DEFAULT_TIMEBASE;

		_Enable_Display_Timebase();

		Serial.println( F( "Button 1 pressed. Reset timebase." ) );
	}
}



static	void	_Button_Toggle_Draw_Filled() {
	if ( 1 == buttonFilled.clicks ) {			// If we need to toggle the draw/filled mode
		b_Draw_Filled = ! b_Draw_Filled;

		if ( b_Draw_Filled ) {
			Serial.println( F( "Button 2 pressed. Filled." ) );
		} else {
			Serial.println( F( "Button 2 pressed. Draw." ) );
		}
	}
}

#endif



// This is the A2D sample loop.

static	void	_Grab_Sample_Input() {
	if ( str_Millis_Sample > 1 ) {				// Sample the analogue pin every 1mS
		str_Millis_Sample = 0;

		// Read the A2D and average it.

		ui32_A2d_Total += analogRead( _D_A2D_INPUT );

		uint8_t	_ui8_samples = ui8_Timebase / 3;

		if ( 0 == _ui8_samples ) {
			_ui8_samples = 1;
		}

		if ( _ui8_samples <= ++ui8_A2d_Sample_Count ) {
			ui8_A2d_Sample_Count = 0;

			ui16_A2d_Smoothed = ui32_A2d_Total / _ui8_samples;
			ui32_A2d_Total = 0;

			if ( ui16_A2d_Smoothed >= _D_A2D_OFFSET ) {
				ui16_A2d_Smoothed -= _D_A2D_OFFSET;
			}

			ui16_Reading_In_Mv = map( ui16_A2d_Smoothed, 0, _D_A2D_MAX_BITS + 1, 0, _D_MAX_INPUT_VOLTAGE );
		}	// End of 'if ( _ui8_samples <= ++ui8_A2d_Sample_Count ) {'
	}		// End of 'if ( str_Millis_Sample > 1 ) {'
}



// This is the oscilloscope display loop.

static	void	_Display_Oscilloscope() {
	if ( str_Millis_Timebase > ui8_Timebase ) {
		str_Millis_Timebase = 0;

		_Display_Voltage( ui16_Reading_In_Mv );

		// Map it to the screen height.

		uint16_t	_ui16_Draw_Height = map( ui16_Reading_In_Mv, 0, _D_MAX_INPUT_VOLTAGE + 1, 0, ST7735_TFTHEIGHT_128 );

// There's also the mathematical way of doing it using slope-intercept form: y = mx + b where the slope (m) is
// found by the ratio of the two ranges and the y intersection is 0.
// See: https://en.wikipedia.org/wiki/Linear_equation#Slope-intercept_form
// Since the intersection with the y-axis is at 0, we can ignore it and the equation simplifies to y = mx.
// The value of m (slope) can be found using the concept of rise over run. The rise is the change in y value,
// so 0..11 in our case. The run is the change in x, or 0..1024. So m = 11 /1024.
//
// uint16_t	_ui16_Draw_Height = ( uint16_t ) ST7735_TFTHEIGHT_128 * ui16_Reading_In_Mv / _D_A2D_MAX_BITS;

		_ui16_Draw_Height++;				// We should display the zero line

		if ( b_Draw_Filled ) {
			// Draw the filled line.

			TFTscreen.drawFastVLine( i16_Scope_X_Pos, ST7735_TFTHEIGHT_128 - _ui16_Draw_Height, _ui16_Draw_Height, _D_OSCILLOSCOPE_COLOUR );
		} else {
			// Draw as an oscilloscope would.

			TFTscreen.drawLine( i16_Scope_Last_X_Pos, ST7735_TFTHEIGHT_128 - ui16_Last_Draw_Height, i16_Scope_X_Pos, ST7735_TFTHEIGHT_128 - _ui16_Draw_Height, _D_OSCILLOSCOPE_COLOUR );

			ui16_Last_Draw_Height = _ui16_Draw_Height;
			i16_Scope_Last_X_Pos = i16_Scope_X_Pos;
		}	// End of 'if ( b_Draw_Filled ) {'

		if ( ui16_Max_Height < _ui16_Draw_Height ) {
			ui16_Max_Height = _ui16_Draw_Height;
		}

		// If the graph has reached the screen edge erase the screen and start again.

		if ( ST7735_TFTWIDTH_128 <= ++i16_Scope_X_Pos ) {
			_Clear_Screen();

			if ( ui8_Timebase_Display_Count ) {
				ui8_Timebase_Display_Count--;
			}

			// If the timebase needs to be displayed.

			if ( ui8_Timebase_Display_Count ) {
				_Display_Timebase();
			}
		}	// End of 'if ( ST7735_TFTHEIGHT_128 <= ++i16_Scope_X_Pos ) {'
	}	// End of 'if ( str_Millis_Timebase > ui8_Timebase ) {'
}



void setup() {
	// Initialise the serial port.

	Serial.begin( _D_SERIAL_SPEED );

	delay( 500 );

	// While the serial stream is not open, do nothing.

	while ( ! Serial ) {
		// Do nothing.
	}

	Serial.println();
	Serial.println( F( "Initialising." ) );

	TFTscreen.initR( INITR_144GREENTAB );

	// SPI speed defaults to SPI_DEFAULT_FREQ defined in the library, you can override it here
	// Note that speed allowable depends on chip and quality of wiring, if you go too fast, you
	// may end up with a black screen some times, or all the time.

	//TFTscreen.setSPISpeed( 40000000 );

	//TFTscreen.setTextWrap( false );			// Allow text to run off right edge
	TFTscreen.setRotation( 1 );
	TFTscreen.setTextSize( 1 );

	ui8_Timebase = _D_DEFAULT_TIMEBASE;			// Must come before '_Enable_Display_Timebase()'

	ui16_Max_Height = ST7735_TFTHEIGHT_128 - 2;

	_Reset_Display_Timebase();				// Must come before '_Clear_Screen()'
	_Clear_Screen();

	// Draw the oscilloscope outline.

	TFTscreen.drawRect( 0, 0, ST7735_TFTWIDTH_128, ST7735_TFTHEIGHT_128, _D_OSCILLOSCOPE_COLOUR );

	_Enable_Display_Timebase();

	b_Draw_Filled = false;					// The reading is displayed as filled from the bottom

	ui8_A2d_Sample_Count = 0;

	ui16_Reading_In_Mv = 0;
	ui16_A2d_Smoothed = 0;
	ui16_Last_Draw_Height = 0;

	ui32_A2d_Total = 0;

	Serial.println( F( "Initialising done." ) );
}



void loop() {
#ifdef	_D_ENABLE_BUTTONS

	// ESD sanity.

	pinMode( button_Timebase_Pin, INPUT );
	pinMode( button_Filled_Pin,   INPUT );

	// Update the button states.

	buttonTimebase.Update();
	buttonFilled.Update();

	_Button_Display_Timebase();
	_Button_Change_Timebase();
	_Button_Reset_Timebase();
	_Button_Toggle_Draw_Filled();

#endif

	_Grab_Sample_Input();
	_Display_Oscilloscope();

#ifdef	_D_DEBUG_VALUES

	if ( str_Millis > 200 ) {
		str_Millis = 0;

		TFTscreen.setCursor( 100, _D_VOLTAGE_Y_POS );
		_Display_Value( ui16_A2d_Smoothed );

		TFTscreen.setCursor( 100, _D_VOLTAGE_Y_POS + 10 );
		_Display_Value( ui16_Reading_In_Mv );
	}

#endif

}
