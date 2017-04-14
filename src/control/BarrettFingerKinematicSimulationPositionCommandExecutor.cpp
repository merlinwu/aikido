#include <aikido/control/BarrettFingerKinematicSimulationPositionCommandExecutor.hpp>
#include <thread>

namespace aikido{
namespace control{

//=============================================================================
BarrettFingerKinematicSimulationPositionCommandExecutor
::BarrettFingerKinematicSimulationPositionCommandExecutor(
  ::dart::dynamics::ChainPtr finger, size_t proximal, size_t distal,
  ::dart::collision::CollisionGroupPtr collideWith,
  ::dart::collision::CollisionOption collisionOptions)
: mFinger(std::move(finger))
, mProximalDof(nullptr)
, mDistalDof(nullptr)
, mCollideWith(std::move(collideWith))
, mCollisionOptions(std::move(collisionOptions))
, mInExecution(false)
{
  if (!mFinger)
    throw std::invalid_argument("Finger is null.");

  if (proximal == distal)
    throw std::invalid_argument("proximal and distal dofs should be different.");

  const auto numDofs = mFinger->getNumDofs();

  if (proximal < numDofs)
    mProximalDof = mFinger->getDof(proximal);

  if (distal < numDofs)
    mDistalDof = mFinger->getDof(distal);

  if (!mProximalDof)
    throw std::invalid_argument("Finger does not have proximal dof.");

  if (!mDistalDof)
    throw std::invalid_argument("Finger does not have distal dof.");

  if (!mCollideWith)
    throw std::invalid_argument("CollideWith is null.");

  mCollisionDetector = mCollideWith->getCollisionDetector();

  mProximalCollisionGroup = mCollisionDetector->createCollisionGroup(
      mProximalDof->getChildBodyNode());

  mDistalCollisionGroup = mCollisionDetector->createCollisionGroup(
      mDistalDof->getChildBodyNode());

  mProximalLimits = mProximalDof->getPositionLimits();
  mDistalLimits = mDistalDof->getPositionLimits();
}

//=============================================================================
std::future<void> BarrettFingerKinematicSimulationPositionCommandExecutor
::execute(double goalPosition)
{
  if (!mFinger->isAssembled())
    throw std::runtime_error("Finger is disassembled.");

  {
    std::lock_guard<std::mutex> lock(mMutex);

    if (mInExecution)
      throw std::runtime_error("Another command in execution.");

    mPromise.reset(new std::promise<void>());
    mInExecution = true;
    mDistalOnly = false;

    // Set mProximalGoalPosition.
    if (goalPosition < mProximalLimits.first)
      mProximalGoalPosition = mProximalLimits.first;
    else if (goalPosition > mProximalLimits.second)
      mProximalGoalPosition = mProximalLimits.second;
    else
      mProximalGoalPosition = goalPosition;

    return mPromise->get_future();
  }
}

//=============================================================================
void BarrettFingerKinematicSimulationPositionCommandExecutor::terminate()
{
  mPromise->set_value();
  mInExecution = false;
  mCollideWith.reset();
}

//=============================================================================
void BarrettFingerKinematicSimulationPositionCommandExecutor::step(
 const std::chrono::milliseconds& timeSincePreviousCall)
{
  auto period = std::chrono::duration<double>(
    timeSincePreviousCall).count();

  std::lock_guard<std::mutex> lock(mMutex);

  if (!mInExecution)
    return;

  double distalPosition = mDistalDof->getPosition();
  double proximalPosition = mProximalDof->getPosition();

  // Check distal collision
  bool distalCollision = mCollisionDetector->collide(
    mDistalCollisionGroup.get(), mCollideWith.get(),
    mCollisionOptions, nullptr);

  if (distalCollision)
  {
    terminate();
    return;
  }

  double newDistal;
  bool distalLimitReached = false;

  if (proximalPosition < mProximalGoalPosition)
  {
    newDistal = distalPosition + period*kDistalSpeed;
    if (mDistalLimits.second <= newDistal)
    {
      newDistal = mDistalLimits.second;
      distalLimitReached = true;
    }
  }
  else
  {
    newDistal = distalPosition - period*kDistalSpeed;
    if (mDistalLimits.first >= newDistal)
    {
      newDistal = mDistalLimits.first;
      distalLimitReached = true;
    }
  }

  mDistalDof->setPosition(newDistal);

  if (distalLimitReached)
  {
    terminate();
    return;
  }

  if (mDistalOnly)
    return;

  // Check proximal collision
  bool proximalCollision = mCollisionDetector->collide(
    mProximalCollisionGroup.get(), mCollideWith.get(),
    mCollisionOptions, nullptr);

  if (proximalCollision){
    mDistalOnly = true;
    return;
  }

  double newProximal;
  bool proximalGoalReached = false;
  if (proximalPosition < mProximalGoalPosition)
  {
    newProximal = proximalPosition + period*kProximalSpeed;
    if (mProximalGoalPosition <= newProximal)
    {
      newProximal = mProximalGoalPosition;
      proximalGoalReached = true;
    }
  }
  else
  {
    newProximal = proximalPosition - period*kProximalSpeed;
    if (mProximalGoalPosition >= newProximal)
    {
      newProximal = mProximalGoalPosition;
      proximalGoalReached = true;
    }
  }

  mProximalDof->setPosition(newProximal);
  if (proximalGoalReached)
  {
    terminate();
    return;
  }

}

//=============================================================================
bool BarrettFingerKinematicSimulationPositionCommandExecutor::setCollideWith(
  ::dart::collision::CollisionGroupPtr collideWith)
{
  std::lock_guard<std::mutex> lockSpin(mMutex);

  if (mInExecution)
    return false;

  mCollideWith = collideWith;
  return true;
}

} // control
} // aikido
