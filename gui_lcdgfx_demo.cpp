/*
 * gui_lcdgfx.cpp
 * Copyright (c) 2014-22 Andre M. Maree / KSS Technologies (Pty) Ltd.
 */

#include "main.h"
#if (cmakeGUI == 1)
#include "gui_lcdgfx.h"
#include "lcdgfx.h"

#include "hal_variables.h"

#include "struct_union.h"
#include "FreeRTOS_Support.h"

#include "printfx.h"
#include "systiming.h"

#define	debugFLAG					0xF000

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

//		ESP32-WROVER-KIT			  Rst busId  cs 	dc	freq	 scl  sda
DisplayILI9341_240x320x16_SPI display(
	ili9341GPIO_RESET,
	{	0,
		ili9341GPIO_CS,
		ili9341GPIO_D_C_X,
		26000000,
		ili9341GPIO_SCLK,
		ili9341GPIO_MOSI,
	}
);

void vTaskGUI_BackLightStatus(bool Status) {
    gpio_set_direction(ili9341GPIO_LIGHT, GPIO_MODE_OUTPUT) ;
    gpio_set_level(ili9341GPIO_LIGHT, !Status) ;
}

void	vTaskGUI_BackLightSetLevel(u8_t Level) {
    gpio_set_direction(ili9341GPIO_LIGHT, GPIO_MODE_OUTPUT) ;
}

/*
 * Heart image below is defined directly in flash memory.
 * This reduces SRAM consumption.
 * The image is defined from bottom to top (bits), from left to
 * right (bytes).
 */
const PROGMEM u8_t heartImage[8] = {
    0B00001110,
    0B00011111,
    0B00111111,
    0B01111110,
    0B01111110,
    0B00111101,
    0B00011001,
    0B00001110
};

const PROGMEM u8_t heartImage8[ 8 * 8 ] = {
    0x00, 0xE0, 0xE0, 0x00, 0x00, 0xE5, 0xE5, 0x00,
    0xE0, 0xC0, 0xE0, 0xE0, 0xE0, 0xEC, 0xEC, 0xE5,
    0xC0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE5, 0xEC, 0xE5,
    0x80, 0xC0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE5, 0xE0,
    0x00, 0x80, 0xC0, 0xE0, 0xE0, 0xE0, 0xE0, 0x00,
    0x00, 0x00, 0x80, 0xE0, 0xE0, 0xE0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0xE0, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
} ;

SAppMenu menu ;

//const char * const menuItems[] = { "draw bitmap", "sprites", "fonts", "nano canvas", "draw lines" } ;
const char * menuItems[] = { "draw bitmap", "sprites", "fonts", "nano canvas", "draw lines" } ;

u8_t rotation = 0;

/* Sprites are not implemented for color modes but there is NanoEngine support
 * To make example clear, we use lambda as function pointer. Since lambda can be
 * passed to function only if it doesn't capture, all variables should be global.
 * Refer to C++ documentation.
 */
NanoPoint sprite;
NanoEngine16<DisplayILI9341_240x320x16_SPI> engine( display );

static void bitmapDemo(void) {
    display.clear();
    display.setColor(RGB_COLOR16(64,64,255));
//    display.drawBitmap1(0, 0, 128, 64, Owl);
    display.drawBitmap8(0, 0, 8, 8, heartImage8);
    display.setColor(RGB_COLOR16(255,64,64));
    display.drawBitmap1(0, 16, 8, 8, heartImage);
    lcd_delay(3000);
}

static void spriteDemo(void) {
    // We not need to clear screen, engine will do it for us
    engine.begin();
    // Force engine to refresh the screen
    engine.refresh();
    // Set function to draw our sprite
    engine.drawCallback( []()->bool {
        engine.getCanvas().clear();
        engine.getCanvas().setColor( RGB_COLOR16(255, 32, 32) );
        engine.getCanvas().drawBitmap1( sprite.x, sprite.y, 8, 8, heartImage );
        return true;
    } );
    sprite.x = 0;
    sprite.y = 0;
    for (int i=0; i<250; i++)
    {
        lcd_delay(15);
        // Tell the engine to refresh screen at old sprite position
        engine.refresh( sprite.x, sprite.y, sprite.x + 8 - 1, sprite.y + 8 - 1 );
        sprite.x++;
        if (sprite.x >= display.width())
        {
            sprite.x = 0;
        }
        sprite.y++;
        if (sprite.y >= display.height())
        {
            sprite.y = 0;
        }
        // Tell the engine to refresh screen at new sprite position
        engine.refresh( sprite.x, sprite.y, sprite.x + 8 - 1, sprite.y + 8 - 1 );
        // Do refresh required parts of screen
        engine.display();
    }
}

static void textDemo(void) {
    display.setFixedFont(ssd1306xled_font6x8);
    display.clear();
    display.setColor(RGB_COLOR16(255,255,0));
    display.printFixed(0,  8, "Normal text", STYLE_NORMAL);
    display.setColor(RGB_COLOR16(0,255,0));
    display.printFixed(0, 16, "bold text?", STYLE_BOLD);
    display.setColor(RGB_COLOR16(0,255,255));
    display.printFixed(0, 24, "Italic text?", STYLE_ITALIC);
//    display.negativeMode();
    display.setColor(RGB_COLOR16(255,255,255));
    display.printFixed(0, 32, "Inverted bold?", STYLE_BOLD);
//    display.positiveMode();
    lcd_delay(3000);
}

static void canvasDemo(void) {
    NanoCanvas<64,16,1> canvas;
    display.setColor(RGB_COLOR16(0,255,0));
    display.clear();
    canvas.clear();
    canvas.fillRect(10, 3, 80, 5);
    display.drawCanvas((display.width()-64)/2, 1, canvas);
    lcd_delay(500);
    canvas.fillRect(50, 1, 60, 15);
    display.drawCanvas((display.width()-64)/2, 1, canvas);
    lcd_delay(1500);
    canvas.setFixedFont(ssd1306xled_font6x8);
    canvas.printFixed(20, 1, " DEMO ", STYLE_BOLD );
    display.drawCanvas((display.width()-64)/2, 1, canvas);
    lcd_delay(3000);
}

static void drawLinesDemo(void) {
    display.clear();
    display.setColor(RGB_COLOR16(255,0,0));
    for (uint16_t y = 0; y < display.height(); y += 8)
    {
        display.drawLine(0,0, display.width() -1, y);
    }
    display.setColor(RGB_COLOR16(0,255,0));
    for (uint16_t x = display.width() - 1; x > 7; x -= 8)
    {
        display.drawLine(0,0, x, display.height() - 1);
    }
    lcd_delay(3000);
}

void	vTaskLCDGFX(void * pVoid) {
	IF_SYSTIMER_INIT(debugTIMING, stGUI0, stMILLIS, "GUI0", 1, 50) ;
	IF_SYSTIMER_INIT(debugTIMING, stGUI1, stMILLIS, "GUI1", 3000, 5000) ;
	vTaskGUI_BackLightStatus(1) ;
    display.begin() ;
    display.getInterface().setRotation(3) ;
    display.setFixedFont(ssd1306xled_font6x8) ;
    display.fill( 0x0000 ) ;
    display.createMenu( &menu, menuItems, sizeof(menuItems) / sizeof(char *) ) ;
    display.showMenu( &menu ) ;
	vTaskSetThreadLocalStoragePointer(NULL, 1, (void *)taskGUI_MASK) ;
	xRtosSetStateRUN(taskGUI_MASK) ;

    while(bRtosVerifyState(taskGUI_MASK)) {
        vTaskDelay(pdMS_TO_TICKS(1000)) ;
        switch (display.menuSelection(&menu)) {
            case 0:	bitmapDemo() ;		break ;
            case 1:	spriteDemo() ;		break ;
            case 2:	textDemo() ;		break ;
            case 3:	canvasDemo() ;		break ;
            case 4:	drawLinesDemo() ;	break ;
            default:					break ;
        }
        if ((menu.count - 1) == display.menuSelection(&menu)) {
             display.getInterface().setRotation((++rotation) & 0x03) ;
        }
        display.fill( 0x00 ) ;
        display.setColor(RGB_COLOR16(255,255,255)) ;
        display.showMenu(&menu) ;
        vTaskDelay(pdMS_TO_TICKS(500)) ;
        display.menuDown(&menu) ;
        display.updateMenu(&menu) ;
    }
	vRtosTaskDelete(NULL) ;
}
#endif
