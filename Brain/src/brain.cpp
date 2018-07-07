#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#include "ActionManager.hpp"
#include "TaskManager.hpp"

#include "DerivedTasks.hpp"

using namespace std;

int main(int argc, char* argv[]) {
	ActionManager action_manager;

	TaskManager task_manager;

	cout << task_manager.getFullName() << endl;

	Task* qge = new QualifierGateEntry(action_manager);
	task_manager.registerTask(*qge);

	cout << qge->getFullName() << endl;

	task_manager.registerTask(*(new QualifierPin(action_manager)));
	task_manager.registerTask(*(new QualifierGateExit(action_manager)));
	task_manager.registerTask(*(new SurfaceAndWait(action_manager)));

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