*A minimal FM demodulator*

This is a quick and dirty FM receiver.

The goal was to do close to the minimum to demodulate an FM signal.
No fancy filtering, just box filters.
No weird code architecture.
Just one thread handling a callback for each block of raw binary data.
It does use some small classes that act as transducers with internal state.
This makes it possible to have convolutions that straddle the divide between
blocks of input data.

Tested on MacOSX and raspberry-pi.

Missing:
1. Stereo
2. De-emphasis (which is why there is too much treble)
3. RBDS/RDS
4. Error checking

*Building*

You'll need to have installed rtl-sdr

See: http://sdr.osmocom.org/trac/wiki/rtl-sdr

Type make

Edit Makefile to use -DFAST_ATAN with g++ if your system is too slow.
It runs fast enough on a raspberry-pi without doing this.

*Running*

Outputs binary audio as single channel signed 16 bit samples at 50KHz.

Under Linux run with a command like this to listen to 97.3 MHz:

./mini_fm 97300000 | aplay -r 50k -f S16_LE

(You may need to use sudo.)

Under MacOSX run with a command like this to listen to 97.3 MHz:

./mini_fm 97300000 | play -r 50000 -t s16 -L -c 1  -
