#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#include "ActionManager.hpp"
#include "TaskManager.hpp"

#include "tasks.cpp_nc"

using namespace std;
using namespace task_function;

int main(int argc, char* argv[]) {
	ActionManager action_manager;

	TaskManager task_manager(action_manager);

	cout << task_manager.getFullName() << endl;

	task_manager.registerTask("QualifierGateEntry", f_TaskQualifierGateEntry);
	task_manager.registerTask("QualifierPin", f_TaskQualifierPin);
	task_manager.registerTask("QualifierGateExit", f_TaskQualifierGateExit);
	task_manager.registerTask("SurfaceAndWait", f_TaskSurfaceAndWait);

	task_manager.onStart("QualifierGateEntry, DoesNotExist");
	task_manager.onBlock("SurfaceAndWait");
	task_manager.configureTree(
		"[QualifierGateEntry ? QualifierPin : SurfaceAndWait]"
		"[QualifierPin ? QualifierGateExit : SurfaceAndWait]"
		"[DoesNotExist ? SurfaceAndWait]"
		"[QualifierPin ? DoesNotExist]"
		"[QualifierPin ? DoesNotExist_S : DoesNotExist_F]"
	);

	cout << task_manager.listTasks();

	task_manager.start();

	while(task_manager.tasksRunning()) {
		task_manager.update();
		cout << task_manager.listTasks();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	return 0;
}