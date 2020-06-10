#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Task
{
	int id;
	int arrival;
	int period;
	int computation;
	int priority;
	xTaskHandle handle;
} Task;

void genericFunction(void *a_pvParameters);
void taskScheduler(void *a_pvParameters);
void sortAndPrioritize(Task a_tasksArr[], int a_size);
void printTasks(Task a_tasksArr[], int a_size);
void admitTask(Task a_newTask);
void deleteTask(int a_index);
double computeUtilization(Task a_tasksArr[], int a_size);
void synthesizeTasks();

int g_currTasks = 0;
Task *g_arrayOfTasks;
int g_maxTime = 0;
xTaskHandle g_schedulerHandle;

#define N 5
#define tst 1;
#define LATEST_ARRIVAL_TIME 15
#define MAXIMUM_COMPUTATION_TIME 8
#define MAXIMUM_PERIOD_MULTIPLER 17
#define SAFE_MODE 0
#define DYNAMIC_MODE 1

int main(void)
{
#if DYNAMIC_MODE == 1
	synthesizeTasks();
	printTasks(g_arrayOfTasks, g_currTasks);
	while (computeUtilization(g_arrayOfTasks, g_currTasks) > 0.7)
	{
		printf("Scheduling not possible\n");
		g_currTasks = 0;
		synthesizeTasks();
		printTasks(g_arrayOfTasks, g_currTasks);
	}
	printf("Scheduling OK\n\n");
	xTaskCreate(taskScheduler, "scheduler", configMINIMAL_STACK_SIZE, NULL, N + 1, &g_schedulerHandle);
#else
	int i = 0;
	synthesizeTasks();
	printTasks(g_arrayOfTasks, g_currTasks);
	while (computeUtilization(g_arrayOfTasks, g_currTasks) > 0.7)
	{
		printf("Scheduling not possible\n");
		g_currTasks = 0;
		synthesizeTasks();
		printTasks(g_arrayOfTasks, g_currTasks);
	}
	printf("Scheduling OK\n\n");
	g_maxTime = 150;
	for (i = 0; i < g_currTasks; i++)
	{
		printf("\t\t\t\tcreate task id:%d, ari:%d  pri: %d, com: %d, per: %d \n", g_arrayOfTasks[i].id, g_arrayOfTasks[i].arrival, g_arrayOfTasks[i].priority, g_arrayOfTasks[i].computation, g_arrayOfTasks[i].period);
		xTaskCreate(genericFunction, NULL, configMINIMAL_STACK_SIZE, (void *)&g_arrayOfTasks[i], g_arrayOfTasks[i].priority, &(g_arrayOfTasks[i].handle));
	}
	xTaskCreate(taskScheduler, "scheduler", configMINIMAL_STACK_SIZE, NULL, N + 1, &g_schedulerHandle);
#endif
	// printf("max %d\n", g_maxTime);

	vTaskStartScheduler();
	while (1)
		;
}

void taskScheduler(void *a_pvParameters)
{
	int i = 0;
	int newTaskId = N + 1;
	portTickType ticksTime = xTaskGetTickCount();
	while (1)
	{
		// if (ticksTime <= (g_maxTime + 1))
		while (g_currTasks)
		{
			vTaskSuspendAll();
			ticksTime = xTaskGetTickCount();
			printf("t = %d\n", ticksTime);
#if DYNAMIC_MODE == 1
			for (i = 0; i < g_currTasks; i++)
			{
				// Only start random deletion after after all tasks are created and have runned at least once
				if ((ticksTime > g_maxTime) && g_arrayOfTasks[i].handle && (rand() % 3 == 0))
				{
					Task taskToCreate;

					vTaskDelete(g_arrayOfTasks[i].handle);
					deleteTask(i);
					i--;

					taskToCreate.id = newTaskId++;
					taskToCreate.arrival = LATEST_ARRIVAL_TIME;
					taskToCreate.computation = (rand() % MAXIMUM_COMPUTATION_TIME) + 1;
					taskToCreate.handle = NULL;

#if SAFE_MODE == 1
					// Safe: random from 3xTc(i) to maximum_period_multipler x Tc(i)
					taskToCreate.period = ((rand() % (MAXIMUM_PERIOD_MULTIPLER - 2)) + 3) * taskToCreate.computation;
#else
					// No Guarantee: random from from 3xTc(i) to 10xTc(i)
					taskToCreate.period = ((rand() % (10 - 2)) + 3) * taskToCreate.computation;
#endif
					admitTask(taskToCreate);
					// computeUtilization(g_arrayOfTasks, g_currTasks);
				}
				if ((ticksTime >= g_arrayOfTasks[i].arrival) && (!g_arrayOfTasks[i].handle))
				{
					printf("\t\t\t\tcreate task id:%d, ari:%d  pri: %d, com: %d, per: %d \n", g_arrayOfTasks[i].id, g_arrayOfTasks[i].arrival, g_arrayOfTasks[i].priority, g_arrayOfTasks[i].computation, g_arrayOfTasks[i].period);
					xTaskCreate(genericFunction, NULL, configMINIMAL_STACK_SIZE, (void *)&g_arrayOfTasks[i], g_arrayOfTasks[i].priority, &(g_arrayOfTasks[i].handle));
				}
			}
			for (i = 0; i < g_currTasks; i++)
			{
				vTaskPrioritySet(g_arrayOfTasks[i].handle, g_arrayOfTasks[i].priority);
			}
#else
			if (ticksTime >= g_maxTime)
			{
				for (i = 0; i < g_currTasks; i++)
				{
					vTaskDelete(g_arrayOfTasks[i].handle);
					deleteTask(i);
					i--;
				}
			}
#endif
			xTaskResumeAll();
			vTaskSuspend(NULL);
		}
	}
}

void genericFunction(void *a_pvParameters)
{
	Task t = *((Task *)(a_pvParameters));
	portTickType tt = xTaskGetTickCount();
	int i = 0;
	while (1)
	{
		for (i = 0; i < t.computation; i++)
		{
			vTaskSuspendAll();
			printf("\t\ttask: %d running\n", t.id);
			xTaskResumeAll();
		}
		vTaskDelayUntil(&tt, t.period);
	}
}

static int myCompare(const void *a, const void *b)
{

	const Task *t1 = (Task *)a;
	const Task *t2 = (Task *)b;
	return t1->period < t2->period;
}

void sortAndPrioritize(Task a_tasksArr[], int a_size)
{
	int i = 0, lastPriority = 1;
	qsort(a_tasksArr, a_size, sizeof(Task), myCompare);

	for (i = 0; i < a_size - 1; i++)
	{
		if (a_tasksArr[i].period != a_tasksArr[i + 1].period)
			a_tasksArr[i].priority = lastPriority++;
		else
			a_tasksArr[i].priority = lastPriority;
	}
	a_tasksArr[i].priority = lastPriority;
}

double computeUtilization(Task a_tasksArr[], int a_size)
{
	int i = 0;
	double cpuUtilization = 0.0;
	for (i = 0; i < a_size; i++)
	{
		cpuUtilization += ((double)a_tasksArr[i].computation / (double)a_tasksArr[i].period);
	}
	printf("CPU Utilization: %lf \n", cpuUtilization);
	return cpuUtilization;
}

void printTasks(Task a_tasksArr[], int a_size)
{
	int i = 0;
	printf("\nTasks\n");
	for (i = 0; i < a_size; i++)
	{
		printf("id:%d, ari:%d  pri: %d, com: %d, per: %d \n", a_tasksArr[i].id, a_tasksArr[i].arrival, a_tasksArr[i].priority, a_tasksArr[i].computation, a_tasksArr[i].period);
	}
	printf("\n");
}

void admitTask(Task a_newTask)
{
	int i = 0;
	Task *temp = malloc((g_currTasks + 1) * sizeof(Task));

	vTaskSuspendAll();
	printf("Add task with id: %d, ", a_newTask.id);
	xTaskResumeAll();

	if (!temp)
		printf("Memmory Allocation Failed\n");
	else
		printf("Memmory Allocation OK\n");

	for (i = 0; i < g_currTasks; i++)
	{
		temp[i] = g_arrayOfTasks[i];
	}

	free(g_arrayOfTasks);
	g_arrayOfTasks = temp;
	g_arrayOfTasks[g_currTasks++] = a_newTask;
	if (a_newTask.arrival + a_newTask.period > g_maxTime)
		g_maxTime = a_newTask.arrival + a_newTask.period;
	sortAndPrioritize(g_arrayOfTasks, g_currTasks);
}

void deleteTask(int a_index)
{
	int i = 0;
	Task *temp = malloc((g_currTasks - 1) * sizeof(Task));

	printf("\t\t\t\tDelete task with id: %d, ", g_arrayOfTasks[a_index].id);

	if (!temp)
		printf("Memmory Deallocation Failed\n");
	else
		printf("Memmory Deallocation OK\n");

	for (i = 0; i < a_index; i++)
	{
		temp[i] = g_arrayOfTasks[i];
	}
	for (i = a_index + 1; i < g_currTasks; i++)
	{
		temp[i - 1] = g_arrayOfTasks[i];
	}

	free(g_arrayOfTasks);
	g_arrayOfTasks = temp;
	g_currTasks--;
	sortAndPrioritize(g_arrayOfTasks, g_currTasks);
}

void synthesizeTasks()
{

	int i = 0;
	Task tempTask;
	// srand(2);
	for (i = 0; i < N; i++)
	{
		tempTask.id = i + 1;

#if DYNAMIC_MODE == 1
		tempTask.arrival = rand() % (LATEST_ARRIVAL_TIME + 1);
		// random from 1 to maximum_computation_time
		tempTask.computation = (rand() % MAXIMUM_COMPUTATION_TIME) + 1;
#else
		tempTask.arrival = 0;
		tempTask.computation = i + 1;
#endif

#if SAFE_MODE == 1
		// Safe: random from 3xTc(i) to maximum_period_multipler x Tc(i)
		tempTask.period = ((rand() % (MAXIMUM_PERIOD_MULTIPLER - 2)) + 3) * tempTask.computation;
#else
		// No Guarantee: random from from 3xTc(i) to 10xTc(i)
		tempTask.period = ((rand() % (10 - 2)) + 3) * tempTask.computation;
#endif

		tempTask.handle = NULL;
		admitTask(tempTask);
	}
}

void vApplicationTickHook(void)
{
	vTaskResume(g_schedulerHandle);
}
