#include "DerivedTasks.hpp"

#include <thread>
#include <cmath>

/*
Example implementation:

// ********************************
// XXX
// ********************************

XXX::XXX(ThreadManager& _t_m, Comms& _c) : Task("XXX", _t_m, _c) {}

const Task::Result XXX::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            break;
        case RunType::Normal:
            break;
        case RunType::Stop:
            break;
    }
    return Task::Result(ReturnStatus::Continue, "");
}

*/

// ********************************
// WaitForStart
// ********************************

WaitForStart::WaitForStart(ThreadManager& _t_m, Comms& _c) : Task("WaitForStart", _t_m, _c), timeout(2) {}

const Task::Result WaitForStart::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            //timeout.reset();
            break;

        case RunType::Normal:
            if(timeout.timedOut()) return Task::Result(ReturnStatus::Failure, "TIMEOUT... which shouldn't happen, but demo reasons");

            if(comms.isSet("pi", "cmdline") && comms.get<std::string>("pi", "cmdline").find("start") != std::string::npos) {
                return Task::Result(ReturnStatus::Success, "Starting...");
            }
            break;

        case RunType::Stop:
            unloadAll();
            break;
    }

    return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// Submerge
// ********************************

Submerge::Submerge(ThreadManager& _t_m, Comms& _c) : Task("Submerge", _t_m, _c),
    timeout(20), pressure_target(1050), pressure_tolerance(3) {}

const Task::Result Submerge::update(void) {
	switch(getRunType()) {
        case RunType::Init:
            comms.send("teensy", "cmd", comms_util::Hint::String, "config:depth:" + std::to_string(pressure_target));
            depth_ts.touch();
            timeout.reset();
            break;

        case RunType::Normal:
        	if(comms.hasNew("teensy", "data_pressure", depth_ts.getTimePoint())) {
                depth_ts.touch();

                float pressure_current = comms.get<float>("teensy", "data_pressure");
                if(fabs(pressure_current - pressure_target) < pressure_tolerance) {
                    return Task::Result(ReturnStatus::Success, "Submerged to " + std::to_string(pressure_target));
                }
            }
            if(timeout.timedOut()) {
                return Task::Result(ReturnStatus::Failure, "TIMEOUT");
            }
            break;

        case RunType::Stop:
        	unloadAll();
            break;
    }

	return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// ValidationGate
// ********************************

ValidationGate::ValidationGate(ThreadManager& _t_m, Comms& _c) : Task("ValidationGate", _t_m, _c) {}

const Task::Result ValidationGate::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            break;
        case RunType::Normal:
            break;
        case RunType::Stop:
            break;
    }
    return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// SurfaceAndWait
// ********************************

SurfaceAndWait::SurfaceAndWait(ThreadManager& _t_m, Comms& _c) : Task("SurfaceAndWait", _t_m, _c) {}

const Task::Result SurfaceAndWait::update(void) {
	switch(getRunType()) {
        case RunType::Init:
            comms.send("teensy", "cmd", comms_util::Hint::String, std::string("stop"));
            break;

        case RunType::Normal:
            break;
        case RunType::Stop:
        	unloadAll();
            break;
    }

	return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// EStopDaemon
// ********************************

EStopDaemon::EStopDaemon(ThreadManager& _t_m, Comms& _c) : Task("EStopDaemon", _t_m, _c) {}

const Task::Result EStopDaemon::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            break;
        case RunType::Normal:
            // check to see if estop bool is set in comms
            if(comms.isSetAs<bool>("teensy", "ESTOP")) {
                // thread_manager.unloadWorkers();
                while(true) { // prevent state machine from spawning more threads when existing threads exit
                    //if(thread_manager.threadCount() == 0) return Task::Result(ReturnStatus::Success, "All threads stopped. Ready to reset.");
                    std::cout << "Estop: Waiting for threads to complete...";
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
            // at that point, the microcontroller would already have disabled all actuators
            // all that remains is to get the pi back to a safe state and send a safe signal to the microcontroller
            // keep updating thread_manager in a blocking loop until there are no more worker threads computing
            // then send reset signal to microcontroller
            // and return Task::Result(ReturnState::Success, "reset complete");
            if(comms.isSet("pi", "cmdline") && comms.get<std::string>("pi", "cmdline").find("estop()") != std::string::npos) {
                comms.send("teensy", "cmd", comms_util::Hint::String, "ESTOP");
                return Task::Result(ReturnStatus::Success, "");
            }
            break;
        case RunType::Stop:
            unloadAll();
            break;
    }

    return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// CommsDaemon
// ********************************

CommsDaemon::CommsDaemon(ThreadManager& _t_m, Comms& _c)
    : Task("CommsDaemon", _t_m, _c), keep_alive(1, true), keep_alive_ticker(0) {}

const Task::Result CommsDaemon::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            load(a_interpreter, "interpreter", th_man::RunLevel::Critical);
            doStuff();
            break;
        case RunType::Normal:
            if(status(a_interpreter) == th_man::ThreadStatus::Paused) {
                comms.send("pi", "cmdline", comms_util::Hint::String, a_interpreter.getStr());
                resume(a_interpreter);
            }
            doStuff();
            break;
        case RunType::Stop:
            unloadAll();
            break;
    }

    return Task::Result(ReturnStatus::Continue, "");
}

void CommsDaemon::doStuff(void) {
    if(keep_alive.timedOut()) { // increment and send an integer under the ALIVE field every 1 second ( or according to timeout )
        keep_alive.reset();
        comms.send("teensy", "ALIVE", comms_util::Hint::Int, (int)keep_alive_ticker);
        ++keep_alive_ticker;
    }
    
    comms.receiveAll();
}

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// DEFERRED - these will not be used this year
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ********************************
// QualifierGateEntry
// ********************************

QualifierGateEntry::QualifierGateEntry(ThreadManager& _t_m, Comms& _c) : Task("QualifierGateEntry", _t_m, _c) {}

const Task::Result QualifierGateEntry::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            load(a_search, "search", th_man::RunLevel::Worker);
            break;
        case RunType::Normal:
            if(status(a_search) == th_man::ThreadStatus::Paused) {
                std::string where_to = a_search.getDirection();
                
                if(where_to.find("locked") != std::string::npos) {
                    unload(a_search);
                    load(a_move, "move", th_man::RunLevel::Worker);
                } else {
                    resume(a_search);
                }
                
            }
            if(status(a_move) == th_man::ThreadStatus::Paused) {
                resume(a_move);
            }
            break;
        case RunType::Stop:
            unloadAll();
            break;
    }

    return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// QualifierPin
// ********************************

QualifierPin::QualifierPin(ThreadManager& _t_m, Comms& _c) : Task("QualifierPin", _t_m, _c) {}

const Task::Result QualifierPin::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            break;
        case RunType::Normal:
            break;
        case RunType::Stop:
            unloadAll();
            break;
    }

    return Task::Result(ReturnStatus::Continue, "");
}

// ********************************
// QualifierGateExit
// ********************************

QualifierGateExit::QualifierGateExit(ThreadManager& _t_m, Comms& _c) : Task("QualifierGateExit", _t_m, _c) {}

const Task::Result QualifierGateExit::update(void) {
    switch(getRunType()) {
        case RunType::Init:
            break;
        case RunType::Normal:
            break;
        case RunType::Stop:
            unloadAll();
            break;
    }

    return Task::Result(ReturnStatus::Continue, "");
}