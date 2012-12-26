**ftx-prog** is a Linux command-line alternative to the [FTProg](http://www.ftdichip.com/Support/Utilities.htm#FT_Prog) utility for [FTDI](http://www.ftdichip.com/)'s [FT-X series](http://www.ftdichip.com/FT-X.htm) of USB interfaces.

## Installation

Install some prerequisites

```
sudo apt-get install build-essential gcc make libftdi-dev
```

then download and

```
make
```

Don't forget to plug in your FT-X device!

## Usage

```
sudo ./ftx_prog
```

This should give you full details on all the possible options.

### Displaying Current Settings

```
sudo ./ftx_prog --dump
```

### Inverting RS232 Signals

### CBUS Pins

### I2C

### RS485

### Misc

## Credits

Modified for the FT-X series by Richard Meadows 2012

Based upon [ft232r_prog](http://rtr.ca/ft232r/), Version 1.23, by [Mark Lord](http://rtr.ca/). Copyright 2010-2012.
