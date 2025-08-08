#pragma once
//任务队列需要的头文件
#include<queue>
#include<condition_variable>
#include<mutex>
#include<functional>
#include<atomic>
//线程池
#include<vector>
#include<thread>a
class Task
{
private:
	//任务队列
	std::queue<std::function<void()>> tasks;
	//任务锁
	std::mutex mtx;
	//条件变量
	std::condition_variable cv;
	//任务开启状态
	bool open_tasks;
	//任务完成通知
	std::condition_variable comple_cv;
	//任务完成锁
	std::mutex comple_mtx;
	//任务数和执行数
	std::atomic<int> pending{ 0 };
	std::atomic<int> running{ 0 };
	

public:
	//构建函数
	Task() :open_tasks(true) {};
	//添加任务
	void addTasks(std::function<void()> task) {
		{
			//获取锁然后把任务压入队列
			std::unique_lock<std::mutex> lock(mtx);
			auto task_plus =  [this,task] {
				task();
				running--;
				// 如果没有任务了，通知等待的线程
				if (pending == 0 && running == 0) {
					std::unique_lock<std::mutex> completion_lock(comple_mtx);
					comple_cv.notify_all();
				} } ;
			tasks.push(task_plus);
			pending++;
		}
		//任务进入队列后通知一下
		cv.notify_one();
	}
	//获取任务
	bool getTasks(std::function<void()>& task) {
		{
			//先进行上锁防止多个线程抢任务
			std::unique_lock<std::mutex> lock(mtx);
			//条件变量进行阻塞(return中的意思是当任务队列不为空继续执行下面代码
			cv.wait(lock, [this] {return !tasks.empty() || !open_tasks; });
			//再判断是否为假唤醒
			if (tasks.empty() && !open_tasks) {
				return false;//确定不是假唤醒，并且任务队列关闭
			}
			//确定不是假唤醒就把队列的任务传给参数
			task = tasks.front();
			//任务出队列
			tasks.pop();
			pending--;
			running++;
			return true;
		}
	}
	//关闭任务队列
	void closeTasks() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			open_tasks = false;
		}
		cv.notify_all();
	}
	void waitForCompletion() {
		std::unique_lock<std::mutex> lock(comple_mtx);
		// 等待条件：没有等待的任务，也没有正在运行的任务
		comple_cv.wait(lock, [this] {
			return pending == 0 && running == 0;
			});
	}
};

//线程池部分
class ThreadPool
{
private:
	//线程池
	std::vector<std::thread> works;
	//线程池处理的任务队列
	Task& tasks;
	//线程池运行状态
	bool open_pool;

public:
	//构建函数
	ThreadPool(Task& task, int num_works) :tasks(task), open_pool(true) {
		//循环创建线程
		for (int i = 0; i < num_works; i++) {
			//用emplace_back把线程压入容器中
			works.emplace_back([this] {
				//线程不断循环
				while (open_pool) {
					//创建一个函数
					std::function<void()> task;
					//从任务队列中获取任务函数
					if (tasks.getTasks(task)) {
						task();
					}
					else {
						return;
					}
				}
				});
		}
	}
	//关闭线程池
	void closePool() {
		open_pool = false;
		tasks.closeTasks();
		//检查所有线程是否运行完毕
		for (auto& work : works) {
			if (work.joinable()) {
				work.join();
			}
		}
	}
	~ThreadPool() {
		closePool();
	}
};