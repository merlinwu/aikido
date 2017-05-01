#include <cassert>
#include <set>
#include <dart/common/StlHelpers.hpp>
#include <aikido/planner/parabolic/ParabolicSmoother.hpp>
#include <aikido/util/Spline.hpp>
#include "DynamicPath.h"
#include "ParabolicUtil.hpp"
#include "HauserParabolicSmoother.hpp"


namespace aikido {
namespace planner {
namespace parabolic {

std::unique_ptr<aikido::trajectory::Spline> doShortcut(
    aikido::trajectory::Spline* _inputTrajectory,
    const aikido::constraint::TestablePtr _feasibilityCheck,
    const Eigen::VectorXd& _maxVelocity,
    const Eigen::VectorXd& _maxAcceleration,
    double _timelimit,
    bool _useVelocity)
{
  auto stateSpace = _inputTrajectory->getStateSpace();
  HauserParabolicSmoother parabolicSmoother(_feasibilityCheck,
                                            _timelimit,
                                            _useVelocity);

  double startTime = 0.0;
  auto dynamicPath = convertToDynamicPath(_inputTrajectory, startTime,
                                          _maxVelocity, _maxAcceleration);

  parabolicSmoother.doShortcut(dynamicPath.get());

  auto outputTrajectory =
            convertToSpline(dynamicPath.get(), startTime, stateSpace);

  return outputTrajectory;
}


std::unique_ptr<trajectory::Spline> doBlend(
    trajectory::Spline* _inputTrajectory,
    const aikido::constraint::TestablePtr _feasibilityCheck,
    const Eigen::VectorXd& _maxVelocity,
    const Eigen::VectorXd& _maxAcceleration,
    double _timelimit,
    bool _useVelocity,
    double _blendRadius,
    int _blendIterations)
{
  auto stateSpace = _inputTrajectory->getStateSpace();
  HauserParabolicSmoother parabolicSmoother(_feasibilityCheck,
                                            _timelimit,
                                            _useVelocity,
                                            _blendRadius,
                                            _blendIterations);
  double startTime = 0.0;
  auto dynamicPath = convertToDynamicPath(_inputTrajectory, startTime,
                                          _maxVelocity, _maxAcceleration);

  parabolicSmoother.doBlend(dynamicPath.get());

  auto outputTrajectory =
              convertToSpline(dynamicPath.get(), startTime, stateSpace);

  return outputTrajectory;
}

std::unique_ptr<trajectory::Spline> doShortcutAndBlend(
    trajectory::Spline* _inputTrajectory,
    const aikido::constraint::TestablePtr _feasibilityCheck,
    const Eigen::VectorXd& _maxVelocity,
    const Eigen::VectorXd& _maxAcceleration,
    double _timelimit,
    bool _useVelocity,
    double _blendRadius,
    int _blendIterations)
{
  auto stateSpace = _inputTrajectory->getStateSpace();
  HauserParabolicSmoother parabolicSmoother(_feasibilityCheck,
                                            _timelimit,
                                            _useVelocity,
                                            _blendRadius,
                                            _blendIterations);

  double startTime = 0.0;
  auto dynamicPath = convertToDynamicPath(_inputTrajectory, startTime,
                                          _maxVelocity, _maxAcceleration);

  parabolicSmoother.doShortcut(dynamicPath.get());
  parabolicSmoother.doBlend(dynamicPath.get());

  auto outputTrajectory =
                convertToSpline(dynamicPath.get(), startTime, stateSpace);

  return outputTrajectory;
}


} // namespace parabolic
} // namespace planner
} // namespace aikido