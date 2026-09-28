// RegularSimplexPoints builds the (dim+1) vertices of a regular simplex
// (equilateral triangle for dim=2, regular tetrahedron for dim=3, ...) with
// a given edge length. FromDenseCoordinateArray is a small copy helper.

#include "catch_amalgamated.hpp"
#include "layout/placement/helpers.hpp"

#include <cmath>
#include <vector>

namespace {
double DistanceBetween(const double *out, uint32_t dim, uint32_t i, uint32_t j) {
  double sumSq = 0.0;
  for (uint32_t d = 0; d < dim; d++) {
    double diff = out[i * dim + d] - out[j * dim + d];
    sumSq += diff * diff;
  }
  return std::sqrt(sumSq);
}
} // namespace

TEST_CASE("A dim=2 regular simplex is an equilateral triangle with the "
         "requested side length",
         "[layout][placement][helpers]") {
  std::vector<double> out(3 * 2);
  RegularSimplexPoints(2, 10.0, out.data());

  REQUIRE(DistanceBetween(out.data(), 2, 0, 1) == Catch::Approx(10.0).margin(1e-9));
  REQUIRE(DistanceBetween(out.data(), 2, 0, 2) == Catch::Approx(10.0).margin(1e-9));
  REQUIRE(DistanceBetween(out.data(), 2, 1, 2) == Catch::Approx(10.0).margin(1e-9));
}

TEST_CASE("A dim=3 regular simplex is a regular tetrahedron with the "
         "requested edge length",
         "[layout][placement][helpers]") {
  std::vector<double> out(4 * 3);
  RegularSimplexPoints(3, 5.0, out.data());

  for (uint32_t i = 0; i < 4; i++) {
    for (uint32_t j = i + 1; j < 4; j++) {
      REQUIRE(DistanceBetween(out.data(), 3, i, j) ==
              Catch::Approx(5.0).margin(1e-9));
    }
  }
}

TEST_CASE("Scaling the side length scales all pairwise distances the same way",
         "[layout][placement][helpers]") {
  std::vector<double> small(3 * 2);
  std::vector<double> big(3 * 2);
  RegularSimplexPoints(2, 1.0, small.data());
  RegularSimplexPoints(2, 100.0, big.data());

  REQUIRE(DistanceBetween(big.data(), 2, 0, 1) ==
          Catch::Approx(100.0 * DistanceBetween(small.data(), 2, 0, 1)));
}

TEST_CASE("FromDenseCoordinateArray copies the first n rows of dim-wide data",
         "[layout][placement][helpers]") {
  double in[8] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}; // 4 rows of dim 2
  double out[4] = {0.0, 0.0, 0.0, 0.0};

  FromDenseCoordinateArray(in, out, /*dim=*/2, /*n=*/2);

  REQUIRE(out[0] == 1.0);
  REQUIRE(out[1] == 2.0);
  REQUIRE(out[2] == 3.0);
  REQUIRE(out[3] == 4.0);
}

TEST_CASE("FromDenseCoordinateArray leaves later rows untouched", "[layout][placement][helpers]") {
  double in[6] = {1.0, 1.0, 1.0, 2.0, 2.0, 2.0}; // 3 rows of dim 2
  double out[6] = {9.0, 9.0, 9.0, 9.0, 9.0, 9.0};

  FromDenseCoordinateArray(in, out, /*dim=*/2, /*n=*/1);

  REQUIRE(out[0] == 1.0);
  REQUIRE(out[1] == 1.0);
  REQUIRE(out[2] == 9.0); // untouched
  REQUIRE(out[3] == 9.0);
}
