#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"
#include "Macros.h"


#define BUFF_SIZE 512
#define LEDPIN 6
#define PREFIXBYTE 0x42
#define ROMSIZE 32768
#define EEPROM_PAGE_SIZE 64
#define EEPROM_PAGE_MASK 0xffc0

void SetupHardware(void);
void FatalError(void);
void ClearCommand(void);
void Echo(const char* string);
void WriteBlock(uint16_t startAddr, uint16_t length);
void ReadBlock(uint16_t startAddr, uint16_t length, bool maskROM);
void SetAddressMSB(uint8_t addr, bool maskROM);
uint32_t CRC32(uint8_t *buf, size_t size);


void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);



// LUFA CDC Class driver interface configuration and state information.
USB_ClassInfo_CDC_Device_t EETool_CDC_Interface =
{
	.Config =
		{
			.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
			.DataINEndpoint           =
				{
					.Address          = CDC_TX_EPADDR,
					.Size             = CDC_TXRX_EPSIZE,
					.Banks            = 1,
				},
			.DataOUTEndpoint =
				{
					.Address          = CDC_RX_EPADDR,
					.Size             = CDC_TXRX_EPSIZE,
					.Banks            = 1,
				},
			.NotificationEndpoint =
				{
					.Address          = CDC_NOTIFICATION_EPADDR,
					.Size             = CDC_NOTIFICATION_EPSIZE,
					.Banks            = 1,
				},
		},
};


typedef enum
{
	CMD_NONE,
	CMD_PING,
	CMD_STARTRW,
	CMD_READBLOCK,
	CMD_READBLOCK_MASK,
	CMD_WRITEBLOCK
} Command;


Command gCurrentCommand = CMD_NONE;
uint8_t gCommBuffer[BUFF_SIZE] = "";
uint16_t gCurrentAddress = 0;



int main(void)
{
	SetupHardware();
	GlobalInterruptEnable();

	for (;;)
	{
		// Check for host data
		uint16_t byteCount = CDC_Device_BytesReceived(&EETool_CDC_Interface);
		byteCount = MIN(byteCount,BUFF_SIZE-1);
		
		if (byteCount>BUFF_SIZE)
		{
			FatalError();
		}
		
		if (byteCount>0)
		{
			// Read all data from host
			int i;
			for (i=0; i<byteCount; i++)
			{
				int16_t value = CDC_Device_ReceiveByte(&EETool_CDC_Interface);
				if (value>=0)
				{
					gCommBuffer[i] = (uint8_t)value;
				}
			}
		
			// Check for new command
			if (gCurrentCommand==CMD_NONE && gCommBuffer[0]==PREFIXBYTE)
			{
				gCurrentCommand = gCommBuffer[1];
				
				// New command
				switch (gCurrentCommand)
				{
					case CMD_PING:
					{
						Echo("PONG");
						ClearCommand();
						break;
					}
						
					case CMD_STARTRW:
					{
						gCurrentAddress = 0;
						PULSEC(LEDPIN);
						break;
					}
						
					case CMD_READBLOCK:
					{
						ReadBlock(gCurrentAddress,BUFF_SIZE,false);
						gCurrentAddress += BUFF_SIZE;
						break;
					}
					
					case CMD_READBLOCK_MASK:
					{
						ReadBlock(gCurrentAddress,BUFF_SIZE,true);
						gCurrentAddress += BUFF_SIZE;
						break;
					}
					
					case CMD_WRITEBLOCK:
					{
						WriteBlock(gCurrentAddress,BUFF_SIZE);
						gCurrentAddress += BUFF_SIZE;
						break;
					}
						
					default:
						FatalError();
						break;
				}
				
				ClearCommand();
			}
		}
		
		CDC_Device_USBTask(&EETool_CDC_Interface);
		USB_USBTask();
	}
}

void SetupHardware(void)
{
	// Disable watchdog if enabled by bootloader/fuses
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// Disable clock division
	clock_prescale_set(clock_div_1);

	// Configure IO pins

	// Address bus, LSB
	DDRB = QDDRB(0) | QDDRB(1) | QDDRB(2) | QDDRB(3) | QDDRB(4) | QDDRB(5) | QDDRB(6) | QDDRB(7);
	
	// Address bus, MSB
	DDRF = QDDRF(0) | QDDRF(1) | QDDRF(4) | QDDRF(5) | QDDRF(6) | QDDRF(7);
	DDRC = QDDRC(6) | QDDRC(7);		// Also LED output
	
	// Control bits
	DDRE = QDDRE(2) | QDDRE(6);	// Output enable  ,   Write Enable / Mask ROM high bit
	
	// Initialize control signals
	SETE_HI(6);
	SETE_HI(2);
	SETC_LO(LEDPIN);
	
	USB_Init();
}


void FatalError(void)
{
	for (;;)
	{
		SETC_LO(LEDPIN);
		_delay_ms(50);
		SETC_HI(LEDPIN);
		_delay_ms(50);
	}
}


void ClearCommand(void)
{
	gCommBuffer[0] = 0;
	gCurrentCommand = CMD_NONE;
}


void Echo(const char* string)
{
	PULSEC(LEDPIN);
	
	CDC_Device_SendString(&EETool_CDC_Interface, string);
}

void SetAddressMSB(uint8_t addr, bool maskROM)
{
	// Drives the high address lines to the specified value. These lines
	// are all mixed up to get around the holes in the AVR's IO space
	if (addr & 0x1)
	{
		SETF_HI(0);
	}
	else
	{
		SETF_LO(0);
	}
	addr>>=1;
	
	if (addr & 0x1)
	{
		SETF_HI(1);
	}
	else
	{
		SETF_LO(1);
	}
	addr>>=1;

	if (addr & 0x1)
	{
		SETF_HI(4);
	}
	else
	{
		SETF_LO(4);
	}
	addr>>=1;

	if (addr & 0x1)
	{
		SETF_HI(5);
	}
	else
	{
		SETF_LO(5);
	}
	addr>>=1;

	if (addr & 0x1)
	{
		SETF_HI(6);
	}
	else
	{
		SETF_LO(6);
	}
	addr>>=1;

	if (addr & 0x1)
	{
		SETF_HI(7);
	}
	else
	{
		SETF_LO(7);
	}
	addr>>=1;

	if (maskROM)
	{
		SETC_HI(7);		// For Mask ROM, this provides extra Vcc

		if (addr & 0x1)
		{
			SETE_HI(6);
		}
		else
		{
			SETE_LO(6);
		}
	}
	else
	{
		if (addr & 0x1)
		{
			SETC_HI(7);
		}
		else
		{
			SETC_LO(7);
		}
	}
}


void ReadBlock(uint16_t startAddr, uint16_t length, bool maskROM)
{
	if (startAddr >= ROMSIZE)
	{
		FatalError();
	}
	
	_delay_ms(10);		// Give the host a moment to set up before we flood them with data
	
	DDRD = 0;			// Data bus for input
	SETE_HI(2);			// Initialize Output Enable
	
	if (!maskROM)
	{
		SETE_HI(6);		// Write Enable to disable
		FatalError();
	}
	
	for (uint16_t i=startAddr; i<startAddr+length; i++)
	{
		// Drive address lines
		PORTB = (uint8_t)(i & 0xff);	// LSB
		SetAddressMSB((uint8_t)(i>>8),maskROM);
		
		// Show falling edge to Output Enable
		SETE_LO(2);
		
		_delay_us(1);
		
		// Read the data byte
		uint8_t data = PIND;
		
		SETE_HI(2);
		uint16_t index = i-startAddr;
		if (index>=BUFF_SIZE)
		{
			FatalError();
		}
		
		gCommBuffer[index] = data;
	}
		
	// Send data to host
	CDC_Device_SendData(&EETool_CDC_Interface, gCommBuffer, length);
	
	// Compute a 32 bit CRC and send it
	uint32_t crc = CRC32(gCommBuffer,length);
	for (int i=0; i<4; i++)
	{
		CDC_Device_SendByte(&EETool_CDC_Interface, (uint8_t)(crc & 0xff));
		crc>>=8;
	}
	_delay_ms(10);
	
	PULSEC(LEDPIN);
}


void WriteBlock(uint16_t startAddr, uint16_t length)
{
	// Configure data bus for output
	DDRD = QDDRD(0) | QDDRD(1) | QDDRD(2) | QDDRD(3) | QDDRD(4) | QDDRD(5) | QDDRD(6) | QDDRD(7);

	if (startAddr+length > ROMSIZE)
	{
		FatalError();
	}
		
	SETE_HI(2);			// Force Output Enable high for writing
	SETE_HI(6);			// Initialize Write Enable to disabled

	uint16_t pageNum = startAddr & EEPROM_PAGE_MASK;
	uint16_t pageByte = 0;
	
	// Wait for entire block from host
	uint16_t byteCount = 0;
	while (byteCount<BUFF_SIZE)
	{
		uint16_t numBytes = CDC_Device_BytesReceived(&EETool_CDC_Interface);
		
		uint16_t i;
		for (i=0; i<numBytes; i++)
		{
			int16_t value = CDC_Device_ReceiveByte(&EETool_CDC_Interface);
			if (value>=0)
			{
				gCommBuffer[byteCount] = (uint8_t)value;
			}
			byteCount++;
			if (byteCount>=BUFF_SIZE)
			{
				break;
			}
		}

	}

	if (byteCount>BUFF_SIZE)
	{
		FatalError();
	}
	
	// Write data to the EEPROM
	for (uint16_t i=startAddr; i<startAddr+length; i++)
	{
		// Drive address lines
		PORTB = (uint8_t)(i & 0xff);	// LSB
		SetAddressMSB((uint8_t)(i>>8),false);
		
		// Prepare read buffer
		uint16_t index = i-startAddr;
		if (index>=BUFF_SIZE)
		{
			FatalError();
		}
		
		// Drive data lines
		PORTD = gCommBuffer[index];
		
		// Show falling edge to Write Enable
		SETE_LO(6);
		SETE_HI(6);

		_delay_ms(8);  // For now, just doing "byte write" mode. Easier than page write (see below), but glacially slow
		
		// After each EEPROM page, wait for the write to complete. Making page write mode work requires hitting small
		// timing windows which is difficult to do from general-purpose C code like this.
//		uint16_t nextPageNum = i & EEPROM_PAGE_MASK;
//		
//		pageByte++;
//		if (pageByte >= EEPROM_PAGE_SIZE || pageNum != nextPageNum)
//		{
//			pageByte = 0;
//			pageNum = nextPageNum;
//			_delay_ms(8);
//		}
	}
	
	// Confirm with host that we're done
	uint8_t ackByte = PREFIXBYTE;
	CDC_Device_SendData(&EETool_CDC_Interface, &ackByte, 1);

	PULSEC(LEDPIN);
}


uint32_t CRC32(uint8_t *buf, size_t size)
{
	// This could be made faster with a table lookup, but it's plenty fast enough for our purposes
	int i, j;
	uint32_t byte, crc, mask;
	
	i = 0;
	crc = 0xFFFFFFFF;
	while (size--)
	{
		byte = buf[i];
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--)
		{
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		i = i + 1;
	}
	return ~crc;
}

// USB Event Handlers
void EVENT_USB_Device_Connect(void)
{
	SETC_HI(LEDPIN);
}

void EVENT_USB_Device_Disconnect(void)
{
	SETC_LO(LEDPIN);
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&EETool_CDC_Interface);
}

void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&EETool_CDC_Interface);
}
