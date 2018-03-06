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
 * File Name    : lcd.h
 * Description  : Header file for SK-S7 LCD Panel Setup
***********************************************************************************************************************/

#ifndef LCD_H_
#define LCD_H_

#define LCD_RESET IOPORT_PORT_06_PIN_10
#define LCD_CMD	  IOPORT_PORT_01_PIN_15
#define LCD_CS	  IOPORT_PORT_06_PIN_11

/* ILI9341 command set */
#define ILI9341_SW_RESET            0x01
#define ILI9341_READ_DISP_ID        0x04
#define ILI9341_READ_DISP_ST        0x09
#define ILI9341_READ_DISP_POWER     0x0A
#define ILI9341_READ_DISP_MADCTL    0x0B
#define ILI9341_READ_DISP_PIXEL     0x0C
#define ILI9341_READ_DISP_IMAGE     0x0D
#define ILI9341_READ_DISP_SIG       0x0E
#define ILI9341_READ_DISP_SDR       0x0F
#define ILI9341_SLEEP_MODE          0x10
#define ILI9341_SLEEP_OUT           0x11
#define ILI9341_PTL_ON              0x12
#define ILI9341_NORMAL_MODE_ON      0x13
#define ILI9341_DISP_INV_OFF        0x20
#define ILI9341_DISP_INV_ON         0x21
#define ILI9341_GAMMA               0x26
#define ILI9341_DISP_OFF            0x28
#define ILI9341_DISP_ON             0x29
#define ILI9341_COLUMN_ADDR         0x2A
#define ILI9341_PAGE_ADDR           0x2B
#define ILI9341_GRAM                0x2C
#define ILI9341_RGB_SET             0x2D
#define ILI9341_RAM_RD              0x2E
#define ILI9341_PTL_AREA            0x30
#define ILI9341_VSCR_DEF            0x33
#define ILI9341_TE_OFF              0x34
#define ILI9341_TE_ON               0x35
#define ILI9341_MAC                 0x36
#define ILI9341_VSCRS_ADDR          0x37
#define ILI9341_IDL_OFF             0x38
#define ILI9341_IDL_ON              0x39
#define ILI9341_PIXEL_FORMAT        0x3A
#define ILI9341_WRITE_MEM_CONT      0x3C
#define ILI9341_READ_MEM_CONT       0x3E
#define ILI9341_SET_TEAR_SCAN_LN    0x44
#define ILI9341_GET_SCAN_LN         0x45
#define ILI9341_WRITE_DISP_BR       0x51
#define ILI9341_READ_DISP_BR        0x52
#define ILI9341_WRITE_CTRL_DISP     0x53
#define ILI9341_READ_CTRL_DISP      0x54
#define ILI9341_WRITE_CABC          0x55
#define ILI9341_READ_CABC           0x56
#define ILI9341_WRITE_CABC_MIN      0x5E
#define ILI9341_READ_CABC_MIN       0x5F
#define ILI9341_READ_ID1            0xDA
#define ILI9341_READ_ID2            0xDB
#define ILI9341_READ_ID3            0xDC

/* ILI9341 extended command set */
#define ILI9341_RGB_INTERFACE       0xB0
#define ILI9341_FRM_CTRL1           0xB1
#define ILI9341_FRM_CTRL2           0xB2
#define ILI9341_FRM_CTRL3           0xB3
#define ILI9341_DISP_INV_CTRL       0xB4
#define ILI9341_BPC                 0xB5
#define ILI9341_DFC                 0xB6
#define ILI9341_ETMOD               0xB7
#define ILI9341_BACKLIGHT1          0xB8
#define ILI9341_BACKLIGHT2          0xB9
#define ILI9341_BACKLIGHT3          0xBA
#define ILI9341_BACKLIGHT4          0xBB
#define ILI9341_BACKLIGHT5          0xBC
#define ILI9341_BACKLIGHT7          0xBE
#define ILI9341_BACKLIGHT8          0xBF
#define ILI9341_POWER1              0xC0
#define ILI9341_POWER2              0xC1
#define ILI9341_VCOM1               0xC5
#define ILI9341_VCOM2               0xC7
#define ILI9341_NVMWR               0xD0
#define ILI9341_NVMPKEY             0xD1
#define ILI9341_RDNVM               0xD2
#define ILI9341_READ_ID4            0xD3
#define ILI9341_PGAMMA              0xE0
#define ILI9341_NGAMMA              0xE1
#define ILI9341_DGAMCTRL1           0xE2
#define ILI9341_DGAMCTRL2           0xE3
#define ILI9341_INTERFACE           0xF6
#define ILI9341_POWERA              0xCB
#define ILI9341_POWERB              0xCF
#define ILI9341_DTCA                0xE8
#define ILI9341_DTCB                0xEA
#define ILI9341_POWER_SEQ           0xED
#define ILI9341_3GAMMA_EN           0xF2
#define ILI9341_PRC                 0xF7

void ILI9341V_Init(void);

#endif /* LCD_H_ */
