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
 *  [-] Need a way to change an individual LED in that frame, defined by LED index
 *      - This is done without a function, by just writing a Colour struct directly to the frame array
 *  [ ] Need a way to change an individual LED in that frame, defined by x,y coordinates (hard to make generalized to diff displays)
 *  [x] Need a way to set the entire frame to a single colour
 *  [x] Need a way to write image frame to LED display
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

void set_colour_LED(struct Colour *frame, uint32_t LED_number, struct Colour desired_colour) {
	frame[LED_number] = desired_colour;
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

void Pattern_cycle_RGB(struct Colour *frame) {
	while (1) {
		set_colour_whole_frame(frame, Red);
		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(500);

		set_colour_whole_frame(frame, Green);
		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(500);

		set_colour_whole_frame(frame, Blue);
		send_frame(frame);
		if (FLAG_BTN) return;
		HAL_Delay(500);
	}
}



// implements a rainbow gradient as per
// https://en.wikipedia.org/wiki/HSL_and_HSV#/media/File:HSV-RGB-comparison.svg
void Pattern_RainbowGradient(struct Colour *frame) {
	while (1) {

		uint32_t delay = 2;

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
			HAL_Delay(delay);
			if (FLAG_BTN) return;
		}

		// G max, R decreasing
		current_colour.Green = 255;
		current_colour.Blue = 0;
		for (int i = 0; i < 256; i++) {
			current_colour.Red = 255 - i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			HAL_Delay(delay);
			if (FLAG_BTN) return;
		}

		// G max, B increasing
		current_colour.Red = 0;
		current_colour.Green = 255;
		for (int i = 0; i < 256; i++) {
			current_colour.Blue = i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			HAL_Delay(delay);
			if (FLAG_BTN) return;
		}

		// B max, G decreasing
		current_colour.Red = 0;
		current_colour.Blue = 255;
		for (int i = 0; i < 256; i++) {
			current_colour.Green = 255 - i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			HAL_Delay(delay);
			if (FLAG_BTN) return;
		}

		// B max, R increasing
		current_colour.Green = 0;
		current_colour.Blue = 255;
		for (int i = 0; i < 256; i++) {
			current_colour.Red = i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			HAL_Delay(delay);
			if (FLAG_BTN) return;
		}

		// R max, B decreasing
		current_colour.Red = 255;
		current_colour.Green = 0;
		for (int i = 0; i < 256; i++) {
			current_colour.Blue = 255 - i;
			set_colour_whole_frame(frame, current_colour);
			send_frame(frame);
			HAL_Delay(delay);
			if (FLAG_BTN) return;
		}
	}
}

/*
// !TODO Rewrite with new functions
void Pattern_RainbowGradientDiag(void) {
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
