#pragma once
//�������
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
	//�������
	std::queue<std::function<void()>> tasks;
	//��
	std::mutex mtx;
	//��������
	std::condition_variable cv;
	//������п���״̬
	bool open;
public:
	//���캯��
	Task() :open(true) {};
	//�������
	void addTasks(std::function<void()> task) {
		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push(task);
		}
		cv.notify_one();
	}
	//��ȡ����
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
	//�رն���
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
	//�̳߳�
	std::vector<std::thread> works;
	//�̳߳�����״̬
	bool work_state;
	//�̳߳ظ�����������
	Task& taskQ;

public:
	//�������������̳߳�
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