/*
 * gui_lcdgfx.cpp
 * Copyright (c) 2014-22 Andre M. Maree / KSS Technologies (Pty) Ltd.
 */

#if (cmakeGUI == 2)
#include "gui_lcdgfx.h"
#include "lcdgfx.h"

#include "hal_variables.h"
#include "hal_gpio.h"

#include "ili9341.h"
#include "struct_union.h"
#include "x_string_general.h"

#include "printfx.h"
#include "systiming.h"

// ######################################## Build macros ###########################################

#define	debugFLAG					0xF000

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

// ########################################### Macros ##############################################

#define	guiPRIORITY					2
#define	guiSTACK_SIZE				(configMINIMAL_STACK_SIZE + 1024 + (flagSTACK * 256))
#define guiINTERVAL_MS				3000

#define	ili9341GPIO_MOSI			GPIO_NUM_23
#define	ili9341GPIO_MISO			GPIO_NUM_25
#define	ili9341GPIO_SCLK			GPIO_NUM_19
#define	ili9341GPIO_RESET			GPIO_NUM_18
#define	ili9341GPIO_D_C_X			GPIO_NUM_21
#define	ili9341GPIO_LIGHT			GPIO_NUM_5
#define ili9341GPIO_CS				GPIO_NUM_22

#define guiBUF_SIZE					(40 * 25)
#define guiOPTION					1					// 1= CANVAS, rest = DISPLAY

// ####################################### Private variables #######################################

static StaticTask_t ttsGUI;
static StackType_t tsbGUI[guiSTACK_SIZE] = { 0 };
static char Buffer[guiBUF_SIZE];
static u8_t Level = 0;

//		ESP32-WROVER-KIT			  Rst busId  cs 	dc	freq	 scl  sda
DisplayILI9341_240x320x16_SPI display (
	ili9341GPIO_RESET,
	{	0,
		ili9341GPIO_CS,
		ili9341GPIO_D_C_X,
		26000000,
		ili9341GPIO_SCLK,
		ili9341GPIO_MOSI,
	}
);

#if (guiOPTION == 1)
typedef NanoCanvas<320,240,1> canvas_mono;
canvas_mono canvas;										// For allocation on the stack
#endif

// ####################################### GUI support APIs ########################################

void vGUI_Init(void) {
	ili9341BacklightInit();
	display.init();
	display.clear();
	display.getInterface().setRotation(3);
	#if (guiOPTION == 1)
	canvas.setFixedFont(ssd1306xled_font6x8);
	#else
	display.setFixedFont(ssd1306xled_font6x8);
	#endif
}

void vGuiBrightness(void) {
	Level %= 100;
	ili9341BacklightLevel(Level += 5);
}

int vGuiPageTasks(void) {
	return xRtosReportTasks(Buffer, guiBUF_SIZE, (fm_t) makeMASK09x23(0,0,1,1,1,1,1,0,1,0x03FFFFF));
}

void vGuiPageMemory(void) {
	return vRtosReportMemory(Buffer, guiBUF_SIZE, (fm_t) makeMASK11x21(0,1,1,1,1,1,1,1,1,1,1,0));
}

void vGuiPageFlags(void) {
	int iRV;
	u32_t EFcur;
	iRV = xBitMapDecodeChanges(EFprv, EFcur, 0x0000F000, EFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	iRV += xBitMapDecodeChanges(EFprv, EFcur, 0x00000F00, EFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	iRV += xBitMapDecodeChanges(EFprv, EFcur, 0x000000F0, EFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	iRV += xBitMapDecodeChanges(EFprv, EFcur, 0x0000000F, EFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	EFprv = EFcur;
	iRV += snprintfx(Buffer+iRV, guiBUF_SIZE-iRV, "System Flags : 0x%08X\r\n", SFcur);
	iRV += xBitMapDecodeChanges(SFprv, SFcur, 0x000FC000, SFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	iRV += xBitMapDecodeChanges(SFprv, SFcur, 0x00003F00, SFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	iRV += xBitMapDecodeChanges(SFprv, SFcur, 0x000000FC, SFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	iRV += xBitMapDecodeChanges(SFprv, SFcur, 0x00000003, SFnames, bmdcNEWLINE, Buffer+iRV, guiBUF_SIZE-iRV);
	SFprv = SFcur;
	iRV += snprintfx(Buffer+iRV, guiBUF_SIZE-iRV, "%!R  %Z\r\n", RunTime, &sTSZ);
	iRV += snprintfx(Buffer+iRV, guiBUF_SIZE-iRV, "Level=%u\r\n", Level);
	return iRV;
}

void vGuiUpdate(void) {
	static u8_t Page = 0;
	switch(Page) {
	case 0: vGuiPageTasks(); break;
	case 1: vGuiPageFlags(); break;
	case 2: vGuiPageMemory(); break;
	default: break;
	}
	Page = ++Page % 3;
}

void vGuiRefresh(void) {
	#if (guiOPTION == 1)
    canvas.clear();
	canvas.setColor(RGB_COLOR16(255,255,255));
    canvas.printFixed(0, 0, (const char *) Buffer, STYLE_NORMAL);
    display.drawCanvas(0, 0, canvas);
    #else
    display.fill(0x0000);
	display.setColor(RGB_COLOR16(255,255,255));
    display.printFixed(0, 0, (const char *) Buffer, STYLE_NORMAL);
	#endif
}

static void vTaskGUI(void * pVoid) {
	IF_SYSTIMER_INIT(debugTIMING, stGUI0, stMILLIS, "GUI0", 1, 10);
	IF_SYSTIMER_INIT(debugTIMING, stGUI1, stMILLIS, "GUI1", 30, 600);
	vTaskSetThreadLocalStoragePointer(NULL, 1, (void *)taskGUI_MASK);
	vGUI_Init();
	xRtosSetStateRUN(taskGUI_MASK);

    while(bRtosVerifyState(taskGUI_MASK)) {
    	vGuiBrightness();

    	IF_SYSTIMER_START(debugTIMING, stGUI0);
    	vGuiUpdate();
    	IF_SYSTIMER_STOP(debugTIMING, stGUI0);

    	IF_SYSTIMER_START(debugTIMING, stGUI1);
    	vGuiRefresh();
    	IF_SYSTIMER_STOP(debugTIMING, stGUI1) ;

        vTaskDelay(pdMS_TO_TICKS((ioB4GET(ioGUIintval)+1) * MILLIS_IN_SECOND));
    }
    canvas.clear();
    display.drawCanvas(0, 0, canvas);
	vRtosTaskDelete(NULL);
}

extern "C" void vTaskGUI_Start(void * pvPara) {
	xRtosTaskCreateStatic(vTaskGUI, "gui", guiSTACK_SIZE, pvPara, guiPRIORITY, tsbGUI, &ttsGUI, tskNO_AFFINITY);
}
#endif
