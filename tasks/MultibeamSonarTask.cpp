/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "MultibeamSonarTask.hpp"

using namespace imaging_sonar_simulation;

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

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See MultibeamSonarTask.hpp for more detailed
// documentation about them.

bool MultibeamSonarTask::configureHook() {
    if (!MultibeamSonarTaskBase::configureHook())
		return false;
    // check if the properties have valid values
    if (_beam_count.value() < 64 || _beam_count.value() > 512) {
        RTT::log(RTT::Error) << "The number of beams must be between 64 and 512." << RTT::endlog();
        return false;
    }

    configureSonarSimulation(false);
    sonar_sim->setSonarBeamCount(_beam_count.value());

    return true;
}

bool MultibeamSonarTask::startHook() {
	if (!MultibeamSonarTaskBase::startHook())
		return false;

    return true;
}

void MultibeamSonarTask::updateHook() {
	MultibeamSonarTaskBase::updateHook();

    base::samples::RigidBodyState link_pose;

    while (_sonar_pose_cmd.read(link_pose) == RTT::NewData) {
        // set underwater acoustic effects
        sonar_sim->enableReverb(_enable_reverberation.value());
        sonar_sim->enableSpeckleNoise(_enable_speckle_noise.value());
        sonar_sim->setAttenuationCoefficient(attenuation_properties.frequency,
                                                attenuation_properties.temperature.getCelsius(),
                                                -link_pose.position.z(),
                                                attenuation_properties.salinity,
                                                attenuation_properties.acidity,
                                                _enable_attenuation.value());

        base::samples::Sonar sonar = sonar_sim->simulateSonarData(link_pose.getTransform());

        // set the sonar bearings
        base::Angle interval = base::Angle::fromRad(
            sonar_sim->getSonarBeamWidth().getRad() / sonar_sim->getSonarBeamCount());
        base::Angle start = base::Angle::fromRad(
            -sonar_sim->getSonarBeamWidth().getRad() / 2);
        sonar.setRegularBeamBearings(start, interval);

        // write sonar sample in the output port
        sonar.validate();
        _sonar_samples.write(sonar);

        //display the shader image
        if (_write_shader_image.value()) {
            std::unique_ptr<base::samples::frame::Frame> frame(new base::samples::frame::Frame());
            *frame = sonar_sim->getLastFrame();
            frame->time = base::Time::now();
            _shader_image.write(RTT::extras::ReadOnlyPointer<base::samples::frame::Frame>(frame.release()));
        }
    }
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
bool MultibeamSonarTask::setBin_count(int value) {
    if (value <= 0) {
        RTT::log(RTT::Error) << "The number of bins must be positive." << RTT::endlog();
        return false;
    }

    sonar_sim->setSonarBinCount(value);
    float width = sonar_sim->getSonarBinCount() * resolution_constant;
    sonar_sim->setupShader(width, false);
    return (MultibeamSonarTaskBase::setBin_count(value));
}

bool MultibeamSonarTask::setBeam_count(int value) {
    if (value < 64 || value > 512) {
        RTT::log(RTT::Error) << "The number of beams must be between 64 and 512." << RTT::endlog();
        return false;
    }
    sonar_sim->setSonarBeamCount(value);
    return (MultibeamSonarTaskBase::setBeam_count(value));
}
