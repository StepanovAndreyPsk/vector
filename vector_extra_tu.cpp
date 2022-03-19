#include <memory>
#include <string>
#include "vector.h"  // Ensure that including the header in a separate TU does not induce ODR violation.
#include "vector.h"  // Ensure that double inclusion does not break anything.

namespace {
[[maybe_unused]] lab_07::vector<std::string> vec_string;
[[maybe_unused]] lab_07::vector<int> vec_int;
[[maybe_unused]] lab_07::vector<std::unique_ptr<int>> vec_pint;
}  // namespace
