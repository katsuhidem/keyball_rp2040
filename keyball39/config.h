// Copyright 2023 yinouet (@yinouet)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// #include "config_common.h"

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT

// For debugging
//#define POINTING_DEVICE_DEBUG

/* SPI & PMW3360 settings. */
#define SPI_DRIVER SPID0
#define SPI_SCK_PIN GP22
#define SPI_MISO_PIN GP20
#define SPI_MOSI_PIN GP23
#define POINTING_DEVICE_CS_PIN GP21

#define POINTING_DEVICE_INVERT_X
#define POINTING_DEVICE_ROTATION_90

#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_RIGHT

/* Debounce reduces chatter (unintended double-presses) - set 0 if debouncing is not needed */
#define DEBOUNCE 5

// Split parameters
#define SPLIT_HAND_MATRIX_GRID  GP27, GP9

// RGB LED settings
#define WS2812_PIO_USE_PIO1

#ifdef RGB_MATRIX_ENABLE
#    define RGB_MATRIX_SPLIT    { 24, 24 }
#endif

#ifndef OLED_FONT_H
#    define OLED_FONT_H "keyboards/keyball_rp2040/lib/glcdfont.c"
#endif

#define     I2C_DRIVER I2CD1
#define     I2C1_SDA_PIN GP2
#define     I2C1_SCL_PIN GP3

#if !defined(LAYER_STATE_8BIT) && !defined(LAYER_STATE_16BIT) && !defined(LAYER_STATE_32BIT)
#    define LAYER_STATE_8BIT
#endif