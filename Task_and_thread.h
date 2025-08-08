#pragma once
//头文件为   任务队列 和 线程池 分开写
//任务队列
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
//线程池
#include<thread>
#include<vector>
class Tasks {
private:
	//任务队列
	std::queue < std::function<void()>> tasks;
	//条件变量
	std::condition_variable cv;
	//锁
	std::mutex mtx;
	//队列开启状态
	bool open;
public:
	Tasks() :open(true) {};
	//添加任务
	void addTasks(std::function<void()> task) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.push(task);
		}
		cv.notify_one();
	}
	//获取任务
	bool getTasks(std::function<void()>& task) {
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] {return !tasks.empty() || !open; });
			if (tasks.empty() && !open)return false;
			task = tasks.front();
			tasks.pop();
			return true;
	}
	//关闭队列
	void closeTask() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			open = false;
		}
		cv.notify_all();
	}
};

class ThreadPool {
private:
	//使用的任务队列
	Tasks& TaskQue;
	//线程容器
	std::vector<std::thread> works;
	//线程状态
	bool work_state;

public:
	//构造函数
	ThreadPool(Tasks& TaskQ, int num_works):TaskQue(TaskQ),work_state(true) {
		//创建线程
		for (int i = 0; i < num_works; i++) {
			works.emplace_back([this] {
				while (work_state) {
					std::function<void()> task;
					if (TaskQue.getTasks(task)) 
						{
						task();
						}
					else 
						{
						return;
						}
				}
			});
		}
	}
	//关闭线程池
	void closeThread() {
		work_state = false;
		TaskQue.closeTask();
		for (auto& work : works) {
			if (work.joinable()) {
				work.join();
			}
		}
	}
	~ThreadPool() {
		closeThread();
	}
};

/*
测试用例
int main() {
	Tasks tasks;
	ThreadPool pool(tasks, 3);
	std::mutex mtx_cout;
	for (int i = 0; i < 5; i++) {
		tasks.addTasks([i,&mtx_cout] {
			std::lock_guard<std::mutex> lock(mtx_cout);
			std::cout << "任务：" << i << "开始执行" << std::endl;
			});
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
*/