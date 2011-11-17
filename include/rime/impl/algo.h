#include <cmath>

namespace rime {
namespace algo {

inline double formula_d(double d, double t, double da, double ta) {
  return d + da * exp((ta - t) / 200);
}

inline double formula_p(double s, double u, double t, double d) {
  const double kM = 1 / (1 - exp(-0.005));
  double m = s - (s - u) * pow((1 - exp(-t / 10000)), 10);
  return (d < 20) ? m + (0.5 - m) * (d / kM) :
      m + (1 - m) * (pow(4, (d / kM)) - 1) / 3;
}

}
}
