#include <algorithm>
#include <complex>
// #include <iostream>
#include <rtl-sdr.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Note RTL-SDR has max bandwidth of 2.4MHz
float RFSamplingRate = 1000000;
float AudioSamplingRate = 40000;

template <class Element> class IIR {
  int n;
  Element *a;
  Element *b;
  Element *x;
  Element *y;

public:
  IIR(int n0, float *b0, float *a0) : n(n0), b(b0), a(a0) {
    x = new float[n];
    y = new float[n];
    for (int i = 0; i < n; ++i) {
      x[i] = 0;
      y[i] = 0;
    }
  }
  float out;

  void in(Element x_new) {
    float y_new(0);
    for (int i = n - 1; i >= 1; --i) {
      x[i] = x[i - 1];
      y[i] = y[i - 1];
    }
    x[0] = x_new;
    for (int i = 0; i < n; ++i) {
      y_new += b[i] * x[i];
      if (i > 0) {
        y_new -= a[i] * y[i];
      }
    }
    y_new = y_new / a[0];
    y[0] = y_new;

    out = y_new;
  }
};

using std::complex;

float to_float(unsigned char x) {
  return (1.0f / 127.0f) * (float(x) - 127.0f);
}

//
// In AM the absolute value of the complex valued signal is the message.
// In FM the rate of change of the phase of the complex valued signal is the
// message.
//
class PhaseChange {
  complex<float> last;

public:
  PhaseChange() : last(1.0) {}
  float out;
  void in(float i, float q) {
    complex<float> current(to_float(i), to_float(q));
    complex<float> xy = conj(last) * current;
    out = arg(xy);
    last = current;
  }
};

//
// Resample radio data to audio frequencies.
// Simple box filter.
//
class DownSampler {
  int n;
  int i;
  float sum;

public:
  float out;
  bool ready;
  DownSampler(int n0) : n(n0), i(0), sum(0.0f), ready(false) {}
  void in(float x) {
    sum += x;
    ++i;
    if (i >= n) {
      i = 0;
      ready = true;
      out = (1.0f / n) * sum;
      sum = 0.0f;
    } else {
      ready = false;
    }
  }
};

//
//        +------+   +-----------+
// I -->--+Lopass+->-+           |   +-------------+
//        +------+   |           |   |             |
//                   |PhaseChange+->-+ DownSampler +-->-- audio
//        +------+   |           |   |             |
// Q -->--+Lopass+->-+           |   +-------------+
//        +------+   +-----------+
//

// A number of filter coefficients suitable for use with IIR operator

// RF Low pass Chebshev filter
// This is the filter that chops out the appropriate region
// of the RF spectrum so FM demodulation can be applied.
// TBH I don't know why a choice of 0.28 * f_Nyquist = 140KHz for
// the cutoff works well.
int size = 8;

// Allow user to choose for now but there is probably a correct choice
// of filter and I'll cut down to just that some time...
// Generated using Python/scipy via
//   for cutoff in [0.12, 0.16, 0.20, 0.24, 0.28, 0.32]:
//     b, a = signal.cheby1(size - 1, 5, cutoff, 'low', analog=False)
//     ...
float b12[] = {8.492114059488941e-08,  5.944479841642259e-07,
               1.7833439524926777e-06, 2.9722399208211295e-06,
               2.9722399208211295e-06, 1.7833439524926777e-06,
               5.944479841642259e-07,  8.492114059488941e-08};
float a12[] = {1.0,
               -6.6015555448213705,
               18.91281595952558,
               -30.47223962602268,
               29.81468894444905,
               -17.712906107457083,
               5.916338625732527,
               -0.8571313815000202};
float b16[] = {6.301994026662001e-07,  4.4113958186634e-06,
               1.3234187455990201e-05, 2.2056979093317e-05,
               2.2056979093317e-05,    1.3234187455990201e-05,
               4.4113958186634e-06,    6.301994026662001e-07};
float a16[] = {1.0,
               -6.363361691384967,
               17.756829521113268,
               -28.141363953764607,
               27.340958451383695,
               -16.280469346107775,
               5.501716397188874,
               -0.8142287129049438};
float b20[] = {2.990828191005441e-06,  2.0935797337038088e-05,
               6.280739201111427e-05,  0.00010467898668519044,
               0.00010467898668519044, 6.280739201111427e-05,
               2.0935797337038088e-05, 2.990828191005441e-06};
float a20[] = {1.0,
               -6.07483805141812,
               16.41656128883667,
               -25.52491209732976,
               24.630249352059007,
               -14.74326141363145,
               5.070081758174261,
               -0.7734980106821583};
float b24[] = {1.071724187524014e-05,  7.502069312668098e-05,
               0.00022506207938004295, 0.00037510346563340494,
               0.00037510346563340494, 0.00022506207938004295,
               7.502069312668098e-05,  1.071724187524014e-05};
float a24[] = {1.0,
               -5.737642995855332,
               14.926489154906347,
               -22.71025721285745,
               21.771461734270137,
               -13.140427313140748,
               4.626581515967827,
               -0.7348330763307507};
float b28[] = {3.1684431544144464e-05, 0.00022179102080901124,
               0.0006653730624270337,  0.0011089551040450562,
               0.0011089551040450562,  0.0006653730624270337,
               0.00022179102080901124, 3.1684431544144464e-05};
float a28[] = {1.0,
               -5.353575029646652,
               13.325143703781212,
               -19.788853613813018,
               18.854242732626513,
               -11.510704168004956,
               4.175933945715462,
               -0.6981319634209104};
float b32[] = {8.148482093572128e-05, 0.000570393746550049,
               0.001711181239650147,  0.002851968732750245,
               0.002851968732750245,  0.001711181239650147,
               0.000570393746550049,  8.148482093572128e-05};
float a32[] = {1.0,
               -4.9245582287877445,
               11.654681376981499,
               -16.852949536401415,
               15.965857312056182,
               -9.891745436943115,
               3.722441430521129,
               -0.6632968603467627};

// Audio frequency de-emphasis filter
// In EU the time constant is 0.000050
float DeEmphasisTimeConstant = 0.000075;
float d = exp(-1. / (DeEmphasisTimeConstant * AudioSamplingRate));
float b1[] = {1 - d, 0.};
float a1[] = {1., -d};

class FMDemodulator {
public:
  IIR<float> i_smooth;
  IIR<float> q_smooth;
  PhaseChange phi;
  DownSampler down_sampler;
  IIR<float> DeEmphasis;

  FMDemodulator(int n, int DownSamplingRate, float *b, float *a)
      : i_smooth(size, b, a), q_smooth(size, b, a),
        down_sampler(DownSamplingRate), DeEmphasis(2, b1, a1) { }

  void in(unsigned char *buf, uint32_t len) {
    signed short buffer[len];
    int j = 0;
    for (int i = 0; i < len; i += 2) {
      i_smooth.in(buf[i]);
      q_smooth.in(buf[i + 1]);
      phi.in(i_smooth.out, q_smooth.out);
      down_sampler.in(phi.out);
      if (down_sampler.ready) {
        float Out = down_sampler.out;
        DeEmphasis.in(Out);
        buffer[j] = (signed short)(10000.0f * DeEmphasis.out);
        ++j;
      }
    }
    write(1, buffer, j * sizeof(signed short));
  }
};

static void rtlsdr_callback(unsigned char *buf, uint32_t len, void *ctx) {
  FMDemodulator *fm_demodulator = (FMDemodulator *)ctx;
  fm_demodulator->in(buf, len);
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
  fprintf(stderr, "Actual frequency = %d\n", rtlsdr_get_center_freq(dev));
  //
  // Sampling at 1 MHz
  //
  rtlsdr_set_sample_rate(dev, RFSamplingRate);
  fprintf(stderr, "Sampling frequency = %d\n", rtlsdr_get_sample_rate(dev));

  float *b = b28;
  float *a = a28;
  if (argc >= 3) {
    switch (atoi(argv[2])) {
    case 12:
      b = b12;
      a = a12;
      break;
    case 16:
      b = b16;
      a = a16;
      break;
    case 20:
      b = b20;
      a = a20;
      break;
    case 24:
      b = b24;
      a = a24;
      break;
    case 28:
      b = b28;
      a = a28;
      break;
    case 32:
      b = b32;
      a = a32;
      break;
    default:
      fprintf(stderr, "Unknown filter width");
      exit(1);
    }
  }

  int DownSamplingRate = RFSamplingRate / AudioSamplingRate;
  FMDemodulator *fm_demodulator = new FMDemodulator(20, DownSamplingRate, b, a);
  rtlsdr_read_async(dev, rtlsdr_callback, fm_demodulator, 0, 0);

  return 0;
}
