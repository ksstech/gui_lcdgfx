# GUI_LCDGFX

set( srcs "gui_lcdgfx.cpp" )
set( include_dirs "." )
set( priv_include_dirs )
set( requires )
set( priv_requires
	hal_esp32 ili9341 printfx rtos-support systiming
	lcdgfx 
)

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS ${include_dirs}
	PRIV_INCLUDE_DIRS ${priv_include_dirs}
	REQUIRES ${requires}
	PRIV_REQUIRES ${priv_requires}
)
