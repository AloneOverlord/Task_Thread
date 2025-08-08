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
	//�̳߳�
	std::vector<std::thread> works;
	//�����б�
	std::queue<std::function<void()>> tasks;
	//��
	std::mutex mtx;
	//��������
	std::condition_variable cv;
	//����������б�־
	bool open;
	//ר��˵������
	inline static std::mutex cout_mtx;
	friend void testTask(const std::string& string);
public:
	TaskQue(int num_works) :open(true) {
		//�����߳�
		for (int i = 0; i < num_works; i++) {
			//���̹߳����������У�ʹ��lambda����ʵ��С������
			works.emplace_back([this] {
				//ÿһ���߳���Ҫһֱѭ�����Ƿ�ȴ�
				while (true) {
					//����һ���պ���
					std::function<void()> task;
					{
						//��ס�鿴�Ƿ�����ȴ�
						std::unique_lock<std::mutex> lock(mtx);
						//�����������������Ϊ�ջ��ߴ��ڴ�״̬�ͼ����ȴ�
						cv.wait(lock, [this] {return !tasks.empty() || !open; });
						//�������Ƕ��в�Ϊ�յ�ִ�����
						//���ж��Ƿ����Ϊ���ҹر�״̬���رյ�״̬�͸��˳�whileѭ��
						if (!open && tasks.empty()) {
							return;
						}
						//������������Ҵ�״̬
						task = tasks.front();
						tasks.pop();
					}
					//ִ������
					std::this_thread::sleep_for(std::chrono::seconds(1));
					task();
				}
			});
		}
	}
	//�������
	void addTask(std::function<void()> task) {
		{
			//����
			std::unique_lock<std::mutex> lock(mtx);
			//����������
			tasks.push(task);
			std::cout << "new task!!!��������������" << tasks.size() << std::endl;
		}
		//֪ͨһ���߳���������
		cv.notify_one();
	}
	//�ر��̳߳��������
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
	std::cout << "�̣߳�" << std::this_thread::get_id << "��������" <<string<< std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	//std::cout << "�̣߳�" << std::this_thread::get_id << "���н���!!!!" << std::endl;
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