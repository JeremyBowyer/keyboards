#pragma once

// Software chip select so the Vista508's active-high CS can be driven directly.
#define SPI_SELECT_MODE SPI_SELECT_MODE_NONE

#include_next <mcuconf.h>

// Set up SPI
#undef RP_SPI_USE_SPI1
#define RP_SPI_USE_SPI1 TRUE

#undef SPI_SELECT_MODE
#define SPI_SELECT_MODE SPI_SELECT_MODE_NONE
