A minimal FM demodulator
========================

Status
------
This is a complete working project. The goal was to write a minimal working FM receiver rather than a good one. So I have no plans to add features.

This is a quick and dirty FM receiver using the rtlsdr dongle you can buy from Amazon for about $10.

My goal was to do close to the minimum to demodulate an FM signal.
That should make it easier for anyone to read the code and figure it how it works.
No weird code architecture.
Just one thread handling a callback for each block of raw binary data.
It does use some small classes that act as transducers with a little bit of internal state.
This makes it possible to have convolutions that straddle the divide between
blocks of input data.

Tested on MacOSX and raspberry-pi.

Missing:
1. Stereo
2. RBDS/RDS
3. Error checking

(I switched to using a Chebyshev filter as it produces much better results. I also added de-emphasis code as it's almost free once you've written IIR code to make the Chebyshev filter worked.)

Building
--------

You'll need to have installed rtl-sdr

See: http://sdr.osmocom.org/trac/wiki/rtl-sdr

Type `make`

Edit Makefile to use `-DFAST_ATAN` with g++ if your system is too slow.
It runs fast enough on a raspberry-pi without doing this.

Running
-------

Outputs binary audio as single channel signed 16 bit samples at 40KHz.

Under Linux run with a command like this to listen to 97.3 MHz:

`./mini_fm 97300000 | aplay -r 40k -f S16_LE`

(You may need to use sudo.)

Under MacOSX run with a command like this to listen to 97.3 MHz:

`./mini_fm 97300000 | play -r 40000 -t s16 -L -c 1  -`

(You may need to install sox: http://sox.sourceforge.net/)
