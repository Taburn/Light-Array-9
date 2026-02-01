/*
 * lib_WS2812C.c
 *
 *  Created on: Jan 2, 2026
 *      Author: Adam Gulyas
 */

#include "lib_WS2812C.h"
#include "main.h"
#include "stdlib.h"   // for rand()

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

// Based on this image https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
// Hue range is 0 - 1,535 ((256*6)-1)
struct Colour HuetoRGB(uint16_t Hue) {
	// The rainbow is broken up into 6 bins, defined by when R, G, B start increasing or decreasing

	// overflow correction
	Hue %= 1536;

	uint8_t Red   = 0;
	uint8_t Green = 0;
	uint8_t Blue  = 0;

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
	default:         // default case should be unreachable, but included for debugging
		Red = 0;
		Green = 0;
		Blue = 0;
	}

	struct Colour return_colour = {.Red=Red, .Green=Green, .Blue=Blue};
	return return_colour;
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

// Predefined colours
// Can also use the create_colour function
//                              R    G    B
const struct Colour Red    = {255,   0,   0};
const struct Colour Green  = {  0, 255,   0};
const struct Colour Blue   = {  0,   0, 255};
const struct Colour Yellow = {255,  90,   0};
const struct Colour Purple = {255,   0, 150};
const struct Colour Cyan   = {  0, 255, 255};
const struct Colour White  = {255, 100, 100};
const struct Colour Black  = {  0,   0,   0};


// Patterns
// Should be able to remove this section of the library
// In other words, nothing here should be core functionality


// Uses the knob to pick a single colour
void Pattern_SingleColour(struct Colour *frame) {

	//uint16_t i = 0;

	while (1) {
		//i = Read_ADC()/43;  //divide ADC range into 6 areas (max ADC = 4095)

		switch (Read_ADC()/43) {
		case 0:
			set_colour_whole_frame(frame, Red);
			break;
		case 1:
			set_colour_whole_frame(frame, Yellow);
			break;
		case 2:
			set_colour_whole_frame(frame, Green);
			break;
		case 3:
			set_colour_whole_frame(frame, Cyan);
			break;
		case 4:
			set_colour_whole_frame(frame, Blue);
			break;
		case 5:
			set_colour_whole_frame(frame, Purple);
			break;
		default:
			set_colour_whole_frame(frame, White);
		}

		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(100);
	}
}



// Cycles all LEDs through Red, Green, then Blue
void Pattern_CycleRGB(struct Colour *frame) {
	while (1) {
		set_colour_whole_frame(frame, Red);
		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(Read_ADC()*10);

		set_colour_whole_frame(frame, Green);
		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(Read_ADC()*10);

		set_colour_whole_frame(frame, Blue);
		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(Read_ADC()*10);
	}
}


// Implements a rainbow gradient as per
// https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
// !TODO Rewrite this to use the Hue to RGB function. Should reduce size a lot.
void Pattern_RainbowGradient(struct Colour *frame) {
	while (1) {

		// uint32_t delay = 2;

		// All red as starting point
		struct Colour current_colour = { .Red = 255, .Green = 0, .Blue = 0 };
		set_colour_whole_frame(frame, current_colour);
		send_frame(frame);

		// R max, G increasing
		current_colour.Red = 255;
		current_colour.Blue = 0;
		for (int i = 0; i < 256; i++) {
			current_colour.Green = i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()/8);
		}

		// G max, R decreasing
		current_colour.Green = 255;
		current_colour.Blue = 0;
		for (int i = 0; i < 256; i++) {
			current_colour.Red = 255 - i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()/8);
		}

		// G max, B increasing
		current_colour.Red = 0;
		current_colour.Green = 255;
		for (int i = 0; i < 256; i++) {
			current_colour.Blue = i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()/8);
		}

		// B max, G decreasing
		current_colour.Red = 0;
		current_colour.Blue = 255;
		for (int i = 0; i < 256; i++) {
			current_colour.Green = 255 - i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()/8);
		}

		// B max, R increasing
		current_colour.Green = 0;
		current_colour.Blue = 255;
		for (int i = 0; i < 256; i++) {
			current_colour.Red = i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()/8);
		}

		// R max, B decreasing
		current_colour.Red = 255;
		current_colour.Green = 0;
		for (int i = 0; i < 256; i++) {
			current_colour.Blue = 255 - i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()/8);
		}
	}
}



// Cycles all LEDs through a rainbow gradient, but colour changes
// sweep diagonally down the LED array
void Pattern_RainbowGradientDiag(struct Colour *frame) {
	while (1) {
		for (uint16_t i = 0; i < 1536; i=i+5) {
			frame[2] = HuetoRGB(i+160);

			frame[1] = HuetoRGB(i+120);
			frame[5] = HuetoRGB(i+120);

			frame[0] = HuetoRGB(i+80);
			frame[4] = HuetoRGB(i+80);
			frame[8] = HuetoRGB(i+80);

			frame[3] = HuetoRGB(i+40);
			frame[7] = HuetoRGB(i+40);

			frame[6] = HuetoRGB(i);

			send_frame(frame);
			if (FLAG_BTN) return;
			HAL_Delay(Read_ADC()*10);

		}
	}
}


void Pattern_RandomFade(struct Colour *frame) {
	uint8_t  brightness[NUM_LEDS];      // keeps track of how bright the colour is for each LED
	uint32_t delay_period = 30;         // frame duration in milliseconds
	uint8_t  new_LED_probability = 10;  // probability of a new LED every period = 1/new_LED_probability
	uint8_t  brightness_decrease = 3;   // max brightness is 255

	//initialize brightness
	for (uint32_t i = 0; i < NUM_LEDS; i++) { brightness[i] = 0; }

	// This sets the default/background colour
	set_colour_whole_frame(frame, Black);
	for (uint32_t i = 0; i < NUM_LEDS; i++) {
		frame[i].Blue  = 100;
	}

	while (1) {

		// decrease brightness of all LEDs by one step
		for (uint32_t i = 0; i < NUM_LEDS; i++) {
			if (brightness[i] <= brightness_decrease) {
				brightness[i] = 0;
			} else {
				brightness[i] = brightness[i] - brightness_decrease;
			}

			//frame[i].Red   = brightness[i];
			frame[i].Green = brightness[i];
			//frame[i].Blue  = brightness[i];
		}

		// chance to pick a random LED (doesn't happen every frame)
		if ((rand() % new_LED_probability) == 0) {

			uint32_t chosen_LED = rand() % NUM_LEDS;
			brightness[chosen_LED] = 255;

			// Set it to full brightness
			//frame[chosen_LED].Red   = 255;
			frame[chosen_LED].Green = 255;
			//frame[chosen_LED].Blue  = 255;

		}

		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(delay_period);
	}
}





