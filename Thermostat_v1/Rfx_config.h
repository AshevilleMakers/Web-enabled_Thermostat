
#define SKETCH_PASS

#define MASTER 1
#define SLAVE 2

// the defines above are minmum required in this file, and shouldn't be changed.
// anything below is optional configuration. Any values that are not explicitly
// defined will be picked up from the default_config.h file in the library directory.
// Board/shield types defines for spi config and flash strings mgmt

#define NODE_TYPE SLAVE // define as slave or master

#define BOARD_ID 105 // nominate ethernet port address to link this node io to
#define ID_STRING "Bsmt tstat"

//#define BOARD_ID 110 // nominate ethernet port address to link this node io to
//#define ID_STRING "Main tstat"

#define FLASH_STRINGS 1 // use flash memory for strings to save ram

#define OUTPUT_BUFSZE 180 // size of buffer for radio output stream
#define INPUT_BUFSZE 90  // size of buffered io for radio input stream
// put these in xxxconfig.h. also put fprintf buffer size there.

#define READ_FROM_ETH 0 // intercept debug cmds from input stream

/*
 board defines for spi config:

 check default_config.h, but list includes:
 BOARD_TYPE_RPI  // ARM Embedded Linux 
 BOARD_TYPE_UNO // use for Uno, Nano, or any generic atmega328/atmega328p based device
 BOARD_TYPE_MEGA  // use for atmega1280/2560 
 BOARD_TYPE_LEO  // Arduino Leonardo
 BOARD_TYPE_DUE  // Arduino Due ARM Cortex-M3 board
 BOARD_TYPE_MAPLE5  // Leaflabs Maple5 ARM Cortex-M3 board
 BOARD_TYPE_TEENSY3  // PJRC Teensy 3.0 ARM Cortex-M4 board
 BOARD_TYPE_BOB  // "Bobuino" 1284p or compatible board
*/

#define BOARD_TYPE_BOB 1
 
/*
 spi pin assignment defines for board/shield layouts:

 SPI_IBOARD, SPI_NANO_IO_SHIELD, SPI_DEMI_SHIELD.

 SPI_* specifies standard shield layouts already defined, including CSN, IRQ
 and CE pins (if HW_SPI is 0 then also defines pins MO, MI, SCK):

 SPI_NANO_IO_SHIELD // Uno-style HW Spi:
 11 (MO), 12 (MI), 13 (SCK), plus 2 (IRQ), 9 (CE), 10 (CSN)

 SPI_IBOARD // Itead Iboard SW Spi:
 5 (MO), 6 (MI), 7 (SCK), plus 2 (IRQ), 3 (CE), 8 (CSN)

 SPI_DEMI_SHIELD // EmbeddedCoolness' own RFX demi_shield, proto shield, 
 and proto board designs (selectable as either HW or SW Spi):
 
 SW Spi (mo, mi, sck bit-banged on digital io pins):
 4 (MO), 6 (MI), 5 (SCK), plus 2 (IRQ), 3 (CE), 7 (CSN)
 - use on any board (e.g. UNO, MEGA, DUE) when HW Spi is unavailable

 HW Spi (digitial io pins):
 11 (MO), 12 (MI), 13 (SCK), plus 2 (IRQ), 3 (CE), 7 (CSN)
 - use for HW Spi on atmega328-based boards (e.g., UNO, Duemilanov), when 
   ICSP HW_SPI pins are unavailable 
 
 HW Spi (icsp pins for mo, mi, sck, digital io pins for CSN, CE, IRQ):
 ICSP (MO), ICSP (MI), ICSP (SCK), plus 2 (IRQ), 3 (CE), 7 (CSN)
 - use when only ICSP HW Spi pins available, (e.g., MEGA, LEO, DUE)

*/

//#define SPI_DEMI_SHIELD 1
#define SPI_CUSTOM 1
#define IRQ_PIN_CUSTOM 2
#define CE_PIN_CUSTOM 31
#define CSN_PIN_CUSTOM 9
#define MOSI_PIN_CUSTOM 22
#define MISO_PIN_CUSTOM 24
#define SCK_PIN_CUSTOM 23

/*
 for devices that use hw spi, HW_SPI sepecifies the no. of spi port (some 
 devices have more than one spi port, e.g., MAPLE5). Note: numbering starts 
 at 1. If HW_SPI is 0, then SW (bit-bashing) SPI is used. 
*/

#define HW_SPI 0

