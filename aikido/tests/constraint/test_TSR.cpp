#include <aikido/constraint/TSR.hpp>
// #include <aikido/statesoace/SE3StateSpace.hpp>
#include <aikido/util/RNG.hpp>
#include <dart/common/StlHelpers.h>
#include <gtest/gtest.h>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <dart/math/Geometry.h>

using aikido::constraint::TSR;
using aikido::constraint::TSRSampleGenerator;
using aikido::statespace::SE3StateSpace;
using aikido::util::RNGWrapper;
using aikido::util::RNG;
using dart::common::make_unique;

using DefaultRNG = RNGWrapper<std::default_random_engine>;


static Eigen::Vector6d getPose(Eigen::Isometry3d isometry)
{
  Eigen::Vector6d pose;
  pose.topRows(3) = isometry.translation(); 
  pose.bottomRows(3) = dart::math::matrixToEulerZYX(isometry.linear());
  return pose; 
}

static std::unique_ptr<DefaultRNG> make_rng()
{
  return make_unique<RNGWrapper<std::default_random_engine>>(0);
}

TEST(TSR, InitializesToIdentity)
{
  TSR tsr;
  TSR tsr_default;

  EXPECT_EQ(tsr.mT0_w.translation(), 
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr.mT0_w.rotation(), 
            Eigen::Isometry3d::Identity().rotation());
  EXPECT_EQ(tsr.mBw, Eigen::MatrixXd::Zero(6,2));
  EXPECT_EQ(tsr.mTw_e.translation(),
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr.mTw_e.rotation(),
            Eigen::Isometry3d::Identity().rotation());

  EXPECT_EQ(tsr_default.mT0_w.translation(), 
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr_default.mT0_w.rotation(), 
            Eigen::Isometry3d::Identity().rotation());
  EXPECT_EQ(tsr_default.mBw, Eigen::MatrixXd::Zero(6,2));
  EXPECT_EQ(tsr_default.mTw_e.translation(),
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr_default.mTw_e.rotation(),
            Eigen::Isometry3d::Identity().rotation());
}

TEST(TSR, CopyConstructor)
{
  TSR tsr;
  TSR tsr2(tsr);

  EXPECT_EQ(tsr2.mT0_w.translation(), 
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr2.mT0_w.rotation(), 
            Eigen::Isometry3d::Identity().rotation());
  EXPECT_EQ(tsr2.mBw, Eigen::MatrixXd::Zero(6,2));
  EXPECT_EQ(tsr2.mTw_e.translation(), 
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr2.mTw_e.rotation(), 
            Eigen::Isometry3d::Identity().rotation());


  tsr2.mBw(0,0) = 3;
  EXPECT_THROW(tsr2.validate(), std::invalid_argument);
  EXPECT_NO_THROW(tsr.validate());

}


TEST(TSR, AssignmentOperator)
{
  TSR tsr;
  TSR tsr2 = tsr;

  EXPECT_EQ(tsr2.mT0_w.translation(), 
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr2.mT0_w.rotation(), 
            Eigen::Isometry3d::Identity().rotation());;
  EXPECT_EQ(tsr2.mBw, Eigen::MatrixXd::Zero(6,2));
  EXPECT_EQ(tsr2.mTw_e.translation(), 
            Eigen::Isometry3d::Identity().translation());
  EXPECT_EQ(tsr2.mTw_e.rotation(), 
            Eigen::Isometry3d::Identity().rotation());

  tsr2.mBw(0,0) = 3;

  EXPECT_THROW(tsr2.validate(), std::invalid_argument);
  EXPECT_NO_THROW(tsr.validate());

}

TEST(TSR, Validate)
{
  TSR tsr; 

  tsr.mBw = Eigen::Matrix<double, 6, 2>::Zero();
  tsr.mBw(3,0) = M_PI/2;
  tsr.mBw(3,1) = -M_PI/2;
  
  EXPECT_THROW(tsr.validate(), std::invalid_argument);

  Eigen::MatrixXd Bw = Eigen::MatrixXd::Zero(6,2);
  Bw(3,0) = M_PI;
  Bw(3,1) = 2*M_PI;
  tsr.mBw = Bw;

  EXPECT_NO_THROW(tsr.validate());
}

TEST(TSRSampleGenerator, SampleIdentity)
{
  TSR tsr;
  auto state = tsr.getSE3StateSpace()->createState();

  ASSERT_TRUE(tsr.createSampleGenerator()->sample(state));
  ASSERT_TRUE(state.getIsometry().isApprox(Eigen::Isometry3d::Identity()));
}

TEST(TSRSampleGenerator, SamplePointTSR)
{
  TSR tsr;

  Eigen::Matrix3d rotation;
  rotation = Eigen::AngleAxisd(M_PI/2, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
  Eigen::Isometry3d T0_w;
  T0_w.setIdentity();
  T0_w.linear() = rotation;
  tsr.mT0_w = T0_w;
  tsr.validate();

  auto state = tsr.getSE3StateSpace()->createState();
  ASSERT_TRUE(tsr.createSampleGenerator()->sample(state));
  EXPECT_TRUE(state.getIsometry().isApprox(T0_w));
}

TEST(TSRSampleGenerator, SampleWithinBounds)
{
  TSR tsr;

  Eigen::MatrixXd Bw = Eigen::Matrix<double, 6, 2>::Zero();
  Bw(0,0) = -1;
  Bw(0,1) = 1;

  tsr.mBw = Bw;

  auto sampler = tsr.createSampleGenerator();
  auto state = tsr.getSE3StateSpace()->createState();
  
  for (int i = 0; i < 10; i++)
  {
    ASSERT_TRUE(sampler->sample(state));

    Eigen::Vector3d translation = state.getIsometry().translation();
    EXPECT_TRUE(translation(0) >= -1 && translation(1) <= 1);
    EXPECT_DOUBLE_EQ(translation(1), 0);
    EXPECT_DOUBLE_EQ(translation(2), 0);
  }
}

TEST(TSRSampleGenerator, SampleSameSequence)
{
  TSR tsr;

  Eigen::MatrixXd Bw = Eigen::Matrix<double, 6, 2>::Zero();
  Bw(0,0) = -1;
  Bw(0,1) = 1;

  tsr.mBw = Bw;

  auto sampler1 = tsr.createSampleGenerator();
  auto sampler2 = tsr.createSampleGenerator();

  auto state1 = tsr.getSE3StateSpace()->createState();
  auto state2 = tsr.getSE3StateSpace()->createState();

  for (int i = 0; i < 10; i++)
  {
    ASSERT_TRUE(sampler1->sample(state1));
    ASSERT_TRUE(sampler2->sample(state2));
    EXPECT_TRUE(state1.getIsometry().isApprox(state2.getIsometry()));
  }
}

TEST(TSR, GetValue)
{
  TSR tsr;

  // non-trivial translation bounds 
  Eigen::MatrixXd Bw = Eigen::Matrix<double, 6, 2>::Zero();
  Bw(0,0) = 0;
  Bw(0,1) = 2;

  tsr.mBw = Bw;

  auto state = tsr.getSE3StateSpace()->createState();  

  // strictly inside TSR
  Eigen::Isometry3d isometry(Eigen::Isometry3d::Identity());
  isometry.translation() = Eigen::Vector3d(1.5, 0, 0);
  state.setIsometry(isometry);
  Eigen::VectorXd value = tsr.getValue(state);
  EXPECT_TRUE(value.isApproxToConstant(0, 1e-3));


  // strictly inside TSR
  isometry.translation() = Eigen::Vector3d(1, 0, 0);
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  EXPECT_TRUE(value.isApproxToConstant(0));


  // boundary of TSR
  isometry.translation() = Eigen::Vector3d(2, 0, 0);
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  EXPECT_TRUE(value.isApproxToConstant(0));


  // outside TSR
  isometry.translation() = Eigen::Vector3d(3, 0, 0);
  state.setIsometry(isometry);

  Eigen::Vector6d expected(Eigen::Vector6d::Zero());
  expected(0) = 1; 
  value = tsr.getValue(state);
  EXPECT_TRUE(tsr.getValue(state).isApprox(expected));


  // TSR non-trivial angle bounds
  Bw = Eigen::Matrix<double, 6, 2>::Zero();
  Bw(3,0) = 0;
  Bw(3,1) = M_PI;

  tsr.mBw = Bw;
  
  // strictly inside TSR
  Eigen::Matrix3d rotation;
  rotation = Eigen::AngleAxisd(M_PI_2, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());

  isometry = Eigen::Isometry3d::Identity();
  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  EXPECT_TRUE(value.isApproxToConstant(0, 1e-3));

  // [PI/2, PI/2+2PI-0.1]
  Bw(3,0) = M_PI_2;
  Bw(3,1) = M_PI_2+M_PI*2 - 0.1;
  tsr.mBw = Bw;

  rotation = Eigen::AngleAxisd(M_PI_4, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  expected = Eigen::Vector6d::Zero();
  EXPECT_TRUE(value.isApproxToConstant(0, 1e-3));

  /* cyclic bound */
  Bw(3,0) = M_PI_2;
  Bw(3,1) = M_PI_2+M_PI*2;
  tsr.mBw = Bw;

  rotation = Eigen::AngleAxisd(M_PI_4, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  expected = Eigen::Vector6d::Zero();
  EXPECT_TRUE(value.isApproxToConstant(0, 1e-3));


  // boundary of TSR
  rotation = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());

  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  EXPECT_TRUE(value.isApproxToConstant(0));


  /* outside TSR */
  // [PI/2, PI] bound
  Bw(3,0) = M_PI_2;
  Bw(3,1) = M_PI;
  tsr.mBw = Bw;

  rotation = Eigen::AngleAxisd(M_PI_2 - 0.1, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  expected = Eigen::Vector6d::Zero();
  expected(3) = 0.1; 
  EXPECT_TRUE(value.isApprox(expected));


  // Same bound, but with 5*2*pi added to each end.  
  Bw(3,0) = M_PI_2 + 5*(M_PI*2);
  Bw(3,1) = M_PI + 5*(M_PI*2);

  tsr.mBw = Bw;

  rotation = Eigen::AngleAxisd(M_PI_2 - 0.1, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  expected = Eigen::Vector6d::Zero();
  expected(3) = 0.1; 
  EXPECT_TRUE(value.isApprox(expected));

  // [PI/2, 2*PI]
  Bw(3,0) = M_PI_2;
  Bw(3,1) = M_PI*2;
  tsr.mBw = Bw;

  rotation = Eigen::AngleAxisd(M_PI_4 - 0.1, Eigen::Vector3d::UnitZ()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitY()) *
             Eigen::AngleAxisd(0, Eigen::Vector3d::UnitX());
  isometry.linear() = rotation;
  state.setIsometry(isometry);
  value = tsr.getValue(state);
  expected = Eigen::Vector6d::Zero();
  expected(3) = M_PI_4-0.1;
  EXPECT_TRUE(value.isApprox(expected, 1e-3));

}

TEST(TSR, GetJacobian)
{
  TSR tsr;

  Eigen::MatrixXd Bw = Eigen::Matrix<double, 6, 2>::Zero();
  Bw(0,0) = 0;
  Bw(0,1) = 2;

  tsr.mBw = Bw;

  auto state = tsr.getSE3StateSpace()->createState();  

  // strictly inside TSR
  Eigen::Isometry3d isometry(Eigen::Isometry3d::Identity());
  isometry.translation() = Eigen::Vector3d(1.5, 0, 0);
  state.setIsometry(isometry);

  Eigen::MatrixXd jacobian = tsr.getJacobian(state);
  EXPECT_TRUE(jacobian.isApproxToConstant(0));

  
  // outside TSR
  isometry.translation() = Eigen::Vector3d(3, 0, 0);
  state.setIsometry(isometry);
  jacobian = tsr.getJacobian(state);

  Eigen::Matrix6d expected(Eigen::Matrix6d::Zero());
  expected(0, 3) = 1; 
  EXPECT_TRUE(jacobian.isApprox(expected, 1e-3));

}