#pragma once
//ͷ�ļ�Ϊ   ������� �� �̳߳� �ֿ�д
//�������
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
//�̳߳�
#include<thread>
#include<vector>
class Tasks {
private:
	//�������
	std::queue < std::function<void()>> tasks;
	//��������
	std::condition_variable cv;
	//��
	std::mutex mtx;
	//���п���״̬
	bool open;
public:
	Tasks() :open(true) {};
	//�������
	void addTasks(std::function<void()> task) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			tasks.push(task);
		}
		cv.notify_one();
	}
	//��ȡ����
	bool getTasks(std::function<void()>& task) {
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] {return !tasks.empty() || !open; });
			if (tasks.empty() && !open)return false;
			task = tasks.front();
			tasks.pop();
			return true;
	}
	//�رն���
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
	//ʹ�õ��������
	Tasks& TaskQue;
	//�߳�����
	std::vector<std::thread> works;
	//�߳�״̬
	bool work_state;

public:
	//���캯��
	ThreadPool(Tasks& TaskQ, int num_works):TaskQue(TaskQ),work_state(true) {
		//�����߳�
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
	//�ر��̳߳�
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
��������
int main() {
	Tasks tasks;
	ThreadPool pool(tasks, 3);
	std::mutex mtx_cout;
	for (int i = 0; i < 5; i++) {
		tasks.addTasks([i,&mtx_cout] {
			std::lock_guard<std::mutex> lock(mtx_cout);
			std::cout << "����" << i << "��ʼִ��" << std::endl;
			});
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
*/