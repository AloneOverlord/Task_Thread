#pragma once
//�������
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
	//�������
	std::queue < std::function<void()>> tasks;
	//�����̳߳�
	std::vector<std::thread> works;
	//������
	std::mutex mtx;
	//��������
	std::condition_variable cv;
	//���п������
	bool open;
	//ר��˵������
	inline static std::mutex cout_mtx;

public:
	//���캯��
	Task(int work_nums):open(true){
		//�����̳߳�����
		for (int i = 0; i < work_nums; i++) {
			works.emplace_back([this] {
				while (true) {
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(mtx);
						//����ȴ�
						cv.wait(lock, [this] {return !tasks.empty() || !open; });
						//�ж��Ƿ�����Ϊ���йر�
						if (tasks.empty() && !open) {
							return;//�ر��߳�
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
	std::cout << std::this_thread::get_id() << "���ڴ���" << string << std::endl;
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	//std::cout<< std::this_thread::get_id() << "�������" << std::endl;
}

/*
*��������
Task T(3);
	for (int i = 0; i < 10; i++) {
		std::string temp;
		temp = i;
		T.addTasks([temp] {return TT(temp); });
	}
*/