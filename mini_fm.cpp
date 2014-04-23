#include <rtl-sdr.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <complex>
#include <algorithm>

//
// After a sample is input, the output is
// the sum of all inputs so far.
//
template<class Sum, class Element>
class RunningSum {
public:
    RunningSum() : out(0) { }
    Sum out;
    void in(Element e) {
        out += Sum(e);
    }
};

//
// Circular buffer of size 10.
// The output lags behind the input by 10 samples.
//
template<class Element>
class Delay {
    int i;
    int n;
    Element *buffer;
public:
    Element out;
    Delay(int n0) : i(0), n(n0) {
        buffer = new Element[n];
        std::fill(buffer, buffer+n, 0);
    }
    void in(Element e) {
        out = buffer[i];
        buffer[i++] = e;
        if (i >= n) {
            i = 0;
        }
    }
};

//
// Box filter smoothing.
// The output is the average of the last n inputs.
// This is achieved by subtracting the running sum
// n samples ago from the current running sum and
// dividing by n.
//
// Filtering cuts down on noise but more importantly it's what
// allows us to pick up just one frequency band. The filter width
// was chosen to cut out the subcarriers with stereo, RDS etc.
//
template<class Sum, class Element>
class Smooth {
    int n;
    RunningSum<Sum, Element> runningSum;
    Delay<Sum> delay;
public:
    Smooth(int n0) : n(n0), delay(n0) { }
    Element out;
    void in(Element e) {
        runningSum.in(e);
        delay.in(runningSum.out);
        out = (runningSum.out-delay.out)/n;
    }
};

using std::complex;

float to_float(unsigned char x) {
    return (1.0f/127.0f)*(float(x)-127.0f);
}

//
// In AM the absolute value of the complex valued signal is the message.
// In FM the rate of change of the phase of the complex valued signal is the message.
//
class PhaseChange {
    complex<float> last;
public:
    PhaseChange () : last(1.0) { }
    float out;
    void in(unsigned int i, unsigned int q) {
        complex<float> current(to_float(i), to_float(q));
        complex<float> xy = conj(last)*current;
#ifdef FAST_ATAN
	float x = xy.real();
	float y = xy.imag();
	if (x>=0 && y>=0) {
	   out = 2*y/(x+y);
        } else if (x<=0 && y>=0) {
	   out = 3-(x+y)/(y-x);
	} else if (x<=0 && y<=0) {
	   out = -3-(x-y)/(x+y);
	} else {
	   out = 2*y/(x-y);
        }
#else
	out = arg(xy);
#endif
        last = current;
    }
};

//
// Resample radio data to audio frequencies.
//
class DownSampler {
    int n;
    int i;
    float sum;
public:
    float out;
    bool ready;
    DownSampler(int n0) : n(n0), i(0), sum(0.0f), ready(false) { }
    void in(float x) {
        sum += x;
        ++i;
        if (i >= n) {
            i = 0;
            ready = true;
            out = (1.0f/n)*sum;
            sum = 0.0f;
        } else {
            ready = false;
        }
    }
};

class Context {
public:
    Smooth<unsigned int, unsigned char> i_smooth;
    Smooth<unsigned int, unsigned char> q_smooth;
    PhaseChange phi;
    DownSampler down_sampler;

    Context(int n, int m) : i_smooth(n), q_smooth(n), down_sampler(m) { }

    void process(unsigned char *buf, uint32_t len) {
        signed short buffer[len];
        int j = 0;
        for (int i = 0; i < len; i += 2) {
            i_smooth.in(buf[i]);
            q_smooth.in(buf[i+1]);
            phi.in(i_smooth.out, q_smooth.out);
            down_sampler.in(phi.out);
            if (down_sampler.ready) {
                buffer[j] = (signed short)(10000.0f*down_sampler.out);
                ++j;
            }
        }
        write(1, buffer, j*sizeof(signed short));
    }
};

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {
    Context *context = (Context *)ctx;
    context->process(buf, len);
}

int main(int argc, char **argv) {
    rtlsdr_dev_t *dev;
    int r = rtlsdr_open(&dev, 0);
    if (r < 0) {
        fprintf(stderr, "Failed to open rtlsdr device msg #%d.\n", r);
        exit(1);
    }
    rtlsdr_set_tuner_gain_mode(dev, 0);
    rtlsdr_reset_buffer(dev);
    uint32_t frequency;
    sscanf(argv[1], "%d", &frequency);
    rtlsdr_set_center_freq(dev, frequency);
    //
    // Sampling at 1 MHz
    //
    rtlsdr_set_sample_rate(dev, 1000000);

    //
    // Subsampling audio with a ratio of 20. That takes us
    // from 1MHz to 50KHz.
    //
    Context *context = new Context(10, 20);
    rtlsdr_read_async(dev, rtlsdr_callback, context, 0, 0);

    return 0;
}
