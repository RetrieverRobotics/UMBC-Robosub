#ifndef DERIVED_TASKS_H
#define DERIVED_TASKS_H

#include "ActionManager.hpp"
#include "Task.hpp"

/*
Example declaration:

class XXX : public Task {
public:
	explicit XXX(ActionManager&);

	Task::task_return_t update(void);
};
*/


class QualifierGateEntry : public Task {
public:
	explicit QualifierGateEntry(ActionManager&);

	Task::task_return_t update(void);
private:

};

class QualifierPin : public Task {
public:
	explicit QualifierPin(ActionManager&);

	Task::task_return_t update(void);
};

class QualifierGateExit : public Task {
public:
	explicit QualifierGateExit(ActionManager&);

	Task::task_return_t update(void);
};

class SurfaceAndWait : public Task {
public:
	explicit SurfaceAndWait(ActionManager&);

	Task::task_return_t update(void);
};

#endif