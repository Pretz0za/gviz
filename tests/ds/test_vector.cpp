// Unit tests for the free-function vector-math helpers in ds/vector.hpp.
// These operate on raw double* buffers tagged with an explicit dimension, and
// most have dedicated fast paths for dim == 2/3/4 plus a generic fallback --
// each case is exercised separately since the fast paths are hand-unrolled
// and can drift from the generic path.

#include "catch_amalgamated.hpp"
#include "ds/vector.hpp"

TEST_CASE("IsZero(scalar) treats values within EPSILON as zero", "[ds][vector]") {
  REQUIRE(IsZero(0.0));
  REQUIRE(IsZero(1e-9));
  REQUIRE_FALSE(IsZero(1e-5));
  REQUIRE_FALSE(IsZero(-1.0));
}

TEST_CASE("IsZero(vec, dim) requires every component to be near zero",
         "[ds][vector]") {
  double allZero2[2] = {0.0, 0.0};
  double oneNonZero2[2] = {0.0, 1.0};
  REQUIRE(IsZero(allZero2, 2));
  REQUIRE_FALSE(IsZero(oneNonZero2, 2));

  double allZero3[3] = {0.0, 0.0, 0.0};
  double oneNonZero3[3] = {0.0, 0.0, 0.5};
  REQUIRE(IsZero(allZero3, 3));
  REQUIRE_FALSE(IsZero(oneNonZero3, 3));

  double allZero4[4] = {0.0, 0.0, 0.0, 0.0};
  double oneNonZero4[4] = {0.0, 0.0, 0.0, 0.1};
  REQUIRE(IsZero(allZero4, 4));
  REQUIRE_FALSE(IsZero(oneNonZero4, 4));

  double allZero5[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
  double oneNonZero5[5] = {0.0, 0.0, 0.0, 0.0, 0.3};
  REQUIRE(IsZero(allZero5, 5));
  REQUIRE_FALSE(IsZero(oneNonZero5, 5));
}

TEST_CASE("IsNan reports whether every component is NaN", "[ds][vector]") {
  double nanVec[3] = {std::nan(""), std::nan(""), std::nan("")};
  double mixed[3] = {std::nan(""), 0.0, std::nan("")};
  double none[3] = {1.0, 2.0, 3.0};

  REQUIRE(IsNan(nanVec, 3));
  REQUIRE_FALSE(IsNan(mixed, 3));
  REQUIRE_FALSE(IsNan(none, 3));
}

TEST_CASE("ZeroOut clears every component", "[ds][vector]") {
  double v[4] = {1.0, 2.0, 3.0, 4.0};
  ZeroOut(v, 4);
  for (double d : v)
    REQUIRE(d == 0.0);
}

TEST_CASE("Copy duplicates dim components", "[ds][vector]") {
  double src[3] = {1.5, -2.5, 3.5};
  double dst[3] = {0.0, 0.0, 0.0};
  Copy(src, dst, 3);
  REQUIRE(dst[0] == 1.5);
  REQUIRE(dst[1] == -2.5);
  REQUIRE(dst[2] == 3.5);
}

TEST_CASE("DotProduct matches the definition across dimensions", "[ds][vector]") {
  SECTION("dim 2") {
    double a[2] = {1.0, 2.0};
    double b[2] = {3.0, 4.0};
    REQUIRE(DotProduct(a, b, 2) == Catch::Approx(11.0));
  }
  SECTION("dim 3") {
    double a[3] = {1.0, 2.0, 3.0};
    double b[3] = {4.0, 5.0, 6.0};
    REQUIRE(DotProduct(a, b, 3) == Catch::Approx(32.0));
  }
  SECTION("dim 4") {
    double a[4] = {1.0, 2.0, 3.0, 4.0};
    double b[4] = {1.0, 1.0, 1.0, 1.0};
    REQUIRE(DotProduct(a, b, 4) == Catch::Approx(10.0));
  }
  SECTION("dim 5 (generic path)") {
    double a[5] = {1.0, 1.0, 1.0, 1.0, 1.0};
    double b[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    REQUIRE(DotProduct(a, b, 5) == Catch::Approx(15.0));
  }
}

TEST_CASE("L2NormSquared and L2Norm agree with the dot-product definition",
         "[ds][vector]") {
  double v[3] = {3.0, 4.0, 0.0};
  REQUIRE(L2NormSquared(v, 3) == Catch::Approx(25.0));
  REQUIRE(L2Norm(v, 3) == Catch::Approx(5.0));
}

TEST_CASE("Vecaxpy computes y += alpha * x per dimension", "[ds][vector]") {
  SECTION("dim 2") {
    double x[2] = {1.0, 2.0};
    double y[2] = {10.0, 10.0};
    Vecaxpy(2.0, x, y, 2);
    REQUIRE(y[0] == Catch::Approx(12.0));
    REQUIRE(y[1] == Catch::Approx(14.0));
  }
  SECTION("dim 3") {
    double x[3] = {1.0, 2.0, 3.0};
    double y[3] = {0.0, 0.0, 0.0};
    Vecaxpy(-1.0, x, y, 3);
    REQUIRE(y[0] == Catch::Approx(-1.0));
    REQUIRE(y[1] == Catch::Approx(-2.0));
    REQUIRE(y[2] == Catch::Approx(-3.0));
  }
  SECTION("dim 4") {
    double x[4] = {1.0, 1.0, 1.0, 1.0};
    double y[4] = {1.0, 2.0, 3.0, 4.0};
    Vecaxpy(3.0, x, y, 4);
    REQUIRE(y[0] == Catch::Approx(4.0));
    REQUIRE(y[1] == Catch::Approx(5.0));
    REQUIRE(y[2] == Catch::Approx(6.0));
    REQUIRE(y[3] == Catch::Approx(7.0));
  }
  SECTION("dim 5 (generic path)") {
    double x[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    Vecaxpy(2.0, x, y, 5);
    for (int i = 0; i < 5; i++)
      REQUIRE(y[i] == Catch::Approx(2.0 * x[i]));
  }
}

TEST_CASE("Subtract computes a - b into out", "[ds][vector]") {
  SECTION("dim 2") {
    double a[2] = {5.0, 5.0};
    double b[2] = {2.0, 1.0};
    double out[2];
    Subtract(a, b, out, 2);
    REQUIRE(out[0] == Catch::Approx(3.0));
    REQUIRE(out[1] == Catch::Approx(4.0));
  }
  SECTION("dim 3") {
    double a[3] = {5.0, 5.0, 5.0};
    double b[3] = {1.0, 2.0, 3.0};
    double out[3];
    Subtract(a, b, out, 3);
    REQUIRE(out[0] == Catch::Approx(4.0));
    REQUIRE(out[1] == Catch::Approx(3.0));
    REQUIRE(out[2] == Catch::Approx(2.0));
  }
  SECTION("dim 4 (generic path)") {
    double a[4] = {5.0, 5.0, 5.0, 5.0};
    double b[4] = {1.0, 2.0, 3.0, 4.0};
    double out[4];
    Subtract(a, b, out, 4);
    REQUIRE(out[0] == Catch::Approx(4.0));
    REQUIRE(out[1] == Catch::Approx(3.0));
    REQUIRE(out[2] == Catch::Approx(2.0));
    REQUIRE(out[3] == Catch::Approx(1.0));
  }
  SECTION("out may alias a") {
    double a[2] = {5.0, 5.0};
    double b[2] = {2.0, 1.0};
    Subtract(a, b, a, 2);
    REQUIRE(a[0] == Catch::Approx(3.0));
    REQUIRE(a[1] == Catch::Approx(4.0));
  }
}

TEST_CASE("Distance computes the Euclidean distance between two points",
         "[ds][vector]") {
  SECTION("dim 2, 3-4-5 triangle") {
    double a[2] = {0.0, 0.0};
    double b[2] = {3.0, 4.0};
    REQUIRE(Distance(a, b, 2) == Catch::Approx(5.0));
  }
  SECTION("dim 3") {
    double a[3] = {0.0, 0.0, 0.0};
    double b[3] = {2.0, 3.0, 6.0};
    REQUIRE(Distance(a, b, 3) == Catch::Approx(7.0));
  }
  SECTION("dim 5 (generic path)") {
    double a[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    double b[5] = {1.0, 1.0, 1.0, 1.0, 1.0};
    REQUIRE(Distance(a, b, 5) == Catch::Approx(std::sqrt(5.0)));
  }
  SECTION("distance from a point to itself is zero") {
    double a[3] = {1.0, 2.0, 3.0};
    REQUIRE(Distance(a, a, 3) == Catch::Approx(0.0));
  }
}

TEST_CASE("Scale multiplies every component by a scalar", "[ds][vector]") {
  SECTION("dim 2") {
    double v[2] = {1.0, 2.0};
    Scale(v, 3.0, 2);
    REQUIRE(v[0] == Catch::Approx(3.0));
    REQUIRE(v[1] == Catch::Approx(6.0));
  }
  SECTION("dim 3") {
    double v[3] = {1.0, 2.0, 3.0};
    Scale(v, -2.0, 3);
    REQUIRE(v[0] == Catch::Approx(-2.0));
    REQUIRE(v[1] == Catch::Approx(-4.0));
    REQUIRE(v[2] == Catch::Approx(-6.0));
  }
  SECTION("dim 4") {
    double v[4] = {1.0, 1.0, 1.0, 1.0};
    Scale(v, 0.5, 4);
    for (double d : v)
      REQUIRE(d == Catch::Approx(0.5));
  }
  SECTION("dim 5 (generic path)") {
    double v[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    Scale(v, 10.0, 5);
    REQUIRE(v[0] == Catch::Approx(10.0));
    REQUIRE(v[4] == Catch::Approx(50.0));
  }
}

TEST_CASE("Negate flips the sign of every component", "[ds][vector]") {
  SECTION("dim 2") {
    double v[2] = {1.0, -2.0};
    Negate(v, 2);
    REQUIRE(v[0] == Catch::Approx(-1.0));
    REQUIRE(v[1] == Catch::Approx(2.0));
  }
  SECTION("dim 3") {
    double v[3] = {1.0, -2.0, 3.0};
    Negate(v, 3);
    REQUIRE(v[0] == Catch::Approx(-1.0));
    REQUIRE(v[1] == Catch::Approx(2.0));
    REQUIRE(v[2] == Catch::Approx(-3.0));
  }
  SECTION("dim 4") {
    double v[4] = {1.0, -2.0, 3.0, -4.0};
    Negate(v, 4);
    REQUIRE(v[0] == Catch::Approx(-1.0));
    REQUIRE(v[1] == Catch::Approx(2.0));
    REQUIRE(v[2] == Catch::Approx(-3.0));
    REQUIRE(v[3] == Catch::Approx(4.0));
  }
  SECTION("returns a pointer to the same buffer") {
    double v[2] = {1.0, 1.0};
    REQUIRE(Negate(v, 2) == v);
  }
}
