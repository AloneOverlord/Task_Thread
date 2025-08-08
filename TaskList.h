#pragma once
//任务队列
#include<iostream>
#include<queue>
#include<functional>
#include<condition_variable>
#include<mutex>
#include<vector>
#include<thread>
#include<string>
class Task
{
private:
	//任务队列
	std::queue < std::function<void()>> tasks;
	//任务线程池
	std::vector<std::thread> works;
	//互斥锁
	std::mutex mtx;
	//条件变量
	std::condition_variable cv;
	//队列开启情况
	bool open;
	//专门说话的锁
	inline static std::mutex cout_mtx;

public:
	//构造函数
	Task(int work_nums):open(true){
		//构造线程池数量
		for (int i = 0; i < work_nums; i++) {
			works.emplace_back([this] {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(mtx);
						//进入等待
						cv.wait(lock, [this] {return !tasks.empty() || !open; });
						//判断是否是因为队列关闭
						if (tasks.empty() && !open) {
							return;//关闭线程
						}
						task = tasks.front();
						tasks.pop();
					}
					//std::this_thread::sleep_for(std::chrono::seconds(1));
					task();
				}
				});
		}
	}
	void addTasks(std::function<void()> task) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.push(task);
		}
		cv.notify_one();
	}
	void closeTasks() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			open = false;
		}
		cv.notify_all();
	}
	~Task() {
		closeTasks();
		for (auto& work : works) {
			if (work.joinable()) {
				work.join();
			}
		}
	}
	friend void TT(std::string string);
};


void TT(std::string string) {
	std::lock_guard<std::mutex> lock(Task::cout_mtx);
	std::cout << std::this_thread::get_id() << "正在处理" << string << std::endl;
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//std::cout<< std::this_thread::get_id() << "处理完成" << std::endl;
}

/*
*测试用例
Task T(3);
	for (int i = 0; i < 10; i++) {
		std::string temp;
		temp = i;
		T.addTasks([temp] {return TT(temp); });
	}
*/