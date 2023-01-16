/*
 * A V4L2 driver for sc530ai Raw cameras.
 *
 * Copyright (c) 2021 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *    Liang WeiJie <liangweijie@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("gqw");
MODULE_DESCRIPTION("A low-level driver for sc530ai sensors");
MODULE_LICENSE("GPL");

#define MCLK				(27 * 1000 * 1000)
#define V4L2_IDENT_SENSOR	0x9e39

#define HDR_RATIO			32

/*
 * The sc530ai i2c address
 */
#define I2C_ADDR			0x60      //0x60   0x64

#define SENSOR_NUM			0x1
#define SENSOR_NAME			"sc530ai_mipi"

static int sc530ai_sensor_vts;

/*
 * The default register settings
 */

static struct regval_list sensor_default_regs[] = {

};

static struct regval_list sensor_2880x1620_30fps_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x1e},
	{0x3250, 0x40},
	{0x3251, 0x98},
	{0x3253, 0x0c},
	{0x325f, 0x20},
	{0x3301, 0x08},
	{0x3304, 0x50},
	{0x3306, 0x78},
	{0x3308, 0x14},
	{0x3309, 0x70},
	{0x330a, 0x00},
	{0x330b, 0xd8},
	{0x330d, 0x10},
	{0x331e, 0x41},
	{0x331f, 0x61},
	{0x3333, 0x10},
	{0x335d, 0x60},
	{0x335e, 0x06},
	{0x335f, 0x08},
	{0x3364, 0x56},
	{0x3366, 0x01},
	{0x337c, 0x02},
	{0x337d, 0x0a},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x08},
	{0x3394, 0x08},
	{0x3395, 0x08},
	{0x3396, 0x40},
	{0x3397, 0x48},
	{0x3398, 0x4b},
	{0x3399, 0x08},
	{0x339a, 0x08},
	{0x339b, 0x08},
	{0x339c, 0x1d},
	{0x33a2, 0x04},
	{0x33ae, 0x30},
	{0x33af, 0x50},
	{0x33b1, 0x80},
	{0x33b2, 0x48},
	{0x33b3, 0x30},
	{0x349f, 0x02},
	{0x34a6, 0x48},
	{0x34a7, 0x4b},
	{0x34a8, 0x30},
	{0x34a9, 0x18},
	{0x34f8, 0x5f},
	{0x34f9, 0x08},
	{0x3632, 0x48},
	{0x3633, 0x32},
	{0x3637, 0x29},
	{0x3638, 0xc1},
	{0x363b, 0x20},
	{0x363d, 0x02},
	{0x3670, 0x09},
	{0x3674, 0x8b},
	{0x3675, 0xc6},
	{0x3676, 0x8b},
	{0x367c, 0x40},
	{0x367d, 0x48},
	{0x3690, 0x32},
	{0x3691, 0x43},
	{0x3692, 0x33},
	{0x3693, 0x40},
	{0x3694, 0x4b},
	{0x3698, 0x85},
	{0x3699, 0x8f},
	{0x369a, 0xa0},
	{0x369b, 0xc3},
	{0x36a2, 0x49},
	{0x36a3, 0x4b},
	{0x36a4, 0x4f},
	{0x36d0, 0x01},
	{0x36ec, 0x13},
	{0x370f, 0x01},
	{0x3722, 0x00},
	{0x3728, 0x10},
	{0x37b0, 0x03},
	{0x37b1, 0x03},
	{0x37b2, 0x83},
	{0x37b3, 0x48},
	{0x37b4, 0x49},
	{0x37fb, 0x25},
	{0x37fc, 0x01},
	{0x3901, 0x00},
	{0x3902, 0xc5},
	{0x3904, 0x08},
	{0x3905, 0x8c},
	{0x3909, 0x00},
	{0x391d, 0x04},
	{0x391f, 0x44},
	{0x3926, 0x21},
	{0x3929, 0x18},
	{0x3933, 0x82},
	{0x3934, 0x0a},
	{0x3937, 0x5f},
	{0x3939, 0x00},
	{0x393a, 0x00},
	{0x39dc, 0x02},
	{0x3e01, 0xcd},
	{0x3e02, 0xa0},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4837, 0x28},
	{0x5010, 0x10},
	{0x5799, 0x06},
	{0x57ad, 0x00},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x2a},
	{0x5ae4, 0x24},
	{0x5ae5, 0x30},
	{0x5ae6, 0x2a},
	{0x5ae7, 0x24},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x2a},
	{0x5af6, 0x24},
	{0x5af7, 0x30},
	{0x5af8, 0x2a},
	{0x5af9, 0x24},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x44},
	{0x37f9, 0x34},
	{0x0100, 0x01},
};

static struct regval_list sensor_2880x1620_20fps_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x1e},
	{0x320e, 0x09},//
	{0x320f, 0xab},//
	{0x3250, 0x40},
	{0x3251, 0x98},
	{0x3253, 0x0c},
	{0x325f, 0x20},
	{0x3301, 0x08},
	{0x3304, 0x50},
	{0x3306, 0x78},
	{0x3308, 0x14},
	{0x3309, 0x70},
	{0x330a, 0x00},
	{0x330b, 0xd8},
	{0x330d, 0x10},
	{0x331e, 0x41},
	{0x331f, 0x61},
	{0x3333, 0x10},
	{0x335d, 0x60},
	{0x335e, 0x06},
	{0x335f, 0x08},
	{0x3364, 0x56},
	{0x3366, 0x01},
	{0x337c, 0x02},
	{0x337d, 0x0a},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x08},
	{0x3394, 0x08},
	{0x3395, 0x08},
	{0x3396, 0x40},
	{0x3397, 0x48},
	{0x3398, 0x4b},
	{0x3399, 0x08},
	{0x339a, 0x08},
	{0x339b, 0x08},
	{0x339c, 0x1d},
	{0x33a2, 0x04},
	{0x33ae, 0x30},
	{0x33af, 0x50},
	{0x33b1, 0x80},
	{0x33b2, 0x48},
	{0x33b3, 0x30},
	{0x349f, 0x02},
	{0x34a6, 0x48},
	{0x34a7, 0x4b},
	{0x34a8, 0x30},
	{0x34a9, 0x18},
	{0x34f8, 0x5f},
	{0x34f9, 0x08},
	{0x3632, 0x48},
	{0x3633, 0x32},
	{0x3637, 0x29},
	{0x3638, 0xc1},
	{0x363b, 0x20},
	{0x363d, 0x02},
	{0x3670, 0x09},
	{0x3674, 0x8b},
	{0x3675, 0xc6},
	{0x3676, 0x8b},
	{0x367c, 0x40},
	{0x367d, 0x48},
	{0x3690, 0x32},
	{0x3691, 0x43},
	{0x3692, 0x33},
	{0x3693, 0x40},
	{0x3694, 0x4b},
	{0x3698, 0x85},
	{0x3699, 0x8f},
	{0x369a, 0xa0},
	{0x369b, 0xc3},
	{0x36a2, 0x49},
	{0x36a3, 0x4b},
	{0x36a4, 0x4f},
	{0x36d0, 0x01},
	{0x36ec, 0x13},
	{0x370f, 0x01},
	{0x3722, 0x00},
	{0x3728, 0x10},
	{0x37b0, 0x03},
	{0x37b1, 0x03},
	{0x37b2, 0x83},
	{0x37b3, 0x48},
	{0x37b4, 0x49},
	{0x37fb, 0x25},
	{0x37fc, 0x01},
	{0x3901, 0x00},
	{0x3902, 0xc5},
	{0x3904, 0x08},
	{0x3905, 0x8c},
	{0x3909, 0x00},
	{0x391d, 0x04},
	{0x391f, 0x44},
	{0x3926, 0x21},
	{0x3929, 0x18},
	{0x3933, 0x82},
	{0x3934, 0x0a},
	{0x3937, 0x5f},
	{0x3939, 0x00},
	{0x393a, 0x00},
	{0x39dc, 0x02},
	{0x3e01, 0xcd},
	{0x3e02, 0xa0},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4837, 0x28},
	{0x5010, 0x10},
	{0x5799, 0x06},
	{0x57ad, 0x00},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x2a},
	{0x5ae4, 0x24},
	{0x5ae5, 0x30},
	{0x5ae6, 0x2a},
	{0x5ae7, 0x24},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x2a},
	{0x5af6, 0x24},
	{0x5af7, 0x30},
	{0x5af8, 0x2a},
	{0x5af9, 0x24},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x44},
	{0x37f9, 0x34},
	{0x0100, 0x01},
};

static struct regval_list sensor_2880x1620_60fps_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x1d},
	{0x3250, 0x40},
	{0x3251, 0x98},
	{0x3253, 0x0c},
	{0x325f, 0x20},
	{0x3301, 0x08},
	{0x3304, 0x58},
	{0x3306, 0xa0},
	{0x3308, 0x14},
	{0x3309, 0x50},
	{0x330a, 0x01},
	{0x330b, 0x10},
	{0x330d, 0x10},
	{0x331e, 0x49},
	{0x331f, 0x41},
	{0x3333, 0x10},
	{0x335d, 0x60},
	{0x335e, 0x06},
	{0x335f, 0x08},
	{0x3364, 0x56},
	{0x3366, 0x01},
	{0x337c, 0x02},
	{0x337d, 0x0a},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x08},
	{0x3394, 0x08},
	{0x3395, 0x08},
	{0x3396, 0x48},
	{0x3397, 0x4b},
	{0x3398, 0x4f},
	{0x3399, 0x0a},
	{0x339a, 0x0a},
	{0x339b, 0x10},
	{0x339c, 0x22},
	{0x33a2, 0x04},
	{0x33ad, 0x24},
	{0x33ae, 0x38},
	{0x33af, 0x38},
	{0x33b1, 0x80},
	{0x33b2, 0x48},
	{0x33b3, 0x20},
	{0x349f, 0x02},
	{0x34a6, 0x48},
	{0x34a7, 0x4b},
	{0x34a8, 0x20},
	{0x34a9, 0x18},
	{0x34f8, 0x5f},
	{0x34f9, 0x04},
	{0x3632, 0x48},
	{0x3633, 0x32},
	{0x3637, 0x29},
	{0x3638, 0xc1},
	{0x363b, 0x20},
	{0x363d, 0x02},
	{0x3670, 0x09},
	{0x3674, 0x88},
	{0x3675, 0x88},
	{0x3676, 0x88},
	{0x367c, 0x40},
	{0x367d, 0x48},
	{0x3690, 0x33},
	{0x3691, 0x34},
	{0x3692, 0x55},
	{0x3693, 0x4b},
	{0x3694, 0x4f},
	{0x3698, 0x85},
	{0x3699, 0x8f},
	{0x369a, 0xa0},
	{0x369b, 0xc3},
	{0x36a2, 0x49},
	{0x36a3, 0x4b},
	{0x36a4, 0x4f},
	{0x36d0, 0x01},
	{0x370f, 0x01},
	{0x3722, 0x00},
	{0x3728, 0x10},
	{0x37b0, 0x03},
	{0x37b1, 0x03},
	{0x37b2, 0x83},
	{0x37b3, 0x48},
	{0x37b4, 0x4f},
	{0x3901, 0x00},
	{0x3902, 0xc5},
	{0x3904, 0x08},
	{0x3905, 0x8d},
	{0x3909, 0x00},
	{0x391d, 0x04},
	{0x3926, 0x21},
	{0x3929, 0x18},
	{0x3933, 0x82},
	{0x3934, 0x08},
	{0x3937, 0x5b},
	{0x3939, 0x00},
	{0x393a, 0x01},
	{0x39dc, 0x02},
	{0x3e01, 0xcd},
	{0x3e02, 0xa0},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x5010, 0x10},
	{0x5799, 0x06},
	{0x57ad, 0x00},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x2a},
	{0x5ae4, 0x24},
	{0x5ae5, 0x30},
	{0x5ae6, 0x2a},
	{0x5ae7, 0x24},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x2a},
	{0x5af6, 0x24},
	{0x5af7, 0x30},
	{0x5af8, 0x2a},
	{0x5af9, 0x24},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x44},
	{0x37f9, 0x44},
	{0x0100, 0x01},
};

static struct regval_list sensor_2880x1620_30fps_shdr_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x1c},
	{0x320e, 0x0c},
	{0x320f, 0xe4},
	{0x3250, 0xff},
	{0x3251, 0x98},
	{0x3253, 0x0c},
	{0x325f, 0x20},
	{0x3281, 0x01},
	{0x3301, 0x08},
	{0x3304, 0x58},
	{0x3306, 0xa0},
	{0x3308, 0x14},
	{0x3309, 0x50},
	{0x330a, 0x01},
	{0x330b, 0x10},
	{0x330d, 0x10},
	{0x331e, 0x49},
	{0x331f, 0x41},
	{0x3333, 0x10},
	{0x335d, 0x60},
	{0x335e, 0x06},
	{0x335f, 0x08},
	{0x3364, 0x56},
	{0x3366, 0x01},
	{0x337c, 0x02},
	{0x337d, 0x0a},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x08},
	{0x3394, 0x08},
	{0x3395, 0x08},
	{0x3396, 0x48},
	{0x3397, 0x4b},
	{0x3398, 0x4f},
	{0x3399, 0x0a},
	{0x339a, 0x0a},
	{0x339b, 0x10},
	{0x339c, 0x22},
	{0x33a2, 0x04},
	{0x33ad, 0x24},
	{0x33ae, 0x38},
	{0x33af, 0x38},
	{0x33b1, 0x80},
	{0x33b2, 0x48},
	{0x33b3, 0x20},
	{0x349f, 0x02},
	{0x34a6, 0x48},
	{0x34a7, 0x4b},
	{0x34a8, 0x20},
	{0x34a9, 0x18},
	{0x34f8, 0x5f},
	{0x34f9, 0x04},
	{0x3632, 0x48},
	{0x3633, 0x32},
	{0x3637, 0x29},
	{0x3638, 0xc1},
	{0x363b, 0x20},
	{0x363d, 0x02},
	{0x3670, 0x09},
	{0x3674, 0x88},
	{0x3675, 0x88},
	{0x3676, 0x88},
	{0x367c, 0x40},
	{0x367d, 0x48},
	{0x3690, 0x33},
	{0x3691, 0x34},
	{0x3692, 0x55},
	{0x3693, 0x4b},
	{0x3694, 0x4f},
	{0x3698, 0x85},
	{0x3699, 0x8f},
	{0x369a, 0xa0},
	{0x369b, 0xc3},
	{0x36a2, 0x49},
	{0x36a3, 0x4b},
	{0x36a4, 0x4f},
	{0x36d0, 0x01},
	{0x370f, 0x01},
	{0x3722, 0x00},
	{0x3728, 0x10},
	{0x37b0, 0x03},
	{0x37b1, 0x03},
	{0x37b2, 0x83},
	{0x37b3, 0x48},
	{0x37b4, 0x4f},
	{0x3901, 0x00},
	{0x3902, 0xc5},
	{0x3904, 0x08},
	{0x3905, 0x8d},
	{0x3909, 0x00},
	{0x391d, 0x04},
	{0x3926, 0x21},
	{0x3929, 0x18},
	{0x3933, 0x82},
	{0x3934, 0x08},
	{0x3937, 0x5b},
	{0x3939, 0x00},
	{0x393a, 0x01},
	{0x39dc, 0x02},
	{0x3c0f, 0x00},
	{0x3e00, 0x01},
	{0x3e01, 0x82},
	{0x3e02, 0x00},
	{0x3e04, 0x18},
	{0x3e05, 0x20},
	{0x3e23, 0x00},
	{0x3e24, 0xc8},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4816, 0x11},
	{0x5010, 0x10},
	{0x5799, 0x06},
	{0x57ad, 0x00},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x2a},
	{0x5ae4, 0x24},
	{0x5ae5, 0x30},
	{0x5ae6, 0x2a},
	{0x5ae7, 0x24},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x2a},
	{0x5af6, 0x24},
	{0x5af7, 0x30},
	{0x5af8, 0x2a},
	{0x5af9, 0x24},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x44},
	{0x37f9, 0x44},
	{0x0100, 0x01},
};

static struct regval_list sensor_2880x1620_20fps_shdr_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x1c},
	{0x320e, 0x13},//
	{0x320f, 0x56},//
	{0x3250, 0xff},
	{0x3251, 0x98},
	{0x3253, 0x0c},
	{0x325f, 0x20},
	{0x3281, 0x01},
	{0x3301, 0x08},
	{0x3304, 0x58},
	{0x3306, 0xa0},
	{0x3308, 0x14},
	{0x3309, 0x50},
	{0x330a, 0x01},
	{0x330b, 0x10},
	{0x330d, 0x10},
	{0x331e, 0x49},
	{0x331f, 0x41},
	{0x3333, 0x10},
	{0x335d, 0x60},
	{0x335e, 0x06},
	{0x335f, 0x08},
	{0x3364, 0x56},
	{0x3366, 0x01},
	{0x337c, 0x02},
	{0x337d, 0x0a},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x08},
	{0x3394, 0x08},
	{0x3395, 0x08},
	{0x3396, 0x48},
	{0x3397, 0x4b},
	{0x3398, 0x4f},
	{0x3399, 0x0a},
	{0x339a, 0x0a},
	{0x339b, 0x10},
	{0x339c, 0x22},
	{0x33a2, 0x04},
	{0x33ad, 0x24},
	{0x33ae, 0x38},
	{0x33af, 0x38},
	{0x33b1, 0x80},
	{0x33b2, 0x48},
	{0x33b3, 0x20},
	{0x349f, 0x02},
	{0x34a6, 0x48},
	{0x34a7, 0x4b},
	{0x34a8, 0x20},
	{0x34a9, 0x18},
	{0x34f8, 0x5f},
	{0x34f9, 0x04},
	{0x3632, 0x48},
	{0x3633, 0x32},
	{0x3637, 0x29},
	{0x3638, 0xc1},
	{0x363b, 0x20},
	{0x363d, 0x02},
	{0x3670, 0x09},
	{0x3674, 0x88},
	{0x3675, 0x88},
	{0x3676, 0x88},
	{0x367c, 0x40},
	{0x367d, 0x48},
	{0x3690, 0x33},
	{0x3691, 0x34},
	{0x3692, 0x55},
	{0x3693, 0x4b},
	{0x3694, 0x4f},
	{0x3698, 0x85},
	{0x3699, 0x8f},
	{0x369a, 0xa0},
	{0x369b, 0xc3},
	{0x36a2, 0x49},
	{0x36a3, 0x4b},
	{0x36a4, 0x4f},
	{0x36d0, 0x01},
	{0x370f, 0x01},
	{0x3722, 0x00},
	{0x3728, 0x10},
	{0x37b0, 0x03},
	{0x37b1, 0x03},
	{0x37b2, 0x83},
	{0x37b3, 0x48},
	{0x37b4, 0x4f},
	{0x3901, 0x00},
	{0x3902, 0xc5},
	{0x3904, 0x08},
	{0x3905, 0x8d},
	{0x3909, 0x00},
	{0x391d, 0x04},
	{0x3926, 0x21},
	{0x3929, 0x18},
	{0x3933, 0x82},
	{0x3934, 0x08},
	{0x3937, 0x5b},
	{0x3939, 0x00},
	{0x393a, 0x01},
	{0x39dc, 0x02},
	{0x3c0f, 0x00},
	{0x3e00, 0x01},
	{0x3e01, 0x82},
	{0x3e02, 0x00},
	{0x3e04, 0x18},
	{0x3e05, 0x20},
	{0x3e23, 0x00},
	{0x3e24, 0xc8},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4816, 0x11},
	{0x5010, 0x10},
	{0x5799, 0x06},
	{0x57ad, 0x00},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x2a},
	{0x5ae4, 0x24},
	{0x5ae5, 0x30},
	{0x5ae6, 0x2a},
	{0x5ae7, 0x24},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x2a},
	{0x5af6, 0x24},
	{0x5af7, 0x30},
	{0x5af8, 0x2a},
	{0x5af9, 0x24},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x44},
	{0x37f9, 0x44},
	{0x0100, 0x01},
};


static struct regval_list sensor_2880x1620_15fps_shdr_regs[] = {
	{0x0103, 0x01},
	{0x0100, 0x00},
	{0x36e9, 0x80},
	{0x37f9, 0x80},
	{0x301f, 0x1f},
	{0x320e, 0x0c},
	{0x320f, 0xe4},
	{0x3250, 0xff},
	{0x3251, 0x98},
	{0x3253, 0x0c},
	{0x325f, 0x20},
	{0x3281, 0x01},
	{0x3301, 0x08},
	{0x3304, 0x50},
	{0x3306, 0x78},
	{0x3308, 0x14},
	{0x3309, 0x70},
	{0x330a, 0x00},
	{0x330b, 0xd8},
	{0x330d, 0x10},
	{0x331e, 0x41},
	{0x331f, 0x61},
	{0x3333, 0x10},
	{0x335d, 0x60},
	{0x335e, 0x06},
	{0x335f, 0x08},
	{0x3364, 0x56},
	{0x3366, 0x01},
	{0x337c, 0x02},
	{0x337d, 0x0a},
	{0x3390, 0x01},
	{0x3391, 0x03},
	{0x3392, 0x07},
	{0x3393, 0x08},
	{0x3394, 0x08},
	{0x3395, 0x08},
	{0x3396, 0x40},
	{0x3397, 0x48},
	{0x3398, 0x4b},
	{0x3399, 0x08},
	{0x339a, 0x08},
	{0x339b, 0x08},
	{0x339c, 0x1d},
	{0x33a2, 0x04},
	{0x33ae, 0x30},
	{0x33af, 0x50},
	{0x33b1, 0x80},
	{0x33b2, 0x48},
	{0x33b3, 0x30},
	{0x349f, 0x02},
	{0x34a6, 0x48},
	{0x34a7, 0x4b},
	{0x34a8, 0x30},
	{0x34a9, 0x18},
	{0x34f8, 0x5f},
	{0x34f9, 0x08},
	{0x3632, 0x48},
	{0x3633, 0x32},
	{0x3637, 0x29},
	{0x3638, 0xc1},
	{0x363b, 0x20},
	{0x363d, 0x02},
	{0x3670, 0x09},
	{0x3674, 0x8b},
	{0x3675, 0xc6},
	{0x3676, 0x8b},
	{0x367c, 0x40},
	{0x367d, 0x48},
	{0x3690, 0x32},
	{0x3691, 0x43},
	{0x3692, 0x33},
	{0x3693, 0x40},
	{0x3694, 0x4b},
	{0x3698, 0x85},
	{0x3699, 0x8f},
	{0x369a, 0xa0},
	{0x369b, 0xc3},
	{0x36a2, 0x49},
	{0x36a3, 0x4b},
	{0x36a4, 0x4f},
	{0x36d0, 0x01},
	{0x36ec, 0x13},
	{0x370f, 0x01},
	{0x3722, 0x00},
	{0x3728, 0x10},
	{0x37b0, 0x03},
	{0x37b1, 0x03},
	{0x37b2, 0x83},
	{0x37b3, 0x48},
	{0x37b4, 0x49},
	{0x37fb, 0x25},
	{0x37fc, 0x01},
	{0x3901, 0x00},
	{0x3902, 0xc5},
	{0x3904, 0x08},
	{0x3905, 0x8c},
	{0x3909, 0x00},
	{0x391d, 0x04},
	{0x391f, 0x44},
	{0x3926, 0x21},
	{0x3929, 0x18},
	{0x3933, 0x82},
	{0x3934, 0x0a},
	{0x3937, 0x5f},
	{0x3939, 0x00},
	{0x393a, 0x00},
	{0x39dc, 0x02},
	{0x3c0f, 0x00},
	{0x3e00, 0x01},
	{0x3e01, 0x82},
	{0x3e02, 0x00},
	{0x3e04, 0x18},
	{0x3e05, 0x20},
	{0x3e23, 0x00},
	{0x3e24, 0xc8},
	{0x440e, 0x02},
	{0x4509, 0x20},
	{0x4800, 0x04},
	{0x4816, 0x11},
	{0x4837, 0x28},
	{0x5010, 0x10},
	{0x5799, 0x06},
	{0x57ad, 0x00},
	{0x5ae0, 0xfe},
	{0x5ae1, 0x40},
	{0x5ae2, 0x30},
	{0x5ae3, 0x2a},
	{0x5ae4, 0x24},
	{0x5ae5, 0x30},
	{0x5ae6, 0x2a},
	{0x5ae7, 0x24},
	{0x5ae8, 0x3c},
	{0x5ae9, 0x30},
	{0x5aea, 0x28},
	{0x5aeb, 0x3c},
	{0x5aec, 0x30},
	{0x5aed, 0x28},
	{0x5aee, 0xfe},
	{0x5aef, 0x40},
	{0x5af4, 0x30},
	{0x5af5, 0x2a},
	{0x5af6, 0x24},
	{0x5af7, 0x30},
	{0x5af8, 0x2a},
	{0x5af9, 0x24},
	{0x5afa, 0x3c},
	{0x5afb, 0x30},
	{0x5afc, 0x28},
	{0x5afd, 0x3c},
	{0x5afe, 0x30},
	{0x5aff, 0x28},
	{0x36e9, 0x44},
	{0x37f9, 0x34},
	{0x0100, 0x01},
};

static struct regval_list sensor_fmt_raw[] = {

};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	data_type explow, expmid, exphigh = 0;
	struct sensor_info *info = to_state(sd);

	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		if (exp_val < 16 * HDR_RATIO)
			exp_val = 16 * HDR_RATIO;

		exphigh = (unsigned char) (0x0f & (exp_val>>15));
		expmid = (unsigned char) (0xff & (exp_val>>7));
		explow = (unsigned char) (0xf0 & (exp_val<<1));

		sensor_write(sd, 0x3e02, explow);
		sensor_write(sd, 0x3e01, expmid);
		sensor_write(sd, 0x3e00, exphigh);

		sensor_dbg("sensor_set_long_exp = %d line Done!\n", exp_val);

		exp_val /= HDR_RATIO;

		expmid = (unsigned char) (0xff & (exp_val>>7));
		explow = (unsigned char) (0xf0 & (exp_val<<1));

		sensor_write(sd, 0x3e05, explow);
		sensor_write(sd, 0x3e04, expmid);

		sensor_dbg("sensor_set_short_exp = %d line Done!\n", exp_val);

	} else {
		if (exp_val < 16)
			exp_val = 16;

		exphigh = (unsigned char) (0xf & (exp_val>>15));
		expmid = (unsigned char) (0xff & (exp_val>>7));
		explow = (unsigned char) (0xf0 & (exp_val<<1));

		sensor_write(sd, 0x3e02, explow);
		sensor_write(sd, 0x3e01, expmid);
		sensor_write(sd, 0x3e00, exphigh);
		sensor_dbg("sensor_set_exp = %d line Done!\n", exp_val);
	}
	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);
	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	struct sensor_info *info = to_state(sd);
	data_type gaindiglow;
	data_type gaindighigh;
	data_type gainana;
	int gain = gain_val << 3;

	if (gain < 256) {			// 2.000 * 128 = 256
		gainana = 0x00;			// gian value:1.000
		gaindighigh = 0x00;
		gaindiglow = gain;
	} else if (gain < 326) {	// 2.550 * 128 = 326.4
		gainana = 0x01;			// gian value:2.000
		gaindighigh = 0x00;
		gaindiglow = gain  * 256 / 512;
	} else if (gain < 653) {	// 5.100 * 128 = 652.8
		gainana = 0x40;			//gian value:2.55
		gaindighigh = 0x00;
		gaindiglow = gain * 256 / 653;
	} else if (gain < 1306) {	// 10.20 * 128 = 1305.6
		gainana = 0x48;			//gian value:5.100
		gaindighigh = 0x00;
		gaindiglow = gain * 256 / 1306;
	} else if (gain < 2611) {   // 20.40 * 128 = 2611.2
		gainana = 0x49;			//gian value:10.20
		gaindighigh = 0x00;
		gaindiglow = gain * 256 / 2661;
	} else if (gain < 5222) {	// 40.80 * 128 = 5222.4
		gainana = 0x4B;			//gian value:20.40
		gaindighigh = 0x00;
		gaindiglow = gain * 256 / 5222;
	} else if (gain < 10445) {	// 81.60 * 1 * 128 = 10444.8
		gainana = 0x4F;			//gian value:40.80
		gaindighigh = 0x00;
		gaindiglow = gain * 256 / 10445;
	} else if (gain < 20890) {  // 81.60 * 2 * 128 = 20889.6
		gainana = 0x5F;			//gian value:81.60
		gaindighigh = 0x00;
		gaindiglow = gain * 256 / 20890;
	} else {
		gainana = 0x5F;			//gian value:81.60
		gaindighigh = 0x01;
		gaindiglow = gain * 256 / 20890 / 2;
	}


	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE) {
		sensor_write(sd, 0x3e13, (unsigned char)gainana);
		sensor_write(sd, 0x3e11, (unsigned char)gaindiglow);
		sensor_write(sd, 0x3e10, (unsigned char)gaindighigh);
	}

	sensor_write(sd, 0x3e09, (unsigned char)gainana);
	sensor_write(sd, 0x3e07, (unsigned char)gaindiglow);
	sensor_write(sd, 0x3e06, (unsigned char)gaindighigh);

	sensor_dbg("sensor_set_anagain = %d, 0x%x, 0x%x Done!\n", gain_val, gain);
	sensor_dbg("digital_gain = 0x%x, 0x%x Done!\n", gaindighigh, gaindiglow);
	info->gain = gain_val;

	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	struct sensor_info *info = to_state(sd);
	int exp_val, gain_val;

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;
	if (gain_val < 1 * 16)
		gain_val = 16;
	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	sensor_dbg("sensor_set_gain exp = %d, %d Done!\n", gain_val, exp_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;

}


static int sensor_s_fps(struct v4l2_subdev *sd,
			struct sensor_fps *fps)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_win_size *wsize = info->current_wins;

	sc530ai_sensor_vts = wsize->pclk/fps->fps/wsize->hts;
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == STBY_ON)
		ret = sensor_write(sd, 0x0100, rdval&0xfe);
	else
		ret = sensor_write(sd, 0x0100, rdval|0x01);
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(100, 120);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(5000, 7000);
		vin_set_mclk(sd, ON);
		usleep_range(5000, 7000);
		vin_set_mclk_freq(sd, MCLK);
		usleep_range(5000, 7000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_set_mclk(sd, OFF);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		vin_gpio_set_status(sd, POWER_EN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;

	sensor_read(sd, 0x3107, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR>>8))
		return -ENODEV;
	sensor_print("0x3107 = 0x%x\n", rdval);
	sensor_read(sd, 0x3108, &rdval);
	if (rdval != (V4L2_IDENT_SENSOR&0xff))
		return -ENODEV;
	sensor_print("0x3108 = 0x%x\n", rdval);

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 2880;
	info->height = 1620;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->exp = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		ret = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_SET_FPS:
		ret = sensor_s_fps(sd, (struct sensor_fps *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	{
		.width = 2880,
		.height = 1620,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3200,
		.vts = 1650,
		.pclk = 158400000,
		.mipi_bps = 396 * 1000 * 1000,
		.fps_fixed = 30,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max = (2 * 3300 - 8) << 4,
		.gain_min = 1 << 4,
		.gain_max = 326 << 4,
		.regs = sensor_2880x1620_30fps_regs,
		.regs_size = ARRAY_SIZE(sensor_2880x1620_30fps_regs),
		.set_size = NULL,
	},

	{
		.width = 2880,
		.height = 1620,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3200,
		.vts = 2475,
		.pclk = 158400000,
		.mipi_bps = 396 * 1000 * 1000,
		.fps_fixed = 20,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max = (2 * 3300 - 8) << 4,
		.gain_min = 1 << 4,
		.gain_max = 326 << 4,
		.regs = sensor_2880x1620_20fps_regs,
		.regs_size = ARRAY_SIZE(sensor_2880x1620_20fps_regs),
		.set_size = NULL,
	},

	/*vb:1650-1620 = 30,must set vb > 36 otherwise isp cannot set*/
	{
		.width = 2880,
		.height = 1620,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3200,
		.vts = 1650,
		.pclk = 316800000,
		.mipi_bps = 792 * 1000 * 1000,
		.fps_fixed = 60,
		.bin_factor = 1,
		.intg_min = 1 << 4,
		.intg_max = (2 * 3300 - 8) << 4,
		.gain_min = 1 << 4,
		.gain_max = 326 << 4,
		.regs = sensor_2880x1620_60fps_regs,
		.regs_size = ARRAY_SIZE(sensor_2880x1620_60fps_regs),
		.set_size = NULL,
	},

	{
		.width = 2880,
		.height = 1620,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3200,
		.vts = 3300,
		.pclk = 316800000,
		.mipi_bps = 792 * 1000 * 1000,
		.fps_fixed = 30,
		.bin_factor = 1,
		.if_mode = MIPI_VC_WDR_MODE,
		.wdr_mode = ISP_DOL_WDR_MODE,
		.intg_min = 1 << 4,
		.intg_max = (2 * 3300 - 8) << 4,
		.gain_min = 1 << 4,
		.gain_max = 326 << 4,
		.regs = sensor_2880x1620_30fps_shdr_regs,
		.regs_size = ARRAY_SIZE(sensor_2880x1620_30fps_shdr_regs),
		.set_size = NULL,
	},

	{
		.width = 2880,
		.height = 1620,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3200,
		.vts = 4950,
		.pclk = 316800000,
		.mipi_bps = 792 * 1000 * 1000,
		.fps_fixed = 20,
		.bin_factor = 1,
		.if_mode = MIPI_VC_WDR_MODE,
		.wdr_mode = ISP_DOL_WDR_MODE,
		.intg_min = 1 << 4,
		.intg_max = (2 * 4950 - 8) << 4,
		.gain_min = 1 << 4,
		.gain_max = 326 << 4,
		.regs = sensor_2880x1620_20fps_shdr_regs,
		.regs_size = ARRAY_SIZE(sensor_2880x1620_20fps_shdr_regs),
		.set_size = NULL,
	},

	{
		.width = 2880,
		.height = 1620,
		.hoffset = 0,
		.voffset = 0,
		.hts = 3200,
		.vts = 3300,
		.pclk = 158400000,
		.mipi_bps = 396 * 1000 * 1000,
		.fps_fixed = 15,
		.bin_factor = 1,
		.if_mode = MIPI_VC_WDR_MODE,
		.wdr_mode = ISP_DOL_WDR_MODE,
		.intg_min = 1 << 4,
		.intg_max = (2 * 3300 - 8) << 4,
		.gain_min = 1 << 4,
		.gain_max = 326 << 4,
		.regs = sensor_2880x1620_15fps_shdr_regs,
		.regs_size = ARRAY_SIZE(sensor_2880x1620_15fps_shdr_regs),
		.set_size = NULL,
	},
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_CSI2;
	if (info->isp_wdr_mode == ISP_DOL_WDR_MODE)
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1;
	else
		cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
				 ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_dbg("sensor_reg_init\n");

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->width = wsize->width;
	info->height = wsize->height;
	sc530ai_sensor_vts = wsize->vts;

	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
		     wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->current_wins->fps_fixed, info->fmt->mbus_code);

	if (!enable)
		return 0;

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_8,
	}
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 1,
			      65536 * 16, 1, 1);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_dev_id;

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	int i;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[i]);
	} else {
		cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv[sensor_dev_id++]);
	}

	sensor_init_controls(sd, &sensor_ctrl_ops);

	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET1 | MIPI_NORMAL_MODE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->time_hs = 0x15;//0x09
	info->exp = 0;
	info->gain = 0;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}
		sd = cci_dev_remove_helper(client, &cci_drv[i]);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		ret = cci_dev_init_helper(&sensor_driver[i]);

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

module_init(init_sensor);
module_exit(exit_sensor);