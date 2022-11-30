# LCDGFX

set( srcs "gui_lcdgfx.cpp" )
#set( srcs "gui_lcdgfx.c" )
set( include_dirs "." )
set( priv_include_dirs )
set( requires  )
set( priv_requires common hal_esp32 ili9341 printfx rtos-support systiming )
list( APPEND priv_requires hal lcdgfx spi_flash ) 

idf_component_register(
	SRCS ${srcs}
	INCLUDE_DIRS ${include_dirs}
	PRIV_INCLUDE_DIRS ${priv_include_dirs}
	REQUIRES ${requires}
	PRIV_REQUIRES ${priv_requires}
)
