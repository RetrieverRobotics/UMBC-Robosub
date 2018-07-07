#include "DerivedTasks.hpp"

// ********************************
// QualifierGateEntry
// ********************************

QualifierGateEntry::QualifierGateEntry(ActionManager& _action_manager) : Task("QualifierGateEntry", _action_manager) {}

Task::task_return_t QualifierGateEntry::update(void) {
	switch(run_type) {
            case Task::RunType::Init:
                    break;
            case Task::RunType::Normal:
                    break;
            case Task::RunType::Stop:
                    break;
    }

	return task_return_t(Task::ReturnStatus::Continue, "");
}

// ********************************
// QualifierPin
// ********************************

QualifierPin::QualifierPin(ActionManager& _action_manager) : Task("QualifierPin", _action_manager) {}

Task::task_return_t QualifierPin::update(void) {
	switch(run_type) {
            case Task::RunType::Init:
                    break;
            case Task::RunType::Normal:
                    break;
            case Task::RunType::Stop:
                    break;
    }

	return task_return_t(Task::ReturnStatus::Continue, "");
}

// ********************************
// QualifierGateExit
// ********************************

QualifierGateExit::QualifierGateExit(ActionManager& _action_manager) : Task("QualifierGateExit", _action_manager) {}

Task::task_return_t QualifierGateExit::update(void) {
	switch(run_type) {
            case Task::RunType::Init:
                    break;
            case Task::RunType::Normal:
                    break;
            case Task::RunType::Stop:
                    break;
    }

	return task_return_t(Task::ReturnStatus::Continue, "");
}

// ********************************
// SurfaceAndWait
// ********************************

SurfaceAndWait::SurfaceAndWait(ActionManager& _action_manager) : Task("SurfaceAndWait", _action_manager) {}

Task::task_return_t SurfaceAndWait::update(void) {
	switch(run_type) {
            case Task::RunType::Init:
                    break;
            case Task::RunType::Normal:
                    break;
            case Task::RunType::Stop:
                    break;
    }

	return task_return_t(Task::ReturnStatus::Continue, "");
}