#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>

#include "plog/Log.h"
#include "plog/Appenders/ConsoleAppender.h"

#include "ThreadManager.hpp"
#include "TaskManager.hpp"

#include "Comms/Comms.hpp"
#include "Comms/Links/DummyLink.hpp"
#include "Comms/Links/USBSerialLink.hpp"
#include "TimeLord.hpp"

#include "DerivedTasks.hpp"

using namespace std;	

void comms_test(Comms&);


//		#  #   ##   ###   #  #
//		####  #  #   #    ## #
//		####  #  #   #    # ##
//		#  #  ####   #    #  #
//		#  #  #  #  ###   #  #


int main(int argc, char* argv[]) {
	LOG_INFO << "Electrifying Brain :)";

	int opt;
	bool TEST_comms = false;
	bool use_usb = false;
	while( (opt = getopt(argc, argv, "tu")) != -1) {
		switch(opt) {
			case 't':
				TEST_comms = true;
				LOG_INFO << "-t = Tests enabled";
				break;
			case 'u':
				use_usb = true;
				LOG_INFO << "-u = Using USB device.";
				break;
		}
	}

	static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::debug, "logs/log.txt").addAppender(&consoleAppender);

	//std::cout << USBSerialLink::stringifyPorts() << std::endl;

	Comms comms;
	comms.addLink("pi", std::make_shared<DummyLink>(), Comms::CopyLocal);
	//comms.addLink("teensy", std::make_shared<USBSerialLink>("/dev/cu.usbmodem848141", 115200));
	if(use_usb) comms.addLink("teensy", std::make_shared<USBSerialLink>("/dev/cu.usbmodem2753871", 115200));

	if(TEST_comms) comms_test(comms); // hacky way to break code out of main - would prefer a separate file but don't know how to write the makefile for this

	ThreadManager thread_manager;
	TaskManager task_manager;

	task_manager.registerTask(std::make_shared<CommsDaemon>(thread_manager, comms));
	task_manager.registerTask(std::make_shared<EStopDaemon>(thread_manager, comms));
	task_manager.registerTask(std::make_shared<WaitForStart>(thread_manager, comms));
	task_manager.registerTask(std::make_shared<Submerge>(thread_manager, comms));
	task_manager.registerTask(std::make_shared<ValidationGate>(thread_manager, comms));
	task_manager.registerTask(std::make_shared<SurfaceAndWait>(thread_manager, comms));

	task_manager.onStart("EStopDaemon, CommsDaemon, WaitForStart");
	task_manager.configureTree(
		"[WaitForStart ? Submerge]"
		"[Submerge ? ValidationGate : SurfaceAndWait]"
		"[ValidationGate ? SurfaceAndWait : SurfaceAndWait]"
		"[EStopDaemon : SurfaceAndWait]"
	);

	cout << task_manager.listTasks();

	task_manager.start();
	cout << task_manager.listTasks();

	TimeStamp cmdline_ts;

	while(task_manager.tasksRunning()) {
		task_manager.update();
		//cout << task_manager.listTasks();
		if(comms.hasNew("pi", "cmdline", cmdline_ts.getTimePoint())) {
			cmdline_ts.touch();
			std::string cmd = comms.get<std::string>("pi", "cmdline");
			if(cmd == "tasks") {
				cout << task_manager.listTasks();
			} else if(cmd == "threads") {
				cout << thread_manager.listThreads();
			} else if(cmd == "kill" || cmd == "quit") {
				comms.send("teensy", "cmd", comms_util::Hint::String, std::string("stop"));
				task_manager.killAll();
			} else if(cmd.find("configure") != std::string::npos) {
				task_manager.configureTree(cmd.substr(cmd.find("configure")+1));
			} else if(cmd == "ESTOP" || cmd == "SAFE") {
				comms.send("teensy", "cmd", comms_util::Hint::String, cmd);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	
	return 0;
}


//		#####  ####   ### #####        ####  #  #  #  #   ### #####  ###    ##   #  #   ###
//		  #    #     #      #          #     #  #  ## #  #      #     #    #  #  ## #  #
//		  #    ###    ##    #          ###   #  #  # ##  #      #     #    #  #  # ##   ##
//		  #    #        #   #          #     #  #  #  #  #      #     #    #  #  #  #     #
//		  #    ####  ###    #          #      ##   #  #   ###   #    ###    ##   #  #  ###


void comms_test(Comms& comms) {
	LOG_INFO << "Starting comms_test()";

	LOG_INFO << "Test: Send and retrieve <double> on DummyLink";
	comms.send("pi", "Submerge", comms_util::Hint::Double, (double)1020.0);
	LOG_INFO << comms.get<double>("pi", "Submerge");

	LOG_INFO << "Test: Send and retrieve <float> on DummyLink";
	comms.send("pi", "Submerge", comms_util::Hint::Other, (float)105.2343);
	LOG_INFO << comms.get<float>("pi", "Submerge");
	
	LOG_INFO << "Test: Type check of field 'Submerge' in link 'pi'...";
	if(comms.isSetAs<int>("pi", "Submerge")) {
		LOG_INFO << "\t<int> : " << comms.get<int>("pi", "Submerge");
	} else {
		LOG_INFO << "\tnot <int>";
	}
	
	LOG_INFO << "Test: Field name does not exist";
	LOG_INFO << "\t" << comms.get<int>("pi", "submerge");

	LOG_INFO << "Test: Send a few fields to a USB serial device.";
	comms.send("teensy", "msg", comms_util::Hint::String, std::string("Hello world!!"));
	comms.send("teensy", "depth_target", comms_util::Hint::Double, (double)1015.43);

	comms.send("teensy", "double vector", comms_util::Hint::DoubleVector, std::vector<double>(3.0, 4.0));

	if(comms.linkExists("teensy")) {
		LOG_INFO << "Test: Begin interactive prompt.";
		TimeStamp result_ts;
		TimeStamp time_ts;
		while(true) {
			comms.receive("teensy");
			if(comms.hasNew("teensy", "result", time_ts.getTimePoint())) {
				time_ts.touch();

				LOG_DEBUG << ">> " << comms.get<std::string>("teensy", "result");
			}
			if(comms.hasNew("teensy", "millis", result_ts.getTimePoint())) {
				result_ts.touch();
				LOG_DEBUG << "@ " << comms.get<int>("teensy", "millis");
			}

			std::string input;
			getline(cin, input);
			if(input == "break") break;
			if(input != "") comms.send("teensy", "cmd", comms_util::Hint::String, input);

			std::this_thread::sleep_for(std::chrono::milliseconds(50)); // nice sedate 20 Hz
		}
	} else {
		LOG_WARNING << "Link 'teensy' is unavailable - skipping interactive interpreter";
	}
	
}