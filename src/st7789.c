/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/pwm.h"

#include "st7789.h"

#define offset_x 0
#define offset_y 34

static struct st7789_config st7789_cfg;
static uint16_t st7789_width;
static uint16_t st7789_height;
static bool st7789_data_mode = false;

static void st7789_cmd(uint8_t cmd, const uint8_t *data, size_t len)
{
    if (st7789_cfg.gpio_cs > -1)
    {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    }
    else
    {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }
    st7789_data_mode = false;

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);

    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));

    if (len)
    {
        sleep_us(1);
        gpio_put(st7789_cfg.gpio_dc, 1);
        sleep_us(1);

        spi_write_blocking(st7789_cfg.spi, data, len);
    }

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

void st7789_caset(uint16_t xs, uint16_t xe)
{
    uint8_t data[] = {
        xs >> 8,
        xs & 0xff,
        xe >> 8,
        xe & 0xff,
    };

    // CASET (2Ah): Column Address Set
    st7789_cmd(0x2a, data, sizeof(data));
}

void st7789_raset(uint16_t ys, uint16_t ye)
{
    uint8_t data[] = {
        ys >> 8,
        ys & 0xff,
        ye >> 8,
        ye & 0xff,
    };

    // RASET (2Bh): Row Address Set
    st7789_cmd(0x2b, data, sizeof(data));
}

void st7789_init(const struct st7789_config *config, uint16_t width, uint16_t height)
{
    memcpy(&st7789_cfg, config, sizeof(st7789_cfg));
    st7789_width = width;
    st7789_height = height;

    spi_init(st7789_cfg.spi, 20 * 1000 * 1000);
    if (st7789_cfg.gpio_cs > -1)
    {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    }
    else
    {
        spi_set_format(st7789_cfg.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }

    gpio_set_function(st7789_cfg.gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(st7789_cfg.gpio_clk, GPIO_FUNC_SPI);

    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_init(st7789_cfg.gpio_cs);
    }
    gpio_init(st7789_cfg.gpio_dc);
    gpio_init(st7789_cfg.gpio_rst);

    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_set_dir(st7789_cfg.gpio_cs, GPIO_OUT);
    }
    gpio_set_dir(st7789_cfg.gpio_dc, GPIO_OUT);
    gpio_set_dir(st7789_cfg.gpio_rst, GPIO_OUT);

    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_put(st7789_cfg.gpio_cs, 1);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    gpio_put(st7789_cfg.gpio_rst, 1);
    sleep_ms(100);

    // SWRESET (01h): Software Reset
    st7789_cmd(0x01, NULL, 0);
    sleep_ms(150);

    // SLPOUT (11h): Sleep Out
    st7789_cmd(0x11, NULL, 0);
    sleep_ms(50);

    st7789_cmd(0xB1, (uint8_t[]){0x05, 0x3A, 0x3A}, 3);
    st7789_cmd(0xB2, (uint8_t[]){0x05, 0x3A, 0x3A}, 3);
    st7789_cmd(0xB3, (uint8_t[]){0x05, 0x3A, 0x3A, 0x05, 0x3A, 0x3A}, 6);
    st7789_cmd(0xB4, (uint8_t[]){0x03}, 1);
    st7789_cmd(0xC1, (uint8_t[]){0xC0}, 1);
    st7789_cmd(0xC2, (uint8_t[]){0x0D, 0x00}, 2);
    st7789_cmd(0xC3, (uint8_t[]){0x8D, 0x6A}, 2);
    st7789_cmd(0xC4, (uint8_t[]){0x8D, 0xEE}, 2);
    st7789_cmd(0xC5, (uint8_t[]){0x0E}, 1);
    st7789_cmd(0xE0, (uint8_t[]){0x10, 0x0E, 0x02, 0x03, 0x0E, 0x07, 0x02, 0x07, 0x0A, 0x12, 0x27, 0x37, 0x00, 0x0D, 0x0E, 0x10}, 16);
    st7789_cmd(0xE1, (uint8_t[]){0x10, 0x0E, 0x02, 0x03, 0x0E, 0x07, 0x02, 0x07, 0x0A, 0x12, 0x27, 0x37, 0x00, 0x0D, 0x0E, 0x10}, 16);

    // COLMOD (3Ah): Interface Pixel Format
    // - RGB interface color format     = 65K of RGB interface
    // - Control interface color format = 16bit/pixel
    st7789_cmd(0x3a, (uint8_t[]){0x55}, 1);
    sleep_ms(10);

    // MADCTL (36h): Memory Data Access Control
    // - Page Address Order            = Top to Bottom
    // - Column Address Order          = Left to Right
    // - Page/Column Order             = Normal Mode
    // - Line Address Order            = LCD Refresh Top to Bottom
    // - RGB/BGR Order                 = RGB
    // - Display Data Latch Data Order = LCD Refresh Left to Right
    st7789_cmd(0x36, (uint8_t[]){0x60}, 1);

    st7789_caset(0 + offset_x, width + offset_x);
    st7789_raset(0 + offset_y, height + offset_y);

    // INVON (21h): Display Inversion On
    st7789_cmd(0x21, NULL, 0);
    sleep_ms(10);

    // NORON (13h): Normal Display Mode On
    st7789_cmd(0x13, NULL, 0);
    sleep_ms(10);

    // DISPON (29h): Display On
    st7789_cmd(0x29, NULL, 0);
    sleep_ms(10);

    gpio_init(st7789_cfg.gpio_bl);
    gpio_set_dir(st7789_cfg.gpio_bl, GPIO_OUT);
    gpio_set_mask(0x01 << st7789_cfg.gpio_bl);

    st7789_fill(0x0000);
    //gpio_set_function(st7789_cfg.gpio_bl, GPIO_FUNC_PWM);

    //uint slice_num = pwm_gpio_to_slice_num(st7789_cfg.gpio_bl);

    //pwm_config cfg = pwm_get_default_config();

    //pwm_config_set_clkdiv(&cfg, 4.f);

    //pwm_init(slice_num, &cfg, true);

    // Set period of 4 cycles (0 to 3 inclusive)
    //pwm_set_wrap(slice_num, 4);
    // Set channel A output high for one cycle before dropping
    //pwm_set_chan_level(slice_num, PWM_CHAN_A, 2);
    // Set initial B output high for three cycles before dropping
    //pwm_set_chan_level(slice_num, PWM_CHAN_B, 2);
    // Set the PWM running
    //pwm_set_enabled(slice_num, true);
}

void st7789_ramwr()
{
    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 0);
    sleep_us(1);

    // RAMWR (2Ch): Memory Write
    uint8_t cmd = 0x2c;
    spi_write_blocking(st7789_cfg.spi, &cmd, sizeof(cmd));

    sleep_us(1);
    if (st7789_cfg.gpio_cs > -1)
    {
        gpio_put(st7789_cfg.gpio_cs, 0);
    }
    gpio_put(st7789_cfg.gpio_dc, 1);
    sleep_us(1);
}

void st7789_write(const void *data, size_t len)
{
    if (!st7789_data_mode)
    {
        st7789_ramwr();

        if (st7789_cfg.gpio_cs > -1)
        {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        }
        else
        {
            spi_set_format(st7789_cfg.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }

        st7789_data_mode = true;
    }

    spi_write16_blocking(st7789_cfg.spi, data, len / 2);
}

void st7789_put(uint16_t pixel)
{
    st7789_write(&pixel, sizeof(pixel));
}

void st7789_fill(uint16_t pixel)
{
    int num_pixels = st7789_width * st7789_height;

    st7789_set_cursor(0, 0);

    for (int i = 0; i < num_pixels; i++)
    {
        st7789_put(pixel);
    }
}

void st7789_set_cursor(uint16_t x, uint16_t y)
{
    st7789_caset(offset_x + x, offset_x + st7789_width);
    st7789_raset(offset_y + y, offset_y + st7789_height);
}

void st7789_set_windows(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd)
{
    st7789_caset(offset_x + xStart, offset_x + xEnd);
    st7789_raset(offset_y + yStart, offset_y + yEnd);
}

void st7789_vertical_scroll(uint16_t row)
{
    uint8_t data[] = {
        (row >> 8) & 0xff,
        row & 0x00ff};

    // VSCSAD (37h): Vertical Scroll Start Address of RAM
    st7789_cmd(0x37, data, sizeof(data));
}
