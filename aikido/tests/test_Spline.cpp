#include <cstdlib>
#include <gtest/gtest.h>
#include <aikido/path/Spline.hpp>

#define ASSERT_EIGEN_EQUAL(_expected_, _actual_, _epsilon_)\
ASSERT_TRUE(CompareEigenMatrices(_expected_, _actual_, _epsilon_))

#define EXPECT_EIGEN_EQUAL(_expected_, _actual_, _epsilon_)\
EXPECT_TRUE(CompareEigenMatrices(_expected_, _actual_, _epsilon_))

using aikido::path::SplineND;

static constexpr double EPSILON = 1e-6;

namespace {

template <class Derived>
testing::AssertionResult CompareEigenMatrices(
  const Eigen::MatrixBase<Derived>& _expected,
  const Eigen::MatrixBase<Derived>& _actual,
  double _epsilon)
{
  using Index = typename Eigen::ArrayBase<Derived>::Index;
  using Scalar = typename Eigen::ArrayBase<Derived>::Scalar;

  if (_actual.rows() != _expected.rows())
    return testing::AssertionFailure()
      << "Arrays have different sizes: expected " << _expected.rows()
      << " rows, got " << _actual.rows() << ".";

  if (_actual.cols() != _expected.cols())
    return testing::AssertionFailure()
      << "Arrays have different sizes: expected " << _expected.cols()
      << " columns, got " << _actual.cols() << ".";

  for (Index irow = 0; irow < _expected.rows(); ++irow)
  for (Index icol = 0; icol < _expected.cols(); ++icol)
  {
    const Scalar actualValue = _actual(irow, icol);
    const Scalar expectedValue = _expected(irow, icol);
    const Scalar errorValue = std::abs(actualValue - expectedValue);

    if (errorValue > _epsilon)
      return testing::AssertionFailure()
        << "Arrays differ in row " << irow << ", column " << icol << ": "
        << std::setprecision(std::numeric_limits<Scalar>::max_digits10)
        << expectedValue << " !=: " << actualValue << ".";
  }
  return testing::AssertionSuccess();
}

Eigen::VectorXd make_vector(double _value)
{
  Eigen::Matrix<double, 1, 1> valueVector;
  valueVector << _value;
  return valueVector;
}

Eigen::VectorXd make_vector(double _value1, double _value2)
{
  Eigen::Matrix<double, 2, 1> valueVector;
  valueVector << _value1, _value2;
  return valueVector;
}

} // namespace

class SplineNDTests : public testing::Test
{
protected:
  using LinearSpline = SplineND<double, int, 2>;

  void SetUp() override
  {
    mTimesA.resize(3);
    mTimesA << 0., 1., 2.;

    mCoefficientsA.resize(2, LinearSpline::SolutionMatrix(2, 2));
    mCoefficientsA[0] << 0.,  1., 1.,  1.;
    mCoefficientsA[1] << 3., -2., 4., -2.;

    mSplineA = LinearSpline(mTimesA, mCoefficientsA);
  }

  LinearSpline mSplineA;
  LinearSpline::TimeVector mTimesA;
  LinearSpline::SolutionMatrices mCoefficientsA;
};

TEST_F(SplineNDTests, LinearSpline_constructor_LengthMismatch)
{
  LinearSpline::TimeVector times(2);
  times << 0., 2.;

  EXPECT_THROW(LinearSpline(times, mCoefficientsA), std::runtime_error);
}

TEST_F(SplineNDTests, LinearSpline_constructor_TimesAreNotMonotone)
{
  LinearSpline::TimeVector times(3);
  times << 0., 2., 1.;

  EXPECT_THROW(LinearSpline(times, mCoefficientsA), std::runtime_error);
}

TEST_F(SplineNDTests, LinearSpline_getTimes)
{
  EXPECT_EIGEN_EQUAL(mTimesA, mSplineA.getTimes(), EPSILON);
}

TEST_F(SplineNDTests, LinearSpline_getCoefficients)
{
  ASSERT_EQ(2, mSplineA.getCoefficients().size());
  EXPECT_EIGEN_EQUAL(mCoefficientsA[0], mSplineA.getCoefficients()[0], EPSILON);
  EXPECT_EIGEN_EQUAL(mCoefficientsA[1], mSplineA.getCoefficients()[1], EPSILON);
}

TEST_F(SplineNDTests, LinearSpline_getNumKnots)
{
  EXPECT_EQ(3, mSplineA.getNumKnots());
}

TEST_F(SplineNDTests, LinearSpline_getNumOutputs)
{
  EXPECT_EQ(2, mSplineA.getNumOutputs());
}

TEST_F(SplineNDTests, LinearSpline_getNumDerivatives)
{
  EXPECT_EQ(1, mSplineA.getNumDerivatives());
}

TEST_F(SplineNDTests, LinearSpline_getNumCoefficients)
{
  EXPECT_EQ(2, mSplineA.getNumCoefficients());
}

TEST_F(SplineNDTests, LinearSpline_getDuration)
{
  EXPECT_DOUBLE_EQ(2., mSplineA.getDuration());
}

TEST_F(SplineNDTests, LinearSpline_getSegmentIndex)
{
  EXPECT_EQ(0, mSplineA.getSegmentIndex(0.));
  EXPECT_EQ(0, mSplineA.getSegmentIndex(0.5));
  EXPECT_EQ(1, mSplineA.getSegmentIndex(1.5));
}

TEST_F(SplineNDTests, LinearSpline_getSegmentIndex_OutOfBounds)
{
  EXPECT_EQ(0, mSplineA.getSegmentIndex(-0.01));
  EXPECT_EQ(1, mSplineA.getSegmentIndex(2.01));
}

TEST_F(SplineNDTests, LinearSpline_evaluate_Derivative0)
{
  EXPECT_EIGEN_EQUAL(make_vector( 0.0, 1.0), mSplineA.evaluate(0.0, 0), EPSILON);
  EXPECT_EIGEN_EQUAL(make_vector( 0.5, 1.5), mSplineA.evaluate(0.5, 0), EPSILON);
  EXPECT_EIGEN_EQUAL(make_vector( 0.0, 1.0), mSplineA.evaluate(1.5, 0), EPSILON);
  EXPECT_EIGEN_EQUAL(make_vector(-1.0, 0.0), mSplineA.evaluate(2.0, 0), EPSILON);
}

TEST_F(SplineNDTests, LinearSpline_evaluate_Derivative1)
{
  LinearSpline spline(mTimesA, mCoefficientsA);

  EXPECT_EIGEN_EQUAL(make_vector( 1.,  1.), spline.evaluate(0.5, 1), EPSILON);
  EXPECT_EIGEN_EQUAL(make_vector(-2., -2.), spline.evaluate(1.5, 1), EPSILON);
}

TEST_F(SplineNDTests, LinearSpline_evaluate_Derivative2)
{
  EXPECT_EIGEN_EQUAL(make_vector(0., 0.), mSplineA.evaluate(0.5, 2), EPSILON);
  EXPECT_EIGEN_EQUAL(make_vector(0., 0.), mSplineA.evaluate(1.5, 2), EPSILON);
}
