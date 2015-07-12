/*
 * This is a Linux command-line alternative to the FTDI MProg/FTProg utilities for FTDI's FT-X series.
 *
 * Modified for the FT-X series by Richard Meadows 2012.
 *
 * Based upon:
 * ft232r_prog.c by Mark Lord.  Copyright 2010-2012.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file LICENSE.txt.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ftdi.h>
#include <stdbool.h>

#define MYVERSION	"0.1"

#define CBUS_COUNT	7

static struct ftdi_context ftdi;
static int verbose = 0;
static bool use_8b_strings = false;
static const char *save_path = NULL, *restore_path = NULL;

/* ------------ Bit Definitions for EEPROM Decoding ------------ */

enum cbus_mode {
	cbus_tristate	=  0,
	cbus_rxled	=  1,
	cbus_txled	=  2,
	cbus_txrxled	=  3,
	cbus_pwren	=  4,
	cbus_sleep	=  5,
	cbus_drive0	=  6,
	cbus_drive1	=  7,
	cbus_gpio	=  8,
	cbus_txden	=  9,
	cbus_clk24	= 10,
	cbus_clk12	= 11,
	cbus_clk6	= 12,
	cbus_bcd_det	= 13,
	cbus_bcd_det_i	= 14,
	cbus_i2c_txe	= 15,
	cbus_i2c_rxf	= 16,
	cbus_vbus_sense	= 17,
	cbus_bitbang_wr	= 18,
	cbus_bitbang_rd	= 19,
	cbus_timestamp	= 20,
	cbus_keep_awake	= 21,
};
enum misc_config {
	bcd_enable		= 0x01,
	force_power_enable	= 0x02,
	deactivate_sleep	= 0x04,
	rs485_echo_suppress	= 0x08,
	ext_osc			= 0x10,
	ext_osc_feedback_en	= 0x20,
	vbus_sense_alloc	= 0x40,
	load_vcp		= 0x80,
};
enum power_config {
	remote_wakeup		= 0x10,
	self_powered		= 0x20,
};
enum dbus_cbus_config {
	dbus_drive_strength	= 0x03,
	dbus_slow_slew		= 0x04,
	dbus_schmitt		= 0x08,
	cbus_drive_strength	= 0x30,
	cbus_slow_slew		= 0x40,
	cbus_schmitt		= 0x80,
};
enum peripheral_config {
	suspend_pull_down	= 0x04,
	serial_number_avail	= 0x08,
	ft1248_cpol		= 0x10,
	ft1248_bord		= 0x20,
	ft1248_flow_control	= 0x40,
	disable_i2c_schmitt	= 0x80,
	invert_txd		= 0x01,
	invert_rxd		= 0x02,
	invert_rts		= 0x04,
	invert_cts		= 0x08,
	invert_dtr		= 0x10,
	invert_dsr		= 0x20,
	invert_dcd		= 0x40,
	invert_ri		= 0x80,
};

/* ------------ Command Line Arguments ------------ */

enum arg_type {
	arg_help,
	arg_dump,
	arg_verbose,
	arg_save,
	arg_restore,
	arg_8b_strings,
	arg_cbus,
	arg_manufacturer,
	arg_product,
	arg_old_serno,
	arg_new_serno,
	arg_max_bus_power,
	arg_suspend_pull_down,
	arg_load_vcp,
	arg_remote_wakeup,
	arg_ft1248_cpol,
	arg_ft1248_bord,
	arg_ft1248_flow_control,
	arg_i2c_schmitt,
	arg_i2c_slave_address,
	arg_i2c_device_id,
	arg_rs485_echo_suppression,
	
	arg_old_vid,
	arg_old_pid,
	arg_new_vid,
	arg_new_pid,
	arg_invert,
	arg_self_powered,
};

/* ------------ Strings for argument parsing ------------ */

static const char* arg_type_strings[] = {
	"--help",
	"--dump",
	"--verbose",
	"--save",
	"--restore",
	"--8bit-strings",
	"--cbus",
	"--manufacturer",
	"--product",
	"--old-serial-number",
	"--new-serial-number",
	"--max-bus-power",
	"--suspend-pull-down",
	"--load-vcp",
	"--remote-wakeup",
	"--ft1248-cpol",
	"--ft1248-bord",
	"--ft1248-flow-control",
	"--i2c-schmitt",
	"--i2c-slave-address",
	"--i2c-device-id",
	"--rs485-echo-supp",
	"--old-vid",
	"--old-pid",
	"--new-vid",
	"--new-pid",
	"--invert",
	"--self-powered",
	NULL
};
static const char* rs232_strings[] = {
	"txd",
	"rxd",
	"rts",
	"cts",
	"dtr",
	"dsr",
	"dcd",
	"ri",
	NULL
};
static const char* cbus_strings[] = {
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	NULL
};
static const char* cbus_mode_strings[] = {
	"Tristate",
	"RxLED",
	"TxLED",
	"TxRxLED",
	"PWREN",
	"SLEEP",
	"Drive_0",
	"Drive_1",
	"GPIO",
	"TXDEN",
	"CLK24MHz",
	"CLK12MHz",
	"CLK6MHz",
	"BCD_Charger",
	"BCD_Charger#",
	"I2C_TXE",
	"I2C_RXF",
	"VBUS_Sense",
	"BitBang_WR",
	"BitBang_RD",
	"Time_Stamp",
	"Keep_Awake",
	NULL
};

static const char *arg_type_help[] = {
	"   				    # (show this help text)",
	"				    # (dump eeprom settings to stdout)",
	"				    # (show debug info and raw eeprom contents)",
	"			 <file>     # (save original eeprom contents to file)",
	"			 <file>     # (restore initial eeprom contents from file)",
	"                      # (byte strings)",
	"[cbus]",
	"		 <string>   # (new USB manufacturer string)",
	"			 <string>   # (new USB product name string)",
	"	 <string>   # (current serial number of device to be reprogrammed)",
	"	 <string>   # (new USB serial number string)",
	"		 <number>   # (max bus current in milli-amperes)",
	"	 [on|off]   # (force I/O pins into logic low state on suspend)",
	"		 [on|off]   # (controls if the VCP drivers are loaded)",
	"		 [on|off]   # (allows the interface to be woken up by something other than USB)",
	"		 [high|low] # (set the clock polarity on the FT1248 interface to active high or active low)",
	"		 [msb|lsb]  # (set the bit order on the FT1248 interface to msb first or lsb first)",
	"	 [on|off]   # (flow control for FT1248 interface)",
	"		 [on|off]   # (schmitt trigger on I2C interface)",
	"	 <number>   # (I2C slave address)",
	"		 <number>   # (I2C device ID)",
	"		 [on|off]   # (enable echo supression on the RS485 bus)",
	"			 <number>   # (current vendor id of device to be reprogrammed, eg. 0x0403)",
	"			 <number>   # (current product id of device to be reprogrammed, eg. 0x6001)",
	"			 <number>   # (new/custom vendor id to be programmed)",
	"			 <number>   # (new/custom product id be programmed)",
	"[invert]",
	"		 [on|off]   # (specify if chip is bus-powered or self-powered)",

};

static const char *bool_strings[] = {
	"False",
	"True",
	"false",
	"true",
	"Off",
	"On",
	"off",
	"on",
	"0",
	"1",
	"No",
	"Yes",
	"no",
	"yes",
	"disable",
	"enable",
	"low",
	"high",
	"msb",
	"lsb",
};

struct eeprom_fields {
	/* Misc Config */
	unsigned char bcd_enable;
	unsigned char force_power_enable;
	unsigned char deactivate_sleep;
	unsigned char rs485_echo_suppress;
	unsigned char ext_osc;
	unsigned char ext_osc_feedback_en;
	unsigned char vbus_sense_alloc;
	unsigned char load_vcp;
	
	/* USB VID/PID */
	unsigned short usb_vid;
	unsigned short usb_pid;
	
	/* USB Release Number */
	unsigned short usb_release_major;
	unsigned short usb_release_minor;
	
	/* Max Power and Config */
	unsigned char remote_wakeup;
	unsigned char self_powered;
	unsigned char max_power; /* Units of 2mA */
	
	/* Device and perhiperal control */
	unsigned char suspend_pull_down;
	unsigned char serial_number_avail;
	unsigned char ft1248_cpol;
	unsigned char ft1248_bord;
	unsigned char ft1248_flow_control;
	unsigned char disable_i2c_schmitt;
	unsigned char invert_txd;
	unsigned char invert_rxd;
	unsigned char invert_rts;
	unsigned char invert_cts;
	unsigned char invert_dtr;
	unsigned char invert_dsr;
	unsigned char invert_dcd;
	unsigned char invert_ri;
	
	/* DBUS & CBUS Control */
	unsigned char dbus_drive_strength;
	unsigned char dbus_slow_slew;
	unsigned char dbus_schmitt;
	unsigned char cbus_drive_strength;
	unsigned char cbus_slow_slew;
	unsigned char cbus_schmitt;
	
	/* Manufacturer, Product and Serial Number string */
	char* manufacturer_string;
	char* product_string;
	char* serial_string;
	
	/* I2C */
	unsigned short i2c_slave_addr;
	unsigned int i2c_device_id;
	
	/* CBUS */
	enum cbus_mode cbus[CBUS_COUNT];
	
	/* Other memory areas */
	unsigned char user_mem[92];			/* user memory space */
	unsigned char factory_config[32];	/* factory configuration values */

	/* These are not actually eeprom values; here for convenience */
	unsigned short		old_vid;
	unsigned short		old_pid;
	const char		*old_serno;
};

/* ------------ libftdi helpers ------------ */

static void do_deinit (void)
{
	ftdi_deinit(&ftdi);
}

static void do_close (void)
{
	ftdi_usb_close(&ftdi);
}

/* ------------ Printing ------------ */

/**
 * Prints a hex dump of a block of memory
 */
static void dumpmem (const char *msg, void *addr, int len)
{
	char *data = addr, hex[3 * 16 + 1], ascii[17];
	unsigned int i, offset = 0;

	if (msg)
		printf("%s:\n", msg);
	for (i = 0; i < len;) {
		unsigned int i16 = i % 16;
		unsigned char c = data[i];
		sprintf(hex + (3 * i16), " %02x", c);
		ascii[i16] = (c < ' ' || c > '~') ? '.' : c;
		if (++i == len || i16 == 15) {
			ascii[i16 + 1] = '\0';
			for (; i16 != 15; ++i16)
				strcat(hex, "   ");
			printf("%04x:%s  %s\n", offset, hex, ascii);
			offset = i;
		}
	}
}
/**
 * A helper that prints either true or false
 */
const char* print_bool(char value)
{
	if (value) return bool_strings[1];
	return bool_strings[0];
}
/**
 * Prints out the current FT-X EEPROM Configuration
 */
static void ee_dump (struct eeprom_fields *ee)
{
	unsigned int c;
	
	/* Misc Config */
	printf("	Battery Charge Detect (BCD) Enabled = %s\n", print_bool(ee->bcd_enable));
	printf("	Force Power Enable Signal on CBUS = %s\n", print_bool(ee->force_power_enable));
	printf("	Deactivate Sleep in Battery Charge Mode = %s\n", print_bool(ee->deactivate_sleep));
	
	printf("	External Oscillator Enabled = %s\n", print_bool(ee->ext_osc));
	printf("	External Oscillator Feedback Resistor Enabled = %s\n", print_bool(ee->ext_osc_feedback_en));
	printf("	CBUS pin allocated to VBUS Sense Mode = %s\n", print_bool(ee->vbus_sense_alloc));
	printf("	Load Virtual COM Port (VCP) Drivers = %s\n", print_bool(ee->load_vcp));
	
	/* USB VID/PID */
	printf("	Vendor ID (VID) = 0x%04x\n", ee->usb_vid);
	printf("	Product ID (PID) = 0x%04x\n", ee->usb_pid);
	
	/* USB Release Number */
	printf("	USB Version = USB%d.%d\n", ee->usb_release_major, ee->usb_release_minor);	
	
	/* Max Power and Config */
	printf("	Remote Wakeup by something other than USB = %s\n", print_bool(ee->remote_wakeup));
	printf("	Self Powered = %s\n", print_bool(ee->self_powered));
	printf("	Maximum Current Supported from USB = %dmA\n", 2 * ee->max_power); /* Units of 2mA */
	
	/* Device and perhiperal control */
	printf("	Pins Pulled Down on USB Suspend = %s\n", print_bool(ee->suspend_pull_down));
	printf("	Indicate USB Serial Number Available = %s\n", print_bool(ee->serial_number_avail));
	
	printf(" FT1248\n");
	printf("-------\n");
	printf("	FT1248 Clock Polarity = %s\n", ee->ft1248_cpol ? "Active High":"Active Low");
	printf("	FT1248 Bit Order = %s\n", ee->ft1248_bord ? "LSB to MSB":"MSB to LSB");
	printf("	FT1248 Flow Control Enabled = %s\n",  print_bool(ee->ft1248_flow_control));

	printf(" RS232\n");
	printf("-------\n");
	printf("	Invert TXD = %s\n", print_bool(ee->invert_txd));
	printf("	Invert RXD = %s\n", print_bool(ee->invert_rxd));
	printf("	Invert RTS = %s\n", print_bool(ee->invert_rts));
	printf("	Invert CTS = %s\n", print_bool(ee->invert_cts));
	printf("	Invert DTR = %s\n", print_bool(ee->invert_dtr));
	printf("	Invert DSR = %s\n", print_bool(ee->invert_dsr));
	printf("	Invert DCD = %s\n", print_bool(ee->invert_dcd));
	printf("	Invert RI = %s\n", print_bool(ee->invert_ri));
	
	printf(" RS485\n");
	printf("-------\n");
	printf("	RS485 Echo Suppression Enabled = %s\n", print_bool(ee->rs485_echo_suppress));
	
	/* DBUS & CBUS Control */
	printf("	DBUS Drive Strength = %dmA\n", 4 * (ee->dbus_drive_strength+1));
	printf("	DBUS Slow Slew Mode = %u\n", ee->dbus_slow_slew);
	printf("	DBUS Schmitt Trigger = %u\n", ee->dbus_schmitt);
	printf("	CBUS Drive Strength = %dmA\n", 4 * (ee->cbus_drive_strength+1));
	printf("	CBUS Slow Slew Mode = %u\n", ee->cbus_slow_slew);
	printf("	CBUS Schmitt Trigger = %u\n", ee->cbus_schmitt);
	
	/* Manufacturer, Product and Serial Number string */
	printf("	Manufacturer = %s\n", ee->manufacturer_string);
	printf("	Product = %s\n", ee->product_string);
	printf("	Serial Number = %s\n", ee->serial_string);
	
	/* I2C */
	printf("  I2C\n");
	printf("-------\n");
	printf("	I2C Slave Address = %d \n", ee->i2c_slave_addr);
	printf("	I2C Device ID = %d \n", ee->i2c_device_id);
	printf("	I2C Schmitt Triggers Disabled = %s\n",  print_bool(ee->disable_i2c_schmitt));

	/* CBUS */
	printf("  CBUS\n");
	printf("-------\n");
	for (c = 0; c < CBUS_COUNT; ++c) {
		printf("	CBUS%u = %s\n", c, cbus_mode_strings[ee->cbus[c]]);
	}
};

/* ------------ Cyclic Redundancy Check ------------ */

static unsigned short calc_crc_ftx (void *addr)
{
	unsigned int i;
	unsigned short crc = 0xaaaa;
	unsigned char* d8 = addr;

	/* Word Addresses 0x0 - 0x11 inclusive */
	for (i = 0; i < 0x12*2; i += 2) {
		crc ^= d8[i] | (d8[i+1] << 8);
		crc  = (crc << 1) | (crc >> 15);
	}
	
	/* Word Addresses 0x12 - 0x39 are ignored */
	
	/* Word Addresses 0x40 - 0x7E inclusive */
	for (i = 0x40*2; i < 0x7F*2; i += 2) {
		crc ^= d8[i] | (d8[i+1] << 8);
		crc  = (crc << 1) | (crc >> 15);
	}
	
	/* Word Address 0x7E is ignored */
	/* Word Address 0x7F is the checksum */
	
	return crc;
}
static unsigned short verify_crc (void *addr, int len)
{
	unsigned short crc    = calc_crc_ftx(addr);
	unsigned char *d8     = addr;
	unsigned short actual = d8[len-2] | (d8[len-1] << 8);

	if (crc != actual) {
		fprintf(stderr, "Bad CRC: crc=0x%04x, actual=0x%04x\n", crc, actual);
		exit(EINVAL);
	}
	if (verbose) printf("CRC: Okay (0x%04x)\n", crc);
	return crc;
}
static unsigned short update_crc (void *addr, int len)
{
	unsigned short crc = calc_crc_ftx(addr);
	unsigned char *d8  = addr;

	d8[len-2] = crc;
	d8[len-1] = crc >> 8;
	return crc;
}

/* ------------ EEPROM Encoding and Decoding ------------ */

/**
 * Checks that the strings aren't too big to fit in the string descriptors memory.
 */
static int ee_check_strings(char* man, char* prod, char* ser)
{
	if (strlen(man) + strlen(prod) + strlen(ser) > 96)	return 1; /* If the strings are too long */
	return 0;
}
/**
 * Inserts a string into a buffer to be written out to the eeprom
 */
static void ee_encode_string(char* str, unsigned char *ptr_field, unsigned char* len_field,
			     unsigned char* eeprom, unsigned char* string_addr)
{
  int original_length = strlen(str), length;

  if (use_8b_strings) {
    length = original_length;
  } else {
    length = strlen(str)*2 + 2;
    char* ftstr = malloc(length);
    
    /* Encode a FT Prog compatible string */
    ftstr[0] = length;
    ftstr[1] = 3;

    int in, out;
    for (in = 0, out = 2; out < length; in++, out += 2) {
      ftstr[out] = str[in];
      ftstr[out+1] = 0;
    }

    str = ftstr;
  }

  /* Copy the strings to the string area */
  memcpy(eeprom + *string_addr, str, length);

  /* Write the the two metadata fields */
  *ptr_field = *string_addr;
  *len_field = length;
  /* Move the string area address forward */
  *string_addr += *len_field;
}
/**
 * Encodes an eeprom_fields object into a buffer ready to be written out to the eeprom
 */
static unsigned short ee_encode (unsigned char *eeprom, int len, struct eeprom_fields *ee)
{	
	int c; unsigned char string_desc_addr = 0xA0;

	memset(eeprom, 0, len);
	
	/* Misc Config */
	if (ee->bcd_enable)			eeprom[0x00] |= bcd_enable;
	if (ee->force_power_enable)		eeprom[0x00] |= force_power_enable;
	if (ee->deactivate_sleep)		eeprom[0x00] |= deactivate_sleep;
	if (ee->rs485_echo_suppress)		eeprom[0x00] |= rs485_echo_suppress;
	if (ee->ext_osc)				eeprom[0x00] |= ext_osc;
	if (ee->ext_osc_feedback_en)		eeprom[0x00] |= ext_osc_feedback_en;
	if (ee->vbus_sense_alloc)		eeprom[0x00] |= vbus_sense_alloc;
	if (ee->load_vcp)			eeprom[0x00] |= load_vcp;
	
	/* USB VID/PID */
	eeprom[0x02] = ee->usb_vid & 0xFF;
	eeprom[0x03] = (ee->usb_vid >> 8) & 0xFF;
	eeprom[0x04] = ee->usb_pid & 0xFF;
	eeprom[0x05] = (ee->usb_pid >> 8) & 0xFF;
	
	/* USB Release Number */
	eeprom[0x07] = ee->usb_release_major;
	eeprom[0x06] = ee->usb_release_minor;
	
	/* Max Power and Config */
	if (ee->remote_wakeup)			eeprom[0x08] |= remote_wakeup;
	if (ee->self_powered)			eeprom[0x08] |= self_powered;
	eeprom[0x08] |= 0x80;	/* This is a reserved bit! */
	eeprom[0x09] = ee->max_power; /* Units of 2mA */
	
	/* Device and perhiperal control */
	if (ee->suspend_pull_down)		eeprom[0x0A] |= suspend_pull_down;
	if (ee->serial_number_avail)		eeprom[0x0A] |= serial_number_avail;
	if (ee->ft1248_cpol)			eeprom[0x0A] |= ft1248_cpol;
	if (ee->ft1248_bord)			eeprom[0x0A] |= ft1248_bord;
	if (ee->ft1248_flow_control)		eeprom[0x0A] |= ft1248_flow_control;
	if (ee->disable_i2c_schmitt)		eeprom[0x0A] |= disable_i2c_schmitt;
	if (ee->invert_txd)			eeprom[0x0B] |= invert_txd;
	if (ee->invert_rxd)			eeprom[0x0B] |= invert_rxd;
	if (ee->invert_rts)			eeprom[0x0B] |= invert_rts;
	if (ee->invert_cts)			eeprom[0x0B] |= invert_cts;
	if (ee->invert_dtr)			eeprom[0x0B] |= invert_dtr;
	if (ee->invert_dsr)			eeprom[0x0B] |= invert_dsr;
	if (ee->invert_dcd)			eeprom[0x0B] |= invert_dcd;
	if (ee->invert_ri)			eeprom[0x0B] |= invert_ri;
	
	/* DBUS & CBUS Control */
	eeprom[0x0C] |= (ee->dbus_drive_strength & dbus_drive_strength);
	if (ee->dbus_slow_slew)			eeprom[0x0C] |= dbus_slow_slew;
	if (ee->dbus_schmitt)			eeprom[0x0C] |= dbus_schmitt;
	eeprom[0x0C] |= (ee->cbus_drive_strength & cbus_drive_strength) << 4;
	if (ee->cbus_slow_slew)			eeprom[0x0C] |= cbus_slow_slew;
	if (ee->cbus_schmitt)			eeprom[0x0C] |= cbus_schmitt;
	
	/* eeprom[0x0D] is unused */
	
	/* Manufacturer, Product and Serial Number string */
	if (ee_check_strings(ee->manufacturer_string, ee->product_string, ee->serial_string)) {
		fprintf(stderr, "Failed to encode, strings too long to fit in string memory area!\n");
		exit(EINVAL);
	}
	ee_encode_string(ee->manufacturer_string, &eeprom[0x0E], &eeprom[0x0F], eeprom, &string_desc_addr);
	ee_encode_string(ee->product_string, &eeprom[0x10], &eeprom[0x11], eeprom, &string_desc_addr);
	ee_encode_string(ee->serial_string, &eeprom[0x12], &eeprom[0x13], eeprom, &string_desc_addr);
	
	/* I2C */
	eeprom[0x14] = ee->i2c_slave_addr & 0xFF;
	eeprom[0x15] = (ee->i2c_slave_addr >> 8) & 0xFF;
	eeprom[0x16] = ee->i2c_device_id & 0xFF;
	eeprom[0x17] = (ee->i2c_device_id >> 8) & 0xFF;
	eeprom[0x18] = (ee->i2c_device_id >> 16) & 0xFF;
	
	/* CBUS */
	for (c = 0; c < CBUS_COUNT; c++) {
		eeprom[0x1A + c] = ee->cbus[c];	
	}
	
	/* User Memory Space */
	memcpy(&eeprom[0x24], ee->user_mem, 92);
	/* Factory Configuration Values */
	memcpy(&eeprom[0x80], ee->factory_config, 32);
	
	return update_crc(eeprom, len);
}
/**
 * Extracts a string from the a buffer read from eeprom
 */
static char* ee_decode_string(unsigned char *eeprom, unsigned char* ptr, unsigned char len)
{
	char* str = malloc(len+1);
	
	if (str != NULL) {
		/* Copy the string from the EEPROM memory */
		memcpy(str, ptr, len);
		
		/* Decode strings written by FT Prog correctly */
		if (use_8b_strings) {
		  str[len] = '\0';
		} else {

		  /* Pick the actual ASCII characters out of the FT Prog encoded string */
		  int in, out;
		  for (in = 2, out = 0; in < len; in += 2, out++) {
		    str[out] = str[in];
		  }

		  str[out] = '\0';
		}
	}
	
	return str;
}
/*
 * Populates an eeprom_fields object from a buffer read from eeprom
 */
static void ee_decode (unsigned char *eeprom, int len, struct eeprom_fields *ee)
{
	int c;
	
	/* Misc Config */
	ee->bcd_enable = (eeprom[0x00] & bcd_enable);
	ee->force_power_enable = (eeprom[0x00] & force_power_enable);
	ee->deactivate_sleep = (eeprom[0x00] & deactivate_sleep);
	ee->rs485_echo_suppress = (eeprom[0x00] & rs485_echo_suppress);
	ee->ext_osc = (eeprom[0x00] & ext_osc);
	ee->ext_osc_feedback_en = (eeprom[0x00] & ext_osc_feedback_en);
	ee->vbus_sense_alloc = (eeprom[0x00] & vbus_sense_alloc);
	ee->load_vcp = (eeprom[0x00] & load_vcp);
	
	/* USB VID/PID */
	ee->usb_vid = eeprom[0x02] | (eeprom[0x03] << 8);
	ee->usb_pid = eeprom[0x04] | (eeprom[0x05] << 8);
	
	/* USB Release Number */
	ee->usb_release_major = eeprom[0x07];
	ee->usb_release_minor = eeprom[0x06];
	
	/* Max Power and Config */
	ee->remote_wakeup = (eeprom[0x08] & remote_wakeup);
	ee->self_powered = (eeprom[0x08] & self_powered);
	ee->max_power = eeprom[0x09]; /* Units of 2mA */
	
	/* Device and perhiperal control */
	ee->suspend_pull_down = (eeprom[0x0A] & suspend_pull_down);
	ee->serial_number_avail = (eeprom[0x0A] & serial_number_avail);
	ee->ft1248_cpol = (eeprom[0x0A] & ft1248_cpol);
	ee->ft1248_bord = (eeprom[0x0A] & ft1248_bord);
	ee->ft1248_flow_control = (eeprom[0x0A] & ft1248_flow_control);
	ee->disable_i2c_schmitt = (eeprom[0x0A] & disable_i2c_schmitt);
	ee->invert_txd = (eeprom[0x0B] & invert_txd);
	ee->invert_rxd = (eeprom[0x0B] & invert_rxd);
	ee->invert_rts = (eeprom[0x0B] & invert_rts);
	ee->invert_cts = (eeprom[0x0B] & invert_cts);
	ee->invert_dtr = (eeprom[0x0B] & invert_dtr);
	ee->invert_dsr = (eeprom[0x0B] & invert_dsr);
	ee->invert_dcd = (eeprom[0x0B] & invert_dcd);
	ee->invert_ri = (eeprom[0x0B] & invert_ri);
	
	/* DBUS & CBUS Control */
	ee->dbus_drive_strength = (eeprom[0x0C] & dbus_drive_strength);
	ee->dbus_slow_slew = (eeprom[0x0C] & dbus_slow_slew);
	ee->dbus_schmitt = (eeprom[0x0C] & dbus_schmitt);
	ee->cbus_drive_strength = (eeprom[0x0C] & cbus_drive_strength) >> 4;
	ee->cbus_slow_slew = (eeprom[0x0C] & cbus_slow_slew);
	ee->cbus_schmitt = (eeprom[0x0C] & cbus_schmitt);
	
	/* eeprom[0x0D] is unused */
	
	/* Manufacturer, Product and Serial Number string */
	ee->manufacturer_string = ee_decode_string(eeprom, eeprom+eeprom[0x0E], eeprom[0x0F]);
	ee->product_string = ee_decode_string(eeprom, eeprom+eeprom[0x10], eeprom[0x11]);
	ee->serial_string = ee_decode_string(eeprom, eeprom+eeprom[0x12], eeprom[0x13]);
	
	/* I2C */
	ee->i2c_slave_addr = eeprom[0x14] | (eeprom[0x15] << 8);
	ee->i2c_device_id = eeprom[0x16] | (eeprom[0x17] << 8) | (eeprom[0x18] << 16);
	
	/* CBUS */
	for (c = 0; c < CBUS_COUNT; c++) {
		ee->cbus[c] = eeprom[0x1A + c];	
	}
	
	/* User Memory Space */
	memcpy(ee->user_mem, &eeprom[0x24], 92);
	/* Factory Configuration Values */
	memcpy(ee->factory_config, &eeprom[0x80], 32);
}

/* ------------ Help ------------ */

static const char *myname;

/*
 * Prints a human-readable expression showing all the possibilities for an option
 */
static void print_options(FILE *fp, const char** options)
{
	int j;
	
	fprintf(fp, "  [");
	for (j = 0; options[j];) {
		fprintf(fp, "%s", options[j]);
		if (options[++j])
			fprintf(fp, "|");
	}
	fprintf(fp, "]");
}
/*
 * Prints a human readable help text.
 */
static void show_help (FILE *fp)
{
	int i;

	fprintf(fp, "\nUsage:  %s [<arg> <val>]..\n", myname);
	fprintf(fp, "\nwhere <arg> must be any of:\n");

	for (i = 0; arg_type_strings[i]; ++i) { /* For each argument */
		/* Get the help string */
		const char *val = arg_type_help[i];
		/* Print its name */
		fprintf(fp, "    %s", arg_type_strings[i]);
		
		if (val) { /* If there is a help string */
			if (strcmp(val, "[cbus]") == 0) {
				fprintf(fp, "  [1..%d]", CBUS_COUNT);
				print_options(fp, cbus_mode_strings);
			} else if (strcmp(val, "[invert]") == 0) {
				print_options(fp, rs232_strings);
			} else {
				fprintf(fp, "  %s", val);
			}
		}
		fputc('\n', fp);
	}
	fputc('\n', fp);
}

/* ------------ EEPROM Reading and Writing ------------ */

static int ee_prepare_write(void)
{
	unsigned short status;
	int ret;

	/* These commands were traced while running MProg */
	if ((ret = ftdi_usb_reset(&ftdi)) != 0)						return ret;
	if ((ret = ftdi_poll_modem_status(&ftdi, &status)) != 0)		return ret;
	if ((ret = ftdi_set_latency_timer(&ftdi, 0x77)) != 0)		return ret;
	
	return 0;
}
static int ee_write(unsigned char *eeprom, int len)
{
	int i;

	if (ee_prepare_write()) {
		fprintf(stderr, "ee_prepare_write() failed: %s\n", ftdi_get_error_string(&ftdi));
		exit(EIO);
	}
		
	for (i = 0; i < len/2; i++) {
		if (ftdi_write_eeprom_location(&ftdi, i, eeprom[i*2] | (eeprom[(i*2)+1] << 8))) {
			fprintf(stderr, "ftdi_write_eeprom_location() failed: %s\n", ftdi_get_error_string(&ftdi));
			exit(EIO);
		}
	}

	return 0;
}

static unsigned short ee_read_and_verify (void *eeprom, int len)
{
	int i;

	for (i = 0; i < len/2; i++) {
		if (ftdi_read_eeprom_location(&ftdi, i, eeprom+(i*2))) {
			fprintf(stderr, "ftdi_read_eeprom_location() failed: %s\n", ftdi_get_error_string(&ftdi));
			exit(EIO);
		}
	}
	
	return verify_crc(eeprom, len);
}

/* ------------ Parsing Command Line ------------ */

static int match_arg (const char *arg, const char **possibles)
{
	int i;

	for (i = 0; possibles[i]; ++i) {
		if (0 == strcasecmp(possibles[i], arg))
			return i;
	}
	fprintf(stderr, "unrecognized arg: \"%s\"\n", arg);
	exit(EINVAL);
	return -1;  /* never reached */
}
static unsigned long unsigned_val (const char *arg, unsigned long max)
{
	unsigned long val;

	errno = 0;
	val = strtoul(arg, NULL, 0);
	if (errno || val > max) {
		fprintf(stderr, "%s: bad value (max=0x%lx)\n", arg, max);
		exit(EINVAL);
	}
	return val;
}
static void process_args (int argc, char *argv[], struct eeprom_fields *ee)
{
  int i; int c;

  for (i = 1; i < argc;) {
    int arg;
    arg = match_arg(argv[i++], arg_type_strings);
    switch (arg) {
      case arg_help:
	show_help(stdout);
	exit(1);
      case arg_dump:
	break;
      case arg_verbose:
	verbose = 1;
	break;
	/* File operations */
      case arg_save:
	save_path = argv[i++];
	break;
      case arg_restore:
	restore_path = argv[i++];
	break;
      case arg_8b_strings:
	use_8b_strings = true;
	break;
      case arg_cbus:
	c = match_arg(argv[i++], cbus_strings);
	ee->cbus[c] = match_arg(argv[i++], cbus_mode_strings);
	break;
      case arg_invert:
	switch(match_arg(argv[i++], rs232_strings)) {
	  case 0:	ee->invert_txd = !ee->invert_txd; break;
	  case 1:	ee->invert_rxd = !ee->invert_rxd; break;
	  case 2:	ee->invert_rts = !ee->invert_rts; break;
	  case 3:	ee->invert_cts = !ee->invert_cts; break;
	  case 4:	ee->invert_dtr = !ee->invert_dtr; break;
	  case 5:	ee->invert_dsr = !ee->invert_dsr; break;
	  case 6:	ee->invert_dcd = !ee->invert_dcd; break;
	  case 7:	ee->invert_ri = !ee->invert_ri; break;
	}
	break;
	/* Strings */
      case arg_manufacturer:
	ee->manufacturer_string = argv[i++];
	break;
      case arg_product:
	ee->product_string = argv[i++];
	break;
      case arg_new_serno:
	ee->serial_string = argv[i++];
	break;
	
      case arg_max_bus_power:
	ee->max_power = unsigned_val(argv[i++], 0x1ff) / 2;
	break;
      case arg_self_powered:
        ee->self_powered = match_arg(argv[i++], bool_strings) & 1;
        break;
      case arg_suspend_pull_down:
	ee->suspend_pull_down = match_arg(argv[i++], bool_strings) & 1;
	break;
      case arg_load_vcp:
	ee->load_vcp = match_arg(argv[i++], bool_strings) & 1;
	break;
      case arg_remote_wakeup:
	ee->remote_wakeup = match_arg(argv[i++], bool_strings) & 1;
	break;
	/* FT1248 */
      case arg_ft1248_cpol:
	ee->ft1248_cpol = match_arg(argv[i++], bool_strings) & 1;
	break;
      case arg_ft1248_bord:
	ee->ft1248_bord = match_arg(argv[i++], bool_strings) & 1;
	break;
      case arg_ft1248_flow_control:
	ee->ft1248_flow_control = match_arg(argv[i++], bool_strings) & 1;
	break;
      case arg_i2c_schmitt: /* The command line arg is enabled +ve, the eeprom is disabled +ve */
	ee->disable_i2c_schmitt = !(match_arg(argv[i++], bool_strings) & 1);
	break;
	/* I2C */
      case arg_i2c_slave_address:
	ee->i2c_slave_addr = unsigned_val(argv[i++], 0xffff);
	break;
      case arg_i2c_device_id:
	ee->i2c_device_id = unsigned_val(argv[i++], 0xffff);
	break;
	/* RS485 */
      case arg_rs485_echo_suppression:
	ee->rs485_echo_suppress = match_arg(argv[i++], bool_strings) & 1;
	break;
	/* VID, PID, Ser No. */
      case arg_old_vid:
	ee->old_vid = unsigned_val(argv[i++], 0xffff);
	break;
      case arg_old_pid:
	ee->old_pid = unsigned_val(argv[i++], 0xffff);
	break;
      case arg_old_serno:
	ee->old_serno = argv[i++];
	break;
      case arg_new_vid:
	ee->usb_vid = unsigned_val(argv[i++], 0xffff);
	break;
      case arg_new_pid:
	ee->usb_pid = unsigned_val(argv[i++], 0xffff);
	break;
    }
  }
}

/* ------------ File Save / Restore ------------ */

static void save_eeprom_to_file (const char *path, void *eeprom, int len)
{
	int count, fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0644);

	if (fd == -1) {
		int err = errno;
		perror(path);
		exit(err);
	}
	count = write(fd, eeprom, len);
	if (count < 0) {
		int err = errno;
		perror(path);
		exit(err);
	}
	close(fd);
	if (count != len) {
		fprintf(stderr, "%s: wrong size, wrote %d/%d bytes\n", path, count, len);
		exit(EINVAL);
	}
	printf("%s: wrote %d bytes\n", path, count);
}

static void restore_eeprom_from_file (const char *path, void *eeprom, int len, int max)
{
	int count, fd = open(path, O_RDONLY);

	if (fd == -1) {
		int err = errno;
		perror(path);
		exit(err);
	}
	count = read(fd, eeprom, max);
	if (count < 0) {
		int err = errno;
		perror(path);
		exit(err);
	}
	close(fd);
	if (count != len ) {
		fprintf(stderr, "%s: wrong size, read %d/%d bytes\n", path, count, len);
		exit(EINVAL);
	}
	printf("%s: read %d bytes\n", path, count);
	verify_crc(eeprom, len);
}

/* ------------ Main ------------ */

int main (int argc, char *argv[])
{
	const char *slash;
	unsigned char old[0x100] = {0,}, new[0x100] = {0,};
	unsigned short new_crc;
	struct eeprom_fields ee;
	unsigned int len = 0x100; /* We only deal with the first 256 bytes and ignore the user memory space */

	myname = argv[0];
	slash = strrchr(myname, '/');
	if (slash)
		myname = slash + 1;
	
	printf("\n%s: version %s\n", myname, MYVERSION);
	printf("Modified for the FT-X series by Richard Meadows\n\n");
	printf("Based upon:\n");
	printf("ft232r_prog: version 1.23, by Mark Lord.\n");
	if (argc < 2) {
		show_help(stdout);
		exit(0);
	}

	ftdi_init(&ftdi);
	atexit(&do_deinit);

	memset(&ee, 0, sizeof(ee));
	ee.old_vid = 0x0403;	/* default; override with --old_vid arg */
	ee.old_pid = 0x6015;	/* default; override with --old_pid arg */
	process_args(argc, argv, &ee);	/* handle --help and --old-* args */

	if (ftdi_usb_open_desc(&ftdi, ee.old_vid, ee.old_pid, NULL, ee.old_serno)) {
		fprintf(stderr, "ftdi_usb_open() failed for %04x:%04x:%s %s\n",
			ee.old_vid, ee.old_pid, ee.old_serno ? ee.old_serno : "", ftdi_get_error_string(&ftdi));
		exit(ENODEV);
	}
	atexit(&do_close);
	
	/* First, read the original eeprom from the device */
	(void) ee_read_and_verify(old, len);
	if (verbose) dumpmem("existing eeprom", old, len);

	/* Save old contents to a file, if requested (--save) */
	if (save_path)
		save_eeprom_to_file(save_path, old, len);

	/* Restore contents from a file, if requested (--restore) */
	if (restore_path) {
		restore_eeprom_from_file(restore_path, new, len, sizeof(new));
		if (verbose) dumpmem(restore_path, new, len);
	}
	
	/* TODO: It'd be nice to check we can restore the EEPROM.. */
	
	/* Decode eeprom contents into ee struct */
	ee_decode(old, len, &ee);
	
	/* process args, and dump new settings */
	process_args(argc, argv, &ee);	/* Handle value-change args */
	ee_dump(&ee);

	/* Build new eeprom image */
	new_crc = ee_encode(new, len, &ee);

	/* If different from original, then write it back to the device */
	if (0 == memcmp(old, new, len)) {
		printf("No change from existing eeprom contents.\n");
	} else {
		if (verbose) dumpmem("new eeprom", new, len);
		
		printf("Rewriting eeprom with new contents.\n");
		ee_write(new, len);
		
		/* Read it back again, and check for differences */
		if (ee_read_and_verify(new, len) != new_crc) {
			fprintf(stderr, "Readback test failed, results may be botched\n");
			exit(EINVAL);
		}
		ftdi_usb_reset(&ftdi);  /* Reset the device to force it to load the new settings */
	}
	
	exit(EINVAL);
	return 0;  /* never reached */
}
