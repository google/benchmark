#ifndef COMPLEXITY_H_
#define COMPLEXITY_H_

#include <string>

namespace benchmark {

// BigO is passed to a benchmark in order to specify the asymptotic computational 
// complexity for the benchmark. In case oAuto is selected, complexity will be 
// calculated automatically to the best fit.
enum BigO {
  oNone,
  o1,
  oN,
  oNSquared,
  oNCubed,
  oLogN,
  oNLogN,
  oAuto
};

inline std::string GetBigO(BigO complexity) {
  switch (complexity) {
    case oN:
      return "* N";
    case oNSquared:
      return "* N**2";
    case oNCubed:
      return "* N**3";
    case oLogN:
      return "* lgN";
    case oNLogN:
      return "* NlgN";
    case o1:
      return "* 1";
    default:
      return "";
  }
}

} // end namespace benchmark
#endif // COMPLEXITY_H_
