#pragma once
//任务队列
#include<iostream>
#include<queue>
#include<mutex>
#include<thread>
#include<condition_variable>
#include<functional>
#include<vector>
class Task
{
private:
	//任务队列
	std::queue<std::function<void()>> tasks;
	//锁
	std::mutex mtx;
	//条件变量
	std::condition_variable cv;
	//任务队列开启状态
	bool open;
public:
	//构造函数
	Task() :open(true) {};
	//添加任务
	void addTasks(std::function<void()> task) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push(task);
		}
		cv.notify_one();
	}
	//获取任务
	bool getTasks(std::function<void()>& task) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] {return !open || !tasks.empty();});
			if (tasks.empty() && !open) {
				return false;
			}
			task = tasks.front();
			tasks.pop();
			return true;
		}
	}
	//关闭队列
	void closeTasks() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			open = false;
		}
		cv.notify_all();
	}
};

class ThreadPool
{
private:
	//线程池
	std::vector<std::thread> works;
	//线程池运行状态
	bool work_state;
	//线程池负责的任务队列
	Task& taskQ;

public:
	//构建函数创建线程池
	ThreadPool(Task& tasks, int num_works) :taskQ(tasks), work_state(true) {
		for (int i = 0; i < num_works; i++) {
			works.emplace_back([this] {
				while (work_state) {
				std::function<void()> task;
					if (taskQ.getTasks(task)) {
						task();
					}
					else {
						return;
					}
				}
				});
		}
	}
	void closeworks() {
		work_state = false;
		taskQ.closeTasks();
		for (auto& work : works) {
			if (work.joinable()) {
				work.join();
			}
		}
	}
	~ThreadPool() {
		closeworks();
	}
};