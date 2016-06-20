/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "MultibeamSonarTask.hpp"
#include <frame_helper/FrameHelper.h>

using namespace imaging_sonar_simulation;
using namespace base::samples::frame;

MultibeamSonarTask::MultibeamSonarTask(std::string const& name) :
		MultibeamSonarTaskBase(name) {
	_beam_width.set(base::Angle::fromDeg(120.0));
	_beam_height.set(base::Angle::fromDeg(20.0));
}

MultibeamSonarTask::MultibeamSonarTask(std::string const& name, RTT::ExecutionEngine* engine) :
		MultibeamSonarTaskBase(name, engine) {
	_beam_width.set(base::Angle::fromDeg(120.0));
	_beam_height.set(base::Angle::fromDeg(20.0));
}

MultibeamSonarTask::~MultibeamSonarTask() {
}

bool MultibeamSonarTask::setRange(double value) {
    if (value <= 0) {
        RTT::log(RTT::Error) << "The range must be positive." << RTT::endlog();
        return false;
    }

	_normal_depth_map.setMaxRange(value);
	_msonar.setRange(value);
	return (imaging_sonar_simulation::MultibeamSonarTaskBase::setRange(value));
}

bool MultibeamSonarTask::setGain(double value) {
    if (value < 0 || value > 1) {
        RTT::log(RTT::Error) << "The gain must be between 0.0 and 1.0." << RTT::endlog();
        return false;
    }

    _msonar.setGain(value);
    return (imaging_sonar_simulation::MultibeamSonarTaskBase::setGain(value));
}

bool MultibeamSonarTask::setBin_count(int value) {
    if (value <= 0) {
        RTT::log(RTT::Error) << "The number of bins must be positive and less than 1500." << RTT::endlog();
        return false;
    }

	_msonar.setBinCount(value);
	return (imaging_sonar_simulation::MultibeamSonarTaskBase::setBin_count(value));
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See MultibeamSonarTask.hpp for more detailed
// documentation about them.

bool MultibeamSonarTask::configureHook() {
	if (!MultibeamSonarTaskBase::configureHook())
		return false;

    _msonar.setRange(_range.value());
    _normal_depth_map.setMaxRange(_range.value());
    _msonar.setGain(_gain.value());
    _msonar.setBinCount(_bin_count.value());
    _msonar.setBeamCount(_beam_count.value());
    _msonar.setBeamWidth(_beam_width.value());
    _msonar.setBeamHeight(_beam_height.value());

    if (_msonar.getRange() <= 0) {
        RTT::log(RTT::Error) << "The range must be positive." << RTT::endlog();
        return false;
    }

    if (_msonar.getGain() < 0 || _msonar.getGain() > 1) {
        RTT::log(RTT::Error) << "The gain must be between 0.0 and 1.0." << RTT::endlog();
        return false;
    }

    if (_msonar.getBinCount() <= 0 || _msonar.getBinCount() > 1500) {
        RTT::log(RTT::Error) << "The number of bins must be positive and less than 1500." << RTT::endlog();
        return false;
    }

    if (_msonar.getBeamCount() < 64 || _msonar.getBeamCount() > 512) {
        RTT::log(RTT::Error) << "The number of beams must be between 64 and 512." << RTT::endlog();
        return false;
    }

    if (_msonar.getBeamHeight().getRad() <= 0 || _msonar.getBeamWidth().getRad() <= 0) {
        RTT::log(RTT::Error) << "The sonar opening angles must be positives." << RTT::endlog();
        return false;
    }

	return true;
}

bool MultibeamSonarTask::startHook() {
	if (!MultibeamSonarTaskBase::startHook())
		return false;

	// set shader image parameters
	uint width = 1536;
	Task::init(_msonar.getBeamWidth(), _msonar.getBeamHeight(), width, _msonar.getRange(), false);

	return true;
}

void MultibeamSonarTask::updateHook() {
	MultibeamSonarTaskBase::updateHook();

	base::samples::RigidBodyState linkPose;

	if (_sonar_pose_cmd.read(linkPose) == RTT::NewData) {
	    base::samples::RigidBodyState multibeamSonarPose = rotatePose(linkPose);
		updateMultibeamSonarPose(multibeamSonarPose);
	}
}

void MultibeamSonarTask::updateMultibeamSonarPose(base::samples::RigidBodyState pose) {

	Task::updateSonarPose(pose);

	// receives shader image
	osg::ref_ptr<osg::Image> osg_image = _capture.grabImage(_normal_depth_map.getNormalDepthMapNode());
	cv::Mat3f cv_image = gpu_sonar_simulation::convertShaderOSG2CV(osg_image);

	// simulate sonar data
	std::vector<float> sonar_data = _msonar.codeSonarData(cv_image);

	// apply the "gain" (in this case, it is a light intensity change)
	float gain_factor = _msonar.getGain() / 0.5;
	std::transform(sonar_data.begin(), sonar_data.end(), sonar_data.begin(), std::bind1st(std::multiplies<float>(), gain_factor));
	std::replace_if(sonar_data.begin(), sonar_data.end(), bind2nd(greater<float>(), 1.0), 1.0);

	// simulate sonar data
	base::samples::Sonar sonar = _msonar.simulateMultiBeam(sonar_data);
	_sonar_samples.write(sonar);

	// display shader image
	std::auto_ptr<Frame> frame(new Frame());
	cv::Mat cv_shader;
	cv_image.convertTo(cv_shader, CV_8UC3, 255);
	frame_helper::FrameHelper::copyMatToFrame(cv_shader, *frame.get());
	_shader_viewer.write(RTT::extras::ReadOnlyPointer<Frame>(frame.release()));
}

base::samples::RigidBodyState MultibeamSonarTask::rotatePose(base::samples::RigidBodyState pose) {
    base::samples::RigidBodyState new_pose;
    new_pose.position = pose.position;

    return new_pose;
}

void MultibeamSonarTask::errorHook() {
	MultibeamSonarTaskBase::errorHook();
}

void MultibeamSonarTask::stopHook() {
	MultibeamSonarTaskBase::stopHook();
}

void MultibeamSonarTask::cleanupHook() {
	MultibeamSonarTaskBase::cleanupHook();
}
