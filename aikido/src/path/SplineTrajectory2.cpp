#include <aikido/path/Spline.hpp>
#include <aikido/path/SplineTrajectory2.hpp>

namespace aikido {
namespace path {

//=============================================================================
SplineTrajectory2::SplineTrajectory2(
      statespace::StateSpacePtr _stateSpace, double _startTime)
  : mStateSpace(std::move(_stateSpace))
  , mStartTime(_startTime)
{
  if (mStateSpace == nullptr)
    throw std::invalid_argument("StateSpace is null.");
}

//=============================================================================
SplineTrajectory2::~SplineTrajectory2()
{
  for (const auto& segment : mSegments)
    mStateSpace->freeState(segment.mStartState);
}

//=============================================================================
void SplineTrajectory2::addSegment(const Eigen::MatrixXd& _coefficients,
  double _duration, const statespace::StateSpace::State* _startState)
{
  if (_duration <= 0.)
    throw std::invalid_argument("Duration must be positive.");

  if (_coefficients.rows() != mStateSpace->getDimension())
    throw std::invalid_argument("Incorrect number of dimensions.");

  if (_coefficients.cols() < 1)
    throw std::invalid_argument("At least one coefficient is required.");

  PolynomialSegment segment;
  segment.mCoefficients = _coefficients;
  segment.mDuration = _duration;
  segment.mStartState = mStateSpace->allocateState();
  mStateSpace->copyState(_startState, segment.mStartState);

  mSegments.emplace_back(std::move(segment));
}

//=============================================================================
void SplineTrajectory2::addSegment(
  const Eigen::MatrixXd& _coefficients, double _duration)
{
  if (mSegments.empty())
    throw std::logic_error(
      "An explicit start state is required because this trajectory is empty.");

  auto startState = mStateSpace->createState();
  evaluate(getEndTime(), startState);

  addSegment(_coefficients, _duration, startState);
}

//=============================================================================
size_t SplineTrajectory2::getNumSegments() const
{
  return mSegments.size();
}

//=============================================================================
statespace::StateSpacePtr SplineTrajectory2::getStateSpace() const
{
  return mStateSpace;
}

//=============================================================================
size_t SplineTrajectory2::getNumDerivatives() const
{
  size_t numDerivatives = 0;

  for (const auto& segment : mSegments)
  {
    numDerivatives = std::max<size_t>(
      numDerivatives, segment.mCoefficients.cols() - 1);
  }

  return numDerivatives;
}

//=============================================================================
double SplineTrajectory2::getStartTime() const
{
  return mStartTime;
}

//=============================================================================
double SplineTrajectory2::getEndTime() const
{
  return mStartTime + getDuration();
}

//=============================================================================
double SplineTrajectory2::getDuration() const
{
  double duration = 0.;

  for (const auto& segment : mSegments)
    duration += segment.mDuration;

  return duration;
}

//=============================================================================
void SplineTrajectory2::evaluate(
  double _t, statespace::StateSpace::State *_out) const
{
  if (mSegments.empty())
    throw std::logic_error("Unable to evaluate empty trajectory.");

  const auto targetSegmentInfo = getSegmentForTime(_t);
  const auto& targetSegment = mSegments[targetSegmentInfo.first];

  mStateSpace->copyState(targetSegment.mStartState, _out);

  const auto evaluationTime = _t - targetSegmentInfo.second;
  const auto tangentVector = evaluatePolynomial(
    targetSegment.mCoefficients, evaluationTime, 0);

  const auto relativeState = mStateSpace->createState();
  mStateSpace->expMap(tangentVector, relativeState);
  mStateSpace->compose(_out, relativeState);
}

//=============================================================================
Eigen::VectorXd SplineTrajectory2::evaluate(double _t, int _derivative) const
{
  if (mSegments.empty())
    throw std::logic_error("Unable to evaluate empty trajectory.");
  if (_derivative < 1)
    throw std::logic_error("Derivative must be positive.");

  const auto targetSegmentInfo = getSegmentForTime(_t);
  const auto& targetSegment = mSegments[targetSegmentInfo.first];
  const auto evaluationTime = _t - targetSegmentInfo.second;

  // Return zero for higher-order derivatives.
  if (_derivative < targetSegment.mCoefficients.cols())
    // TODO: We should transform this into the body frame using the adjoint
    // transformation.
    return evaluatePolynomial(targetSegment.mCoefficients, evaluationTime,
      _derivative);
  else
    return Eigen::VectorXd::Zero(mStateSpace->getDimension());
}

//=============================================================================
std::pair<size_t, double> SplineTrajectory2::getSegmentForTime(double _t) const
{
  auto segmentStartTime = mStartTime;

  for (size_t isegment = 0; isegment < mSegments.size(); ++isegment)
  {
    const auto& segment = mSegments[isegment];
    const auto nextSegmentStartTime = segmentStartTime + segment.mDuration;

    if (_t <= nextSegmentStartTime)
      return std::make_pair(isegment, segmentStartTime);

    segmentStartTime = nextSegmentStartTime;
  }

  // After the end of the last segment.
  return std::make_pair(
    mSegments.size() - 1, segmentStartTime - mSegments.back().mDuration);
}

//=============================================================================
Eigen::VectorXd SplineTrajectory2::evaluatePolynomial(
  const Eigen::MatrixXd& _coefficients, double _t, int _derivative)
{
  const auto numOutputs = _coefficients.rows();
  const auto numCoeffs = _coefficients.cols();

  const auto timeVector = SplineProblem<>::createTimeVector(
    _t, _derivative, numCoeffs);
  const auto derivativeMatrix = SplineProblem<>::createCoefficientMatrix(
    numCoeffs);
  const auto derivativeVector = derivativeMatrix.row(_derivative);
  const auto evaluationVector = derivativeVector.cwiseProduct(timeVector.transpose());

  Eigen::VectorXd outputVector(numOutputs);

  for (size_t ioutput = 0; ioutput < numOutputs; ++ioutput)
    outputVector[ioutput] = _coefficients.row(ioutput).dot(evaluationVector);

  return outputVector;
}

} // namespace path
} // namespace aikido
