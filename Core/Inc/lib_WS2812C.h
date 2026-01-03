/*
 * lib_WS2812C.h
 *
 *  Created on: Jan 2, 2026
 *      Author: Adam Gulyas
 */

#ifndef SRC_LIB_WS2812C_H_
#define SRC_LIB_WS2812C_H_

#include <stdint.h>

#define MAX_LED 9
volatile uint8_t FLAG_DataSent;

// Core Functions

void HuetoRGB(uint8_t LEDnum, uint16_t Hue);
void set_LED (uint8_t LEDnum, uint8_t Red, uint8_t Green, uint8_t Blue);
void set_all_LED(uint8_t Red, uint8_t Green, uint8_t Blue);
void WS2812C_Send(void);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);

// Patterns

void cycle_RGB(void);
void Rainbow(void);
void GradientRainbowDiag(void);

#endif /* SRC_LIB_WS2812C_H_ */
