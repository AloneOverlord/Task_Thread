#pragma once
#include<iostream>
#include<queue>
#include<string>
#include<functional>
#include<condition_variable>
#include<vector>
#include<mutex>
#include<thread>
#include<winsock2.h>
class TaskQue
{
private:
	//线程池
	std::vector<std::thread> works;
	//任务列表
	std::queue<std::function<void()>> tasks;
	//锁
	std::mutex mtx;
	//条件变量
	std::condition_variable cv;
	//任务队列运行标志
	bool open;
	//专门说话的锁
	inline static std::mutex cout_mtx;
	friend void testTask(const std::string& string);
public:
	TaskQue(int num_works) :open(true) {
		//创建线程
		for (int i = 0; i < num_works; i++) {
			//把线程构建到容器中，使用lambda可以实现小区域函数
			works.emplace_back([this] {
				//每一个线程需要一直循环看是否等待
				while (true) {
					//构建一个空函数
					std::function<void()> task;
					{
						//锁住查看是否继续等待
						std::unique_lock<std::mutex> lock(mtx);
						//条件变量，如果队列为空或者处于打开状态就继续等待
						cv.wait(lock, [this] {return !tasks.empty() || !open; });
						//接下来是队列不为空的执行情况
						//先判断是否队列为空且关闭状态，关闭的状态就该退出while循环
						if (!open && tasks.empty()) {
							return;
						}
						//如果是有任务且打开状态
						task = tasks.front();
						tasks.pop();
					}
					//执行任务
					std::this_thread::sleep_for(std::chrono::seconds(1));
					task();
				}
			});
		}
	}
	//添加任务
	void addTask(std::function<void()> task) {
		{
			//上锁
			std::unique_lock<std::mutex> lock(mtx);
			//任务进入队列
			tasks.push(task);
			std::cout << "new task!!!现在任务数量：" << tasks.size() << std::endl;
		}
		//通知一个线程有任务了
		cv.notify_one();
	}
	//关闭线程池任务队列
	void closeTask() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			open = false;
		}
		cv.notify_all();
	}
	~TaskQue() {
		closeTask();
		for (auto& work : works) {
			if (work.joinable()) {
				work.join();
			}
		}
	}
	
};
//std::mutex TaskQue::cout_mtx;
void testTask(const std::string & string) {
	std::lock_guard<std::mutex> lock(TaskQue::cout_mtx);
	std::cout << "线程：" << std::this_thread::get_id << "正在运行" <<string<< std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	//std::cout << "线程：" << std::this_thread::get_id << "运行结束!!!!" << std::endl;
}

/*
 int main() {
	TaskQue ta(5);
	ta.addTask([] {return testTask("01"); });
	ta.addTask([] {return testTask("02"); });
	ta.addTask([] {return testTask("03"); });
	ta.addTask([] {return testTask("04"); });
	std::this_thread::sleep_for(std::chrono::seconds(5));
	std::cout << "close" << std::endl;
}
 */