/***********************************************************************************************************************
 * Copyright [2015] Renesas Electronics Corporation and/or its licensors. All Rights Reserved.
 *
 * The contents of this file (the "contents") are proprietary and confidential to Renesas Electronics Corporation
 * and/or its licensors ("Renesas") and subject to statutory and contractual protections.
 *
 * Unless otherwise expressly agreed in writing between Renesas and you: 1) you may not use, copy, modify, distribute,
 * display, or perform the contents; 2) you may not use any name or mark of Renesas for advertising or publicity
 * purposes or in connection with your use of the contents; 3) RENESAS MAKES NO WARRANTY OR REPRESENTATIONS ABOUT THE
 * SUITABILITY OF THE CONTENTS FOR ANY PURPOSE; THE CONTENTS ARE PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY, INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT; AND 4) RENESAS SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, OR CONSEQUENTIAL DAMAGES,
 * INCLUDING DAMAGES RESULTING FROM LOSS OF USE, DATA, OR PROJECTS, WHETHER IN AN ACTION OF CONTRACT OR TORT, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THE CONTENTS. Third-party contents included in this file may
 * be subject to different terms.
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * File Name    : lcd_setup.c
 * Description  : Definition for the SK-S7 LCD Panel setup function through SPI interface
***********************************************************************************************************************/

#include "main_thread.h"
#include "lcd.h"

static void lcd_write(uint8_t cmd, char *data ,uint32_t len)
{
    ssp_err_t err;

    g_ioport_on_ioport.pinWrite(LCD_CMD,IOPORT_LEVEL_LOW);
    g_ioport_on_ioport.pinWrite(LCD_CS,IOPORT_LEVEL_LOW);

    err = g_spi_lcdc.p_api->write(g_spi_lcdc.p_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);
    if (SSP_SUCCESS != err)
    {
        while(1);
    }

    tx_semaphore_get(&g_main_semaphore_lcdc,TX_WAIT_FOREVER);

    if (len)
    {
        g_ioport_on_ioport.pinWrite(LCD_CMD,IOPORT_LEVEL_HIGH);

        err = g_spi_lcdc.p_api->write(g_spi_lcdc.p_ctrl, (void const *)data, len,SPI_BIT_WIDTH_8_BITS);
        if (SSP_SUCCESS != err)
        {
            while(1);
        }

        tx_semaphore_get(&g_main_semaphore_lcdc,TX_WAIT_FOREVER);
    }
    g_ioport_on_ioport.pinWrite(LCD_CS,IOPORT_LEVEL_HIGH);

}

static void lcd_read(uint8_t cmd, char *data ,uint32_t len)
{
    ssp_err_t err;
    static uint8_t dummy_write[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    g_ioport_on_ioport.pinWrite(LCD_CMD,IOPORT_LEVEL_LOW);
    g_ioport_on_ioport.pinWrite(LCD_CS,IOPORT_LEVEL_LOW);

    err = g_spi_lcdc.p_api->write(g_spi_lcdc.p_ctrl, &cmd, 1, SPI_BIT_WIDTH_8_BITS);
    if (SSP_SUCCESS != err)
    {
        while(1);
    }

    tx_semaphore_get(&g_main_semaphore_lcdc,TX_WAIT_FOREVER);

    g_ioport_on_ioport.pinWrite(LCD_CMD,IOPORT_LEVEL_HIGH);
    g_ioport_on_ioport.pinCfg(IOPORT_PORT_01_PIN_02,(IOPORT_CFG_PORT_DIRECTION_OUTPUT | ((g_spi_lcdc.p_cfg->clk_polarity == SPI_CLK_POLARITY_HIGH) ? IOPORT_LEVEL_LOW : IOPORT_LEVEL_HIGH)));
    R_BSP_SoftwareDelay(5,BSP_DELAY_UNITS_MICROSECONDS);
    g_ioport_on_ioport.pinCfg(IOPORT_PORT_01_PIN_02,(IOPORT_CFG_PERIPHERAL_PIN | IOPORT_PERIPHERAL_SCI0_2_4_6_8));

    err = g_spi_lcdc.p_api->writeRead(g_spi_lcdc.p_ctrl, dummy_write, (void const *)data,len,SPI_BIT_WIDTH_8_BITS);
    if (SSP_SUCCESS != err)
    {
        while(1);
    }

    tx_semaphore_get(&g_main_semaphore_lcdc,TX_WAIT_FOREVER);
    g_ioport_on_ioport.pinWrite(LCD_CS,IOPORT_LEVEL_HIGH);
}

void ILI9341V_Init(void)
{
    uint8_t data[8];
    int i;

    g_ioport_on_ioport.pinWrite(LCD_CS,IOPORT_LEVEL_HIGH);
    g_ioport_on_ioport.pinWrite(LCD_RESET,IOPORT_LEVEL_HIGH);
    g_ioport_on_ioport.pinWrite(LCD_RESET,IOPORT_LEVEL_LOW);
    tx_thread_sleep(1);
    g_ioport_on_ioport.pinWrite(LCD_RESET,IOPORT_LEVEL_HIGH);

    //120ms delay based on longest possible delay needed before sending
    //commands when the device is in sleep out mode.
    tx_thread_sleep(12);
    
    lcd_write(ILI9341_SW_RESET, "\x0", 0);
    
    //Ensure we wait at least 5ms before sending new commands after a software reset.
    tx_thread_sleep(5);
    
    for (i=0; i<4; i++)
    {
        data[0] = (uint8_t)(0x10+i);
        lcd_write(0xD9,(char *)data,1);
        lcd_read(0xD3,(char *)data, 1);
    }

    lcd_write(ILI9341_POWERB,       "\x00\xC1\x30", 3);
    lcd_write(ILI9341_DTCA,         "\x85\x00\x78", 3);
    lcd_write(ILI9341_DTCB,         "\x00\x00", 2);
    lcd_write(ILI9341_POWERA,       "\x39\x2C\x00\x34\x02", 5);
    lcd_write(ILI9341_POWER_SEQ,    "\x64\x03\x12\x81", 4);
    lcd_write(ILI9341_PRC,          "\x20", 1);
    lcd_write(ILI9341_POWER1,       "\x23", 1);
    lcd_write(ILI9341_POWER2,       "\x10", 1);
    lcd_write(ILI9341_VCOM1,        "\x3E\x28", 2);
    lcd_write(ILI9341_VCOM2,        "\x86", 1);
    lcd_write(ILI9341_MAC,          "\x48", 1);
    lcd_write(ILI9341_PIXEL_FORMAT, "\x55", 1);          /* DPI[101]: RGB 16 bits/pixel; DBI[101] MCU 16 bit/pixel */
    lcd_write(ILI9341_FRM_CTRL1,    "\x00\x18", 2);      /* 79 kHz */
    lcd_write(ILI9341_DFC,          "\x08\x82\x27", 3);
    lcd_write(ILI9341_3GAMMA_EN,    "\x00", 1);
    lcd_write(ILI9341_RGB_INTERFACE,"\xC2", 1);          /* ByPass_Mode =1; RCM[1:0] = 10; VSPL=0; HSPL = 0; DPL=1; EPL=0 */
    lcd_write(ILI9341_INTERFACE,    "\x01\x00\x06", 3);  /* DE pol - high active, DOTCLK Pol = 1 data latch on falling, HSYNC Pol - High level Sync, Vsync Low level sync */
    //lcd_write(ILI9341_BPC,        "\x02\0x2\x0A\x14", 4); /* Vert FP = 2 BP = 3; Horiz FP= 10, PB = 20 */
    lcd_write(ILI9341_COLUMN_ADDR,  "\x00\x00\x00\xEF", 4);
    lcd_write(ILI9341_PAGE_ADDR,    "\x00\x00\x01\x3F", 4);
    lcd_write(ILI9341_GAMMA,        "\x01", 1);
    lcd_write(ILI9341_PGAMMA,       "\x0F\x31\x2B\x0C\x0E\x08\x4E\xF1\x37\x07\x10\x03\x0E\x09\x00", 15);
    lcd_write(ILI9341_NGAMMA,       "\x00\x0E\x14\x03\x11\x07\x31\xC1\x48\x08\x0F\x0C\x31\x36\x0F", 15);
    lcd_write(ILI9341_SLEEP_OUT,    "\x00", 1);
    tx_thread_sleep(2);
    lcd_write(ILI9341_DISP_ON,      "\x00", 1);
    lcd_read(ILI9341_READ_DISP_PIXEL,(char *)data,1);
}
