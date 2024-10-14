**ftx-prog** is a Linux command-line alternative to the
[FTProg](http://www.ftdichip.com/Support/Utilities.htm#FT_Prog)
utility for [FTDI](http://www.ftdichip.com/)'s
[FT-X series](http://www.ftdichip.com/FT-X.htm) of USB interfaces.

## Installation

Install some prerequisites

```
sudo apt-get install build-essential gcc make libftdi-dev libusb-1.0-0-dev
```

then
[download](https://github.com/richardeoin/ftx-prog/archive/master.zip),
unzip, open a terminal in the unzipped directory and

```
make
```

Don't forget to plug in your FT-X device!

## Usage

```
sudo ./ftx_prog
```

This should give you full details on all the possible options.

### Display Current Settings

```
sudo ./ftx_prog --dump
```

### CBUS Pins

```
sudo ./ftx_prog --cbus [cbus pin number] [function]
```

Sets up configurable CBUS pins with a particular function. Have a look
at `--help` to see all the possible functions, and §7.12 of
[this application note](http://www.ftdichip.com/Support/Documents/AppNotes/AN_201_FT-X%20MTP%20Memory%20Configuration.pdf)
to see what they all do. Don't forget that the pin numbers are zero
based, so `--cbus 0` corresponds to the `CBUS0` pin.

The most commonly used are:

* `RxLED` *(pulses low when data is being sent from the host to the USB device)*
* `TxLED` *(pulses low when data is being sent from the USB device to the host)*
* `TxRxLED` *(pulses low when data is being sent either way)*
* `SLEEP` *(goes low when the interface is in USB suspend mode)*

### Inverting RS232 Signals
`FT230X` and `FT231X` only

```
sudo ./ftx_prog --invert [rs232 pin]
```

This will invert the polarity of this pin on the chip. So if it was
Active High it'll become Active Low and vice versa. You can use
`--dump` to see if the pin is in its default state or not.

The possible pins are:
* `txd` *The interface's data output pin*
* `rxd` *The interface's data input pin*
* `rts` *Request to Send output pin*
* `cts` *Clear to Send input pin*
* `dtr` *Data Terminal Ready output pin*
* `dsr` *Data Set Ready input pin*
* `dcd` *Data Carrier Detect input pin*
* `ri` *Ring Indicator input pin*

Taking `RI` low (or high when polarity is inverted) for > 20ms will
resume the USB host controller from suspend if remote wakeup has been
enabled using `--remote-wakeup on`.

### I2C
`FT200XD` and `FT201X` only

*Note: The functionality of these options is completely untested*

```
sudo ./ftx_prog --i2c-slave-address <number>
```

Sets the slave address of the FT-X on the I2C bus. For instance,
`--i2c-slave-address 0x22` will restore the slave address to its
default value.

```
sudo ./ftx_prog --i2c-schmitt [on|off]
```

The I2C pins have schmitt triggers which can be turned on or off.


```
sudo ./ftx_prog --i2c-device-id <number>
```

Sets the I2C device ID.

### SPI / FT1248
`FT220X` and `FT221X` only

*Note: The functionality of these options is completely untested*

```
sudo ./ftx_prog --ft1248-cpol [high|low]
```

The clock on the SPI / FT1248 bus can either be active `high` (Like
SPI `Mode 1`) or active `low` (Like SPI `Mode 3`).

```
sudo ./ftx_prog --ft1248-bord [msb|lsb]
```

The bit order on the SPI / FT1248 bus can be either `msb` first or `lsb` first.

```
sudo ./ftx_prog --ft1248-flow-control [on|off]
```

Turns on flow control when SS_n is inactive.

### RS485

*Note: The functionality of these options is completely untested*

```
sudo ./ftx_prog --rs485-echo-supp [on|off]
```

Used to enable echo supression if the interface is being used in a
RS-485 system.

### Misc

```
sudo ./ftx_prog --load-vcp [on|off]
```

Controls if the Virtual COM Port (VCP) drivers are loaded.

```
sudo ./ftx_prog --remote-wakeup [on|off]
```

Allows the interface to be woken up by something other than USB.

Use `sudo ./ftx_prog --help` to see details of all the command line options.

*There are other configuration options that have not yet been
 implemented in the user interface. Support for the user configurable
 area in the EEPROM is also a possibility.*

## Workarounds for FT-X devices

### All

Due to an error in the Silicon Revisions A, B and C of the FT-X series:

> The device is put into suspend mode during a transfer of certain
> data patterns most notable with binary zeros. This can halt the data
> transfer in certain circumstances and will require the device to be
> reenumerated to recover.  NB. It is the presence of this data
> pattern on the USB bus regardless of whether the data is intended
> for the [FT-X] or other devices (e.g. a broadcast) on the bus that
> forces the suspend state.

This can be fixed by connecting any `CBUS` pin to ground on the PCB
and then configuring it as `KeepAwake#`. If you were to choose `CBUS0`
then `sudo ./ftx_prog --cbus 0 Keep_Awake` will do the configuration.

### `FT230X` and `FT231X`

In Silicon Revisons A and B of the `FT230X` and `FT231X`:

> An incorrect value for the VCP driver was programmed into the MTP on some production runs.

This means the Virtual COM Port (VCP) drivers are disabled, preventing
the device from appearing as a COM Port on windows. *Note: This is
untested and may not be true!*

To enable the VCP drivers, use `sudo ./ftx_prog --load-vcp true`.

## License

GPL v2

## Contributing

Bug reports and pull requests welcome!

This project is passively maintained, so there may be no changes for a long
time. PRs are more likely to get merged if they are well explained, well
commented and match the style of the surrounding code.

## Credits

Modified for the FT-X series by Richard Meadows 2012

Based upon [ft232r_prog](http://rtr.ca/ft232r/), Version 1.23, by
[Mark Lord](http://rtr.ca/). Copyright 2010-2012.
