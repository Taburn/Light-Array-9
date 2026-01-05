/*
 * lib_WS2812C.h
 *
 *  Created on: Jan 2, 2026
 *      Author: Adam Gulyas
 */

#ifndef SRC_LIB_WS2812C_H_
#define SRC_LIB_WS2812C_H_

#include <stdint.h>
#include "stm32c0xx_hal.h"
extern TIM_HandleTypeDef htim1;

#define DISPLAY_WIDTH  3
#define DISPLAY_HEIGHT 3
#define NUM_LEDS       9

extern volatile uint8_t FLAG_DataSent;

// A struct that holds 3x 8-bit colour values
struct Colour {
	uint8_t Red;
	uint8_t Green;
	uint8_t Blue;
};

// Core Functions

struct Colour create_colour (uint8_t Red, uint8_t Green, uint8_t Blue);
void clear_frame(struct Colour *frame);
struct Colour HuetoRGB(uint16_t Hue);
void set_colour_whole_frame(struct Colour *frame, struct Colour desired_colour);
void set_colour_LED(struct Colour *frame, uint32_t LED_number, struct Colour desired_colour);
void send_frame(struct Colour *frame);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);

// Colour Definitions

extern const struct Colour Red;
extern const struct Colour Green;
extern const struct Colour Blue;
extern const struct Colour Yellow;
extern const struct Colour Purple;
extern const struct Colour Cyan;
extern const struct Colour White;
extern const struct Colour Black;


// Patterns

void Pattern_cycle_RGB(struct Colour *frame);
void Pattern_RainbowGradient(struct Colour *frame);
//void GradientRainbowDiag(void);


#endif /* SRC_LIB_WS2812C_H_ */
