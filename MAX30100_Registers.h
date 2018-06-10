/*
MAX30100 register
*/

#define MAX30100_ADDRESS          0x57 //7-bit I2C Address
#define MAX30100_FIFO_DEPTH       0x10

// Status Registers
static const uint8_t MAX30100_INTSTAT   	  =	0x00;
static const uint8_t MAX30100_INTENABLE 	  =	0x01;

// FIFO Registers
static const uint8_t MAX30100_FIFOWRITEPTR 	= 0x02;
static const uint8_t MAX30100_FIFOOVERFLOW 	=	0x03;
static const uint8_t MAX30100_FIFOREADPTR  	=	0x04;
static const uint8_t MAX30100_FIFODATA     	=	0x05;

// Configuration Registers
static const uint8_t MAX30100_MODECONFIG    =	0x06;
static const uint8_t MAX30100_SPO2CONFIG    =	0x07;    
static const uint8_t MAX30100_LEDCONFIG     =	0x09;

// Die Temperature Registers
static const uint8_t MAX30100_DIETEMPINT    =	0x16;
static const uint8_t MAX30100_DIETEMPFRAC   =	0x17;

// Part ID Registers
static const uint8_t MAX30100_REVISIONID    =	0xFE;
static const uint8_t MAX30100_PARTID        =	0xFF;

////////////////////////////////////////////////////////////////////////
// MAX30100 Status
// Interrupt status
static const uint8_t MAX30100_INT_A_FULL_MASK      =	(byte)~0b10000000;
static const uint8_t MAX30100_INT_A_FULL_ENABLE    =  (byte) 0b10000000;
static const uint8_t MAX30100_INT_A_FULL_DISABLE   =  (byte) 0b00000000;

static const uint8_t MAX30100_INT_TEMP_RDY_MASK    = 	(byte)~0b01000000;
static const uint8_t MAX30100_INT_TEMP_RDY_ENABLE  =	(byte)~0b01000000;
static const uint8_t MAX30100_INT_TEMP_RDY_DISABLE = 	(byte) 0b00000000;

static const uint8_t MAX30100_INT_HR_RDY_MASK      =  (byte)~0b00100000;
static const uint8_t MAX30100_INT_HR_RDY_ENABLE    = 	(byte) 0b00100000;
static const uint8_t MAX30100_INT_HR_RDY_DISABLE   = 	(byte) 0b00000000;

static const uint8_t MAX30100_INT_SPO2_RDY_MASK    =  (byte)~0b00010000;
static const uint8_t MAX30100_INT_SPO2_RDY_ENABLE  = 	(byte) 0b00010000;
static const uint8_t MAX30100_INT_SPO2_RDY_DISABLE = 	(byte) 0b00000000;

static const uint8_t MAX30100_INT_PWR_RDY_MASK     =  (byte)~0b00000001;

// Mode configuration commands (0x06)
static const uint8_t MAX30100_SHUTDOWN_MASK    = 	(byte)~0b10000000;
static const uint8_t MAX30100_SHUTDOWN         =	(byte) 0b10000000;
static const uint8_t MAX30100_WAKEUP           =	(byte) 0b00000000;

static const uint8_t MAX30100_RESET_MASK       =  (byte)~0b01000000;
static const uint8_t MAX30100_RESET            = 	(byte) 0b01000000;

static const uint8_t MAX30100_TEMPREAD_MASK    =  (byte)~0b00001000;
static const uint8_t MAX30100_TEMPREAD         =  (byte) 0b00001000;

static const uint8_t MAX30100_MODE_MASK        =	(byte)~0b00000111;
static const uint8_t MAX30100_MODE_HR          = 	(byte) 0b00000010;
static const uint8_t MAX30100_MODE_SPO2        = 	(byte) 0b00000011;

// SPO2 configuration commands (0x07)
static const uint8_t MAX30100_SPO2HIRESEN_MASK =  (byte)~0b01000000;
static const uint8_t MAX30100_SPO2HIRES_ENABLE =  (byte) 0b01000000; // 16bit ADC & 1.6ms
static const uint8_t MAX30100_SPO2HIRES_DISABLE=  (byte) 0b00000000;

static const uint8_t MAX30100_SAMPLERATE_MASK  =  (byte)~0b00011100;
static const uint8_t MAX30100_SAMPLERATE_50    =  (byte) 0b00000000;
static const uint8_t MAX30100_SAMPLERATE_100   =  (byte) 0b00000100;
static const uint8_t MAX30100_SAMPLERATE_167   =  (byte) 0b00001000;
static const uint8_t MAX30100_SAMPLERATE_200   =  (byte) 0b00001100;
static const uint8_t MAX30100_SAMPLERATE_400   =  (byte) 0b00010000;
static const uint8_t MAX30100_SAMPLERATE_600   =  (byte) 0b00010100;
static const uint8_t MAX30100_SAMPLERATE_800   =  (byte) 0b00011000;
static const uint8_t MAX30100_SAMPLERATE_1000  =  (byte) 0b00011100;

static const uint8_t MAX30100_PULSEWIDTH_MASK  = (byte)~0b00000011;
static const uint8_t MAX30100_PULSEWIDTH_200   = (byte) 0b00000000; // 13bit ADC
static const uint8_t MAX30100_PULSEWIDTH_400   = (byte) 0b00000001; // 14bit ADC
static const uint8_t MAX30100_PULSEWIDTH_800   = (byte) 0b00000010; // 15bit ADC
static const uint8_t MAX30100_PULSEWIDTH_1600  = (byte) 0b00000011; // 16bit ADC

static const uint8_t MAX30100_IRLED_CURR_MASK     = (byte)~0b00001111;
static const uint8_t MAX30100_IRLED_CURR_0MA      = (byte) 0b00000000;
static const uint8_t MAX30100_IRLED_CURR_4_4MA    = (byte) 0b00000001;
static const uint8_t MAX30100_IRLED_CURR_7_6MA    = (byte) 0b00000010;
static const uint8_t MAX30100_IRLED_CURR_11MA     = (byte) 0b00000011;
static const uint8_t MAX30100_IRLED_CURR_14_2MA   = (byte) 0b00000100;
static const uint8_t MAX30100_IRLED_CURR_17_4MA   = (byte) 0b00000101;
static const uint8_t MAX30100_IRLED_CURR_20_8MA   = (byte) 0b00000110;
static const uint8_t MAX30100_IRLED_CURR_24MA     = (byte) 0b00000111;
static const uint8_t MAX30100_IRLED_CURR_27_1MA   = (byte) 0b00001000;
static const uint8_t MAX30100_IRLED_CURR_30_6MA   = (byte) 0b00001001;
static const uint8_t MAX30100_IRLED_CURR_33_8MA   = (byte) 0b00001010;
static const uint8_t MAX30100_IRLED_CURR_37MA     = (byte) 0b00001011;
static const uint8_t MAX30100_IRLED_CURR_40_2MA   = (byte) 0b00001100;
static const uint8_t MAX30100_IRLED_CURR_43_6MA   = (byte) 0b00001101;
static const uint8_t MAX30100_IRLED_CURR_46_8MA   = (byte) 0b00001110;
static const uint8_t MAX30100_IRLED_CURR_50MA     = (byte) 0b00001111;

static const uint8_t MAX30100_REDLED_CURR_MASK     = (byte)~0b11110000;
static const uint8_t MAX30100_REDLED_CURR_0MA      = (byte) 0b00000000;
static const uint8_t MAX30100_REDLED_CURR_4_4MA    = (byte) 0b00010000;
static const uint8_t MAX30100_REDLED_CURR_7_6MA    = (byte) 0b00100000;
static const uint8_t MAX30100_REDLED_CURR_11MA     = (byte) 0b00110000;
static const uint8_t MAX30100_REDLED_CURR_14_2MA   = (byte) 0b01000000;
static const uint8_t MAX30100_REDLED_CURR_17_4MA   = (byte) 0b01010000;
static const uint8_t MAX30100_REDLED_CURR_20_8MA   = (byte) 0b01100000;
static const uint8_t MAX30100_REDLED_CURR_24MA     = (byte) 0b01110000;
static const uint8_t MAX30100_REDLED_CURR_27_1MA   = (byte) 0b10000000;
static const uint8_t MAX30100_REDLED_CURR_30_6MA   = (byte) 0b10010000;
static const uint8_t MAX30100_REDLED_CURR_33_8MA   = (byte) 0b10100000;
static const uint8_t MAX30100_REDLED_CURR_37MA     = (byte) 0b10110000;
static const uint8_t MAX30100_REDLED_CURR_40_2MA   = (byte) 0b11000000;
static const uint8_t MAX30100_REDLED_CURR_43_6MA   = (byte) 0b11010000;
static const uint8_t MAX30100_REDLED_CURR_46_8MA   = (byte) 0b11100000;
static const uint8_t MAX30100_REDLED_CURR_50MA     = (byte) 0b11110000;

// SPO2 Mode
// 50,100S/s 200-1600uS pulse, 16 bit
// 167,200S/s 200-800uS pulse, 15bit
// 400S/s 200,400uS, 14bit
// 600,800,100, 200uS, 13bit

// TEMPERATURE INTEER 0x16
static const uint8_t MAX30100_TEMPINT_MASK   =  (byte)~0b11111111;
// TEMPERATURE FRACTION 0x17
static const uint8_t MAX30100_TEMPFRAC_MASK  =  (byte)~0b00001111;

// Device IDs
static const uint8_t MAX_30100_EXPECTEDPARTID = 0x11;
