#pragma GCC optimize ("-Ofast")

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_conf.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico.h"
#include "hardware/vreg.h"
#include <Arduino.h>
#include "hardware/watchdog.h"
#include "hardware/adc.h"

#define ADC_CLOCK_OUT 13

#define SW1 24
#define SW2 25
#define SW4 22
#define SW8 23

#define ADC_OFFSET 0.8361618f

#define TIME_BASE_MIN 0
#define TIME_BASE_MAX 100

#define K 8
#define N 256

#define RCA 1

bool timerCallback(struct repeating_timer *t)
{
    lv_tick_inc(1);
    return true;
}

static lv_obj_t *chart;

struct repeating_timer timer0;
struct repeating_timer timer1;
int16_t adc[N];
int16_t fftVaule[N / 2];

typedef struct
{
    float r;
    float i;
} complex;

static complex w[N / 2];
static complex dat[N];

static void butter_fly(complex *a, complex *b, const complex *c)
{
    complex bc;
    bc.r = b->r * c->r - b->i * c->i;
    bc.i = b->r * c->i + b->i * c->r;
    b->r = a->r - bc.r;
    b->i = a->i - bc.i;
    a->r += bc.r;
    a->i += bc.i;
}

static uint32_t bits_reverse(uint32_t index, uint32_t bits)
{
    uint32_t left, right;
    left = index << 16;
    right = index >> 16;
    index = left | right;
    left = (index << 8) & 0xff00ff00;
    right = (index >> 8) & 0x00ff00ff;
    index = left | right;
    left = (index << 4) & 0xf0f0f0f0;
    right = (index >> 4) & 0x0f0f0f0f;
    index = left | right;
    left = (index << 2) & 0xc3c3c3c3;
    right = (index >> 2) & 0x3c3c3c3c;
    index = left | right;
    left = (index << 1) & 0xa5a5a5a5;
    right = (index >> 1) & 0x5a5a5a5a;
    index = left | right;
    return index >> (32 - bits);
}

static void fft_k(complex *dat, const complex *w, uint32_t k, uint32_t k_all)
{
    uint32_t i, j;
    complex *dat1;
    k_all = 1 << (k_all - k - 1);
    k = 1 << k;
    dat1 = dat + k;
    for (i = 0; i < k_all; i++)
    {
        for (j = 0; j < k; j++)
        {
            butter_fly(dat++, dat1++, w + j * k_all);
        }
        dat += k;
        dat1 += k;
    }
}

void fft_init(complex *w, uint32_t k)
{
    float beta = 0.0f, dbeta = 3.1415926535f / k;
    for (; k; k--)
    {
        w->r = cosf(beta);
        w->i = sinf(beta);
        beta += dbeta;
        w++;
    }
}

void fft(complex *dat, const complex *w, uint32_t k)
{
    uint32_t i, j, n;
    complex temp;
    n = 1 << k;
    for (i = 0; i < n; i++)
    {
        j = bits_reverse(i, k);
        if (i <= j)
        {
            continue;
        }
        temp = dat[i];
        dat[i] = dat[j];
        dat[j] = temp;
    }
    for (i = 0; i < k; i++)
    {
        fft_k(dat, w, i, k);
    }
}


void initADC()
{
    gpio_init(1);
    gpio_init(2);
    gpio_init(3);
    gpio_init(4);
    gpio_init(5);
    gpio_init(6);
    gpio_init(7);
    gpio_init(8);
    gpio_init(9);
    gpio_init(10);
    gpio_init(11);
    gpio_init(12);
    gpio_init(13);
    gpio_set_dir(1, GPIO_IN);
    gpio_set_dir(2, GPIO_IN);
    gpio_set_dir(3, GPIO_IN);
    gpio_set_dir(4, GPIO_IN);
    gpio_set_dir(5, GPIO_IN);
    gpio_set_dir(6, GPIO_IN);
    gpio_set_dir(7, GPIO_IN);
    gpio_set_dir(8, GPIO_IN);
    gpio_set_dir(9, GPIO_IN);
    gpio_set_dir(10, GPIO_IN);
    gpio_set_dir(11, GPIO_IN);
    gpio_set_dir(12, GPIO_IN);
    gpio_set_dir(13, GPIO_OUT);
}

void initSwitch()
{
    gpio_init(SW1);
    gpio_init(SW2);
    gpio_init(SW4);
    gpio_init(SW8);

    gpio_pull_up(SW1);
    gpio_pull_up(SW2);
    gpio_pull_up(SW4);
    gpio_pull_up(SW8);

    gpio_set_dir(SW1, GPIO_IN);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_set_dir(SW4, GPIO_IN);
    gpio_set_dir(SW8, GPIO_IN);
}

#define PICO_FLASH_SPI_CLKDIV 2
lv_obj_t *screen_label_1;
lv_obj_t *screen_label_2;
lv_obj_t *screen_label_3;
lv_obj_t *screen_label_4;
lv_obj_t *screen_label_5;
lv_obj_t *screen_label_6;
lv_obj_t *screen_label_7;
lv_obj_t *screen_label_8;
lv_obj_t *screen_label_9;

uint16_t timeData = 500;
int test_data, test_old_data, adc_time_base;

_Bool adcFlag = false;
static float oldData = 0.0f;
static int avg = 0;

void chan_adc_time_base()
{
    int offset;

    offset = test_old_data - test_data;
    if (abs(offset) <= 2)
    {
        adc_time_base -= offset;
    }
}

bool getADC(struct repeating_timer *t)
{
    watchdog_update();
    // set_sys_clock_khz(252 * 1000, true);

    if (adc_time_base == 0)
    {
        for (uint16_t i = 0; i < N; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            adc[i] = sio_hw->gpio_in;
        }
    }
    else if (adc_time_base == 1)
    {
        for (uint16_t i = 0; i < N; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            asm volatile("nop \n nop");
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            asm volatile("nop ");
            adc[i] = sio_hw->gpio_in;
            asm volatile("nop ");
        }
    }
    else if (adc_time_base == 2)
    {
        for (uint16_t i = 0; i < N; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            asm volatile("nop ");
            asm volatile("nop \n nop");
            adc[i] = sio_hw->gpio_in;
            asm volatile("nop ");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
        }
    }
    else if (adc_time_base == 3)
    {
        for (uint16_t i = 0; i < N; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            asm volatile("nop ");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            adc[i] = sio_hw->gpio_in;
            asm volatile("nop ");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
            asm volatile("nop \n nop");
        }
    }
    else if (adc_time_base == -1)
    {
        for (uint16_t i = 0; i < N / 2; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            adc[i] = sio_hw->gpio_in;
        }
    }
    else if (adc_time_base == -2)
    {
        for (uint16_t i = 0; i < N / 4; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            adc[i] = sio_hw->gpio_in;
        }
    }
        else if (adc_time_base == -3)
    {
        for (uint16_t i = 0; i < N / 8; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            adc[i] = sio_hw->gpio_in;
        }
    }
        else if (adc_time_base == -4)
    {
        for (uint16_t i = 0; i < N / 16; i++)
        {
            gpio_set_mask(0x01 << ADC_CLOCK_OUT);
            gpio_clr_mask(0x01 << ADC_CLOCK_OUT);
            adc[i] = sio_hw->gpio_in;
        }
    }
    for (uint16_t i = 0; i < N; i++)
    {
        adc[i] = 4095 - adc[i];
        uint32_t tmp = adc[i];

        uint16_t readValue = !!((1ul << 1) & tmp) << 11;
        readValue += !!((1ul << 2) & tmp) << 10;
        readValue += !!((1ul << 3) & tmp) << 9;
        readValue += !!((1ul << 4) & tmp) << 8;
        readValue += !!((1ul << 5) & tmp) << 7;
        readValue += !!((1ul << 6) & tmp) << 6;
        readValue += !!((1ul << 7) & tmp) << 5;
        readValue += !!((1ul << 8) & tmp) << 4;
        readValue += !!((1ul << 9) & tmp) << 3;
        readValue += !!((1ul << 10) & tmp) << 2;
        readValue += !!((1ul << 11) & tmp) << 1;
        readValue += !!((1ul << 12) & tmp);
        dat[i].r = RCA * readValue + (1.0f - RCA) * oldData;
        oldData = RCA * readValue + (1.0f - RCA) * oldData;
        adc[i] = oldData;
        dat[i].i = 0.0f;
        avg += oldData;
    }
    // fft_init((complex *)w, N / 2);
    // fft((complex *)dat, (const complex *)w, K);

    test_data = !((1ul << SW8) & sio_hw->gpio_in) << 3;
    test_data += !((1ul << SW4) & sio_hw->gpio_in) << 2;
    test_data += !((1ul << SW2) & sio_hw->gpio_in) << 1;
    test_data += !((1ul << SW1) & sio_hw->gpio_in);

    if (test_data == 0)
    {
        if ((test_old_data == 1) | (test_old_data == 9))
        {
            chan_adc_time_base();
            test_old_data = 0;
        }
    }
    else
    {
        chan_adc_time_base();
        test_old_data = test_data;
    }

    adcFlag = true;

    return true;
}

lv_chart_series_t *ser;

void simple_update_array()
{
    lv_chart_set_point_count(chart, N);
    lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
}

int main()
{
    watchdog_enable(200, 1);
    int max = 0;
    float max_value = 0;

    vreg_set_voltage(VREG_VOLTAGE_1_20); // 300MHz需要加到1.35v
    sleep_ms(10);
    watchdog_update();
    set_sys_clock_khz(266 * 1000, true);
    watchdog_update();

    initADC();
    watchdog_update();

    adc_init();
    adc_gpio_init(27);
    adc_select_input(1);
    watchdog_update();

    initSwitch();
    watchdog_update();

    test_data = !((1ul << SW8) & sio_hw->gpio_in) << 3;
    test_data += !((1ul << SW4) & sio_hw->gpio_in) << 2;
    test_data += !((1ul << SW2) & sio_hw->gpio_in) << 1;
    test_data += !((1ul << SW1) & sio_hw->gpio_in);
    watchdog_update();

    stdio_init_all();
    watchdog_update();

    add_repeating_timer_ms(1, timerCallback, NULL, &timer0);

    add_repeating_timer_ms(20, getADC, NULL, &timer1);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_disp_set_rotation(NULL, LV_DISP_ROT_90);
    /*

    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 240, 200);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 10);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, (lv_coord_t)(0), (lv_coord_t)(4096));
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_series_t *ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_PRIMARY_Y);
    //lv_chart_series_t *ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    uint32_t pcnt = sizeof(fftVaule) / sizeof(fftVaule[0]);
    //uint32_t pcnt1 = sizeof(adc) / sizeof(adc[0]);
    lv_chart_set_point_count(chart, pcnt);
    //lv_chart_set_point_count(chart, pcnt1);
    lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
    //lv_chart_set_ext_y_array(chart, ser1, (lv_coord_t *)fftVaule);

    lv_obj_t *label1 = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label1, "Vmax:%dV Vmin:0.0V", N);
    lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 15);

    lv_obj_t *label2 = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label2, true);
    lv_label_set_text_fmt(label2, "#ff0000 Freq: %d Khz# #00ff00 Vavg: %d V#", 6 * max, avg);
    lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label2, LV_ALIGN_TOP_MID, 0, 0);

    */

    lv_obj_t *screen = lv_scr_act();

/*
	lv_obj_t *screen_bar_1 = lv_bar_create(screen);

	//Write style LV_BAR_PART_BG for screen_bar_1
	static lv_style_t style_screen_bar_1_bg;
	lv_style_reset(&style_screen_bar_1_bg);

	//Write style state: LV_STATE_DEFAULT for style_screen_bar_1_bg
	lv_style_set_radius(&style_screen_bar_1_bg, 10);
	lv_style_set_bg_color(&style_screen_bar_1_bg, lv_color_make(0xd4, 0xd7, 0xd9));
	lv_style_set_bg_grad_color(&style_screen_bar_1_bg, lv_color_make(0xd4, 0xd7, 0xd9));
	lv_style_set_bg_grad_dir(&style_screen_bar_1_bg,  LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_bar_1_bg, 255);
	lv_style_set_pad_left(&style_screen_bar_1_bg, 0);
	lv_style_set_pad_right(&style_screen_bar_1_bg, 0);
	lv_style_set_pad_top(&style_screen_bar_1_bg, 0);
	lv_style_set_pad_bottom(&style_screen_bar_1_bg, 0);
	lv_obj_add_style(screen_bar_1, &style_screen_bar_1_bg,LV_PART_MAIN | LV_STATE_DEFAULT);

	//Write style LV_BAR_PART_INDIC for screen_bar_1
	static lv_style_t style_screen_bar_1_indic;
	lv_style_reset(&style_screen_bar_1_indic);

	//Write style state: LV_STATE_DEFAULT for style_screen_bar_1_indic
	lv_style_set_radius(&style_screen_bar_1_indic, 10);
	lv_style_set_bg_color(&style_screen_bar_1_indic, lv_color_make(0x01, 0xa2, 0xb1));
	lv_style_set_bg_grad_color(&style_screen_bar_1_indic, lv_color_make(0x01, 0xa2, 0xb1));
	lv_style_set_bg_grad_dir(&style_screen_bar_1_indic, LV_GRAD_DIR_VER);
	lv_style_set_bg_opa(&style_screen_bar_1_indic, 255);
	lv_obj_add_style(screen_bar_1, &style_screen_bar_1_indic ,LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_pos(screen_bar_1, 150, 160);
	lv_obj_set_size(screen_bar_1, 269, 20);

	//Write animation: screen_bar_1move in x direction
	lv_anim_t screen_bar_1_x;
	lv_anim_init(&screen_bar_1_x);
	lv_anim_set_var(&screen_bar_1_x, screen_bar_1);
	lv_anim_set_time(&screen_bar_1_x, 100);
	lv_anim_set_exec_cb(&screen_bar_1_x, (lv_anim_exec_xcb_t)lv_obj_set_x);
	lv_anim_set_values(&screen_bar_1_x, lv_obj_get_x(screen_bar_1), 0);
	lv_anim_start(&screen_bar_1_x);

	//Write animation: screen_bar_1move in y direction
	lv_anim_t screen_bar_1_y;
	lv_anim_init(&screen_bar_1_y);
	lv_anim_set_var(&screen_bar_1_y, screen_bar_1);
	lv_anim_set_time(&screen_bar_1_y, 100);
	lv_anim_set_exec_cb(&screen_bar_1_y, (lv_anim_exec_xcb_t)lv_obj_set_y);
	lv_anim_set_values(&screen_bar_1_y, lv_obj_get_y(screen_bar_1), 0);
	lv_anim_start(&screen_bar_1_y);

*/

    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_pos(chart, 0, 0);
    lv_obj_set_size(chart, 215, 145);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 4095);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_set_div_line_count(chart, 4, 9);
    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_ORANGE), LV_CHART_AXIS_PRIMARY_Y);

    static lv_style_t style_screen_chart_1_main_main_default;
    lv_style_set_bg_color(&style_screen_chart_1_main_main_default, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_bg_grad_color(&style_screen_chart_1_main_main_default, lv_color_make(0x00, 0x00, 0x00));
    lv_style_set_bg_grad_dir(&style_screen_chart_1_main_main_default, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_screen_chart_1_main_main_default, 255);
    lv_style_set_pad_left(&style_screen_chart_1_main_main_default, 5);
    lv_style_set_pad_right(&style_screen_chart_1_main_main_default, 5);
    lv_style_set_pad_top(&style_screen_chart_1_main_main_default, 5);
    lv_style_set_pad_bottom(&style_screen_chart_1_main_main_default, 5);
    lv_style_set_line_color(&style_screen_chart_1_main_main_default, lv_color_make(0x68, 0x68, 0x68));
    lv_style_set_line_width(&style_screen_chart_1_main_main_default, 1);
    lv_style_set_line_opa(&style_screen_chart_1_main_main_default, 100);
    lv_obj_add_style(chart, &style_screen_chart_1_main_main_default, LV_PART_MAIN | LV_STATE_DEFAULT);

    // lv_chart_series_t *ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
    // uint32_t pcnt1 = sizeof(adc) / sizeof(adc[0]);
    lv_chart_set_point_count(chart, N);
    // lv_chart_set_point_count(chart, pcnt1);
    lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
    // lv_chart_set_ext_y_array(chart, ser1, (lv_coord_t *)fftVaule);

    // Write codes screen_label_1
    screen_label_1 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_1, 232, 5);
    lv_obj_set_size(screen_label_1, 80, 15);
    lv_label_set_text_fmt(screen_label_1, "Vp 9999mv");
    lv_label_set_long_mode(screen_label_1, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_1, LV_TEXT_ALIGN_CENTER, 0);

    // Write codes screen_label_2
    screen_label_2 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_2, 232, 30);
    lv_obj_set_size(screen_label_2, 80, 15);
    lv_label_set_text_fmt(screen_label_2, "Vn 9999mv");
    lv_label_set_long_mode(screen_label_2, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_2, LV_TEXT_ALIGN_CENTER, 0);

    // Write codes screen_label_3
    screen_label_3 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_3, 232, 55);
    lv_obj_set_size(screen_label_3, 80, 15);
    lv_label_set_text_fmt(screen_label_3, "Va 9999mv");
    lv_label_set_long_mode(screen_label_3, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_3, LV_TEXT_ALIGN_CENTER, 0);

    // Write codes screen_label_4
    screen_label_4 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_4, 232, 80);
    lv_obj_set_size(screen_label_4, 80, 15);
    lv_label_set_text_fmt(screen_label_4, "Freq:0.0Mhz");
    lv_label_set_long_mode(screen_label_4, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_4, LV_TEXT_ALIGN_CENTER, 0);

    // Write codes screen_label_5
    screen_label_5 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_5, 3, 154);
    lv_obj_set_size(screen_label_5, 80, 15);
    lv_label_set_long_mode(screen_label_5, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_5, LV_TEXT_ALIGN_CENTER, 0);

    // Write codes screen_label_6
    screen_label_6 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_6, 121, 153);
    lv_obj_set_size(screen_label_6, 80, 15);
    lv_label_set_text(screen_label_6, "waitting...");
    lv_label_set_long_mode(screen_label_6, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_6, LV_TEXT_ALIGN_CENTER, 0);

    // Write codes screen_label_7
    screen_label_7 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_7, 234, 105);
    lv_obj_set_size(screen_label_7, 80, 15);
    lv_label_set_long_mode(screen_label_7, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_7, LV_TEXT_ALIGN_CENTER, 0);

    screen_label_8 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_8, 234, 153);
    lv_obj_set_size(screen_label_8, 80, 15);
    lv_label_set_long_mode(screen_label_8, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_8, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(screen_label_8, "SoftVer 0.2");

    screen_label_9 = lv_label_create(screen);
    lv_obj_set_pos(screen_label_9, 234, 130);
    lv_obj_set_size(screen_label_9, 80, 15);
    lv_label_set_long_mode(screen_label_9, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(screen_label_9, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(screen_label_9, "BAT:%d", adc_read() * 6600 / (1 << 12));

    adc_time_base = 0;

    while (1)
    {
        lv_task_handler();

        if (adcFlag)
        {
            for (uint16_t i = 0; i < N / 2; i++)
            {
                fftVaule[i] = dat[i].r;
                if (fftVaule[i] > max_value)
                {
                    max_value = fftVaule[i];
                    max = i;
                }
            }

            int adc_max_value = 0, adc_min_value = 4096;
            int adc_avg_value = 0;

            if (adc_time_base == -1)
            {
                for (uint16_t i = 0; i < N / 2; i++)
                {
                    if (adc[i] > adc_max_value)
                    {
                        adc_max_value = adc[i];
                    }
                    if (adc[i] < adc_min_value)
                    {
                        adc_min_value = adc[i];
                    }
                    adc_avg_value += adc[i];
                }
                adc_avg_value = adc_avg_value / N / 2;
            }
            else
            {

                for (uint16_t i = 0; i < N; i++)
                {
                    if (adc[i] > adc_max_value)
                    {
                        adc_max_value = adc[i];
                    }
                    if (adc[i] < adc_min_value)
                    {
                        adc_min_value = adc[i];
                    }
                    adc_avg_value += adc[i];
                }
                adc_avg_value = adc_avg_value / N;
            }

            lv_label_set_text_fmt(screen_label_1, "Vp %dmv", (int)(adc_max_value * ADC_OFFSET));
            lv_label_set_text_fmt(screen_label_2, "Vn %dmv", (int)(adc_min_value * ADC_OFFSET));
            lv_label_set_text_fmt(screen_label_3, "Va %dmv", (int)(adc_avg_value * ADC_OFFSET));

            // lv_chart_set_ext_y_array(chart, ser1, (lv_coord_t *)fftVaule);
            lv_label_set_text_fmt(screen_label_7, "%d , %d ", test_old_data, adc_time_base);
            lv_label_set_text_fmt(screen_label_4, "Freq:%d", max);

            if (adc_time_base == 0)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 500ns");
                simple_update_array();
            }

            else if (adc_time_base == 1)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 1 us");
                simple_update_array();
            }
            else if (adc_time_base == 2)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 2 us");
                simple_update_array();
            }
            else if (adc_time_base == 3)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 4 us");
                simple_update_array();
            }
            else if (adc_time_base == -1)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 250ns");
                lv_chart_set_point_count(chart, N / 2);
                lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
            }
            else if (adc_time_base == -1)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 250ns");
                lv_chart_set_point_count(chart, N / 2);
                lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
            }
            else if (adc_time_base == -2)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 125ns");
                lv_chart_set_point_count(chart, N / 4);
                lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
            }
                        else if (adc_time_base == -3)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 62.5ns");
                lv_chart_set_point_count(chart, N / 8);
                lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
            }
                        else if (adc_time_base == -4)
            {
                lv_label_set_text_fmt(screen_label_5, "Div: 31.25ns");
                lv_chart_set_point_count(chart, N / 16);
                lv_chart_set_ext_y_array(chart, ser, (lv_coord_t *)adc);
            }
            avg = 0;
            max = 0;
            max_value = 0;
            adcFlag = false;
            lv_label_set_text_fmt(screen_label_9, "B:%d mv", adc_read() * 6600 / (1 << 12));
        }
    }
}
