/*
 * lib_WS2812C.c
 *
 *  Created on: Jan 2, 2026
 *      Author: Adam Gulyas
 */

/*
 * What's the intended usage flow?
 *   - Define several patterns (different animations or simple programs) the user can cycle through
 *   - Define an empty image frame (pass in display dimensions)
 *      - Only do this once at the start of program
 *   - Each pattern need to be able to:
 *      - Edit image frame to be what you want
 *      - Write image frame to LED display
 *
 * What functionality do I need?
 *  [x] Need to create an image frame (an array that has all the colour data for each LED), initialized to black
 *  [ ] Need a way to change an individual LED in that frame, defined by LED index
 *  [ ] Need a way to change an individual LED in that frame, defined by x,y coordinates (hard to make generalized to diff displays)
 *  [x] Need a way to set the entire frame to a single colour
 *  [ ] Need a way to write image frame to LED display
 *
 *  - Somewhere to define the parameters of the LED display I'm using (each programmer does this in main.c)
 *  	display_width   The number of LEDs horizontally
 *  	display_height  The number of LEDs vertically
 *  	MAX_LED         The total number of LEDs in the display (would be display_width * display_height)
 *
 *
 */

#include "lib_WS2812C.h"
#include "main.h"

volatile uint8_t FLAG_DataSent = 0;


// A function that returns an instance of a Colour struct with defined RGB values
struct Colour create_colour (uint8_t Red, uint8_t Green, uint8_t Blue) {

	struct Colour Current_Colour;

	Current_Colour.Red = Red;
	Current_Colour.Green = Green;
	Current_Colour.Blue = Blue;

	return Current_Colour;
}


// Fills the pointed to array with zeroes
void clear_frame(struct Colour *frame) {
	for (size_t i = 0; i < NUM_LEDS; i++) {
		frame[i] = create_colour(0, 0, 0);
	}
}


// Arrays are passed to functions as a pointer to that array.
// Functions modify the the original array, not a copy of it you pass in.
// Therefore there's nothing to return
void set_colour_whole_frame(struct Colour *frame, struct Colour desired_colour) {

	for (size_t i = 0; i < NUM_LEDS; i++){
		frame[i] = desired_colour;
	}
}


void send_frame(struct Colour *frame) {

	static uint16_t pwmData[(24*NUM_LEDS)+600];  // 24  = 24 bits of colour data for each LED
	                                             // 600 = 300 zeros before and after actual data to hold data line low
	                                             //       needed for timing requirements

	uint32_t index = 0;    // Keeps track of our current place writing data to pwmData

	// Set first 300 elements of pwmData to 0% duty cycles to keep line low for the latch command (reset LEDs)
	for (uint16_t i = 0; i < 300; i++) {
		pwmData[index] = 0;
		index++;
	}

	uint32_t color;      // color data is 24 bits. Will hold all the RGB bits.

	for (uint32_t LED = 0; LED < NUM_LEDS; LED++) {     // for each LED

		// Concatenate color values into a single string
		color = (((uint32_t)frame[LED].Green << 16) |
				 ((uint32_t)frame[LED].Red   << 8 ) |
				 ((uint32_t)frame[LED].Blue));

		for (int bit = 23; bit >= 0; bit--) {    // for each bit of color values
			if (color & (1 << bit)) {
				pwmData[index] = 30;   // 50% duty cycle
			} else {
				pwmData[index] = 15;   // 25% duty cycle
			}
			index++;
		}
	}

	// send a bunch of 0% duty cycles to keep line low for the latch command
	for (uint16_t i = 0; i < 300; i++) {
		pwmData[index] = 0;
		index++;
	}

	// Start DMA and wait until it's done
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*) pwmData, index);
	while (!FLAG_DataSent) {};
	FLAG_DataSent = 0;

}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	FLAG_DataSent = 1;
}


// ############################################################################

/*

struct Colour make_single_colour_array(uint8_t numLEDs, uint8_t Red, uint8_t Green, uint8_t Blue) {

	struct Colour LED_Data[numLEDs];

	for (int i = 0; i < numLEDs; i++) {
		LED_Data[i] = create_colour(Red, Green, Blue);
	}

	return LED_Data;
}


// Core Functions

volatile uint8_t FLAG_DataSent = 0;



// Based on this image https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
// Hue range is 0 - 1,535 ((256*6)-1)



void HuetoRGB(uint8_t LEDnum, uint16_t Hue) {
	// The rainbow is broken up into 6 bins, defined by when R, G, B start increasing or decreasing

	if(Hue > 1535) {
		Hue = Hue - 1536;
	}

	uint8_t Red = 0;
	uint8_t Green = 0;
	uint8_t Blue = 0;

	uint8_t Bin = Hue / 256; // The rainbow is broken up into 6 bins, defined by when R, G, B start increasing or decreasing
	uint8_t x = Hue % 256;  // How far along the bin you are

	switch (Bin) {
	case 0:
		Red = 255;
		Green = x;
		break;
	case 1:
		Green = 255;
		Red = 255 - x;
		break;
	case 2:
		Green = 255;
		Blue = x;
		break;
	case 3:
		Blue = 255;
		Green = 255 - x;
		break;
	case 4:
		Blue = 255;
		Red = x;
		break;
	case 5:
		Red = 255;
		Blue = 255 - x;
		break;
	default:
		Red = 255;
		Green = 255;
		Blue = 255;
	}

	set_LED(LEDnum, Red, Green, Blue);

}

void set_LED (uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue) {
	LED_Data[LEDnum][0] = Green;
	LED_Data[LEDnum][1] = Red;
	LED_Data[LEDnum][2] = Blue;
}

void set_all_LED(uint8_t Red, uint8_t Green, uint8_t Blue) {
	for (int i = 0; i < MAX_LED; i++) {
		set_LED(i, Red, Green, Blue);
	}
}





// Patterns
// Should be able to remove this section of the library
// In other words, nothing here should be core functionality

void cycle_RGB(void) {
	while (1) {
		set_all_LED(255, 0, 0);
		WS2812C_Send();
		HAL_Delay(500);

		set_all_LED(0, 255, 0);
		WS2812C_Send();
		HAL_Delay(500);

		set_all_LED(0, 0, 255);
		WS2812C_Send();
		HAL_Delay(500);
	}
}


// implements a rainbow gradient as per
// https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
void Rainbow(void) {

	// All red as starting point
	uint8_t Red = 255;
	uint8_t Green = 0;
	uint8_t Blue = 0;
	uint32_t delay = 2;

	set_all_LED(Red, Green, Blue);
	WS2812C_Send();

	// R max, G increasing
	Blue = 0;
	for (int i = 0; i < 256; i++) {
		Green = i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// G max, R decreasing
	Blue = 0;
	for (int i = 0; i < 256; i++) {
		Red = 255 - i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// G max, B increasing
	Red = 0;
	for (int i = 0; i < 256; i++) {
		Blue = i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// B max, G decreasing
	Red = 0;
	for (int i = 0; i < 256; i++) {
		Green = 255 - i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// B max, R increasing
	Green = 0;
	for (int i = 0; i < 256; i++) {
		Red = i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

	// R max, B decreasing
	Green = 0;
	for (int i = 0; i < 256; i++) {
		Blue = 255 - i;
		set_all_LED(Red, Green, Blue);
		WS2812C_Send();
		HAL_Delay(delay);
	}

}


void GradientRainbowDiag(void) {
	while (1) {
		for (uint16_t i = 0; i < 1536; i++) {
			HuetoRGB(2, i + 160);

			HuetoRGB(1, i + 120);
			HuetoRGB(5, i + 120);

			HuetoRGB(0, i + 80);
			HuetoRGB(4, i + 80);
			HuetoRGB(8, i + 80);

			HuetoRGB(3, i + 40);
			HuetoRGB(7, i + 40);

			HuetoRGB(6, i);

			WS2812C_Send();
			HAL_Delay(20);

		}
	}
}


*/
