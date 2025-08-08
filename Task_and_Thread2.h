#pragma once
//���������Ҫ��ͷ�ļ�
#include<queue>
#include<condition_variable>
#include<mutex>
#include<functional>
#include<atomic>
//�̳߳�
#include<vector>
#include<thread>a
class Task
{
private:
	//�������
	std::queue<std::function<void()>> tasks;
	//������
	std::mutex mtx;
	//��������
	std::condition_variable cv;
	//������״̬
	bool open_tasks;
	//�������֪ͨ
	std::condition_variable comple_cv;
	//���������
	std::mutex comple_mtx;
	//��������ִ����
	std::atomic<int> pending{ 0 };
	std::atomic<int> running{ 0 };
	

public:
	//��������
	Task() :open_tasks(true) {};
	//�������
	void addTasks(std::function<void()> task) {
		{
			//��ȡ��Ȼ�������ѹ�����
			std::unique_lock<std::mutex> lock(mtx);
			auto task_plus =  [this,task] {
				task();
				running--;
				// ���û�������ˣ�֪ͨ�ȴ����߳�
				if (pending == 0 && running == 0) {
					std::unique_lock<std::mutex> completion_lock(comple_mtx);
					comple_cv.notify_all();
				} } ;
			tasks.push(task_plus);
			pending++;
		}
		//���������к�֪ͨһ��
		cv.notify_one();
	}
	//��ȡ����
	bool getTasks(std::function<void()>& task) {
		{
			//�Ƚ���������ֹ����߳�������
			std::unique_lock<std::mutex> lock(mtx);
			//����������������(return�е���˼�ǵ�������в�Ϊ�ռ���ִ���������
			cv.wait(lock, [this] {return !tasks.empty() || !open_tasks; });
			//���ж��Ƿ�Ϊ�ٻ���
			if (tasks.empty() && !open_tasks) {
				return false;//ȷ�����Ǽٻ��ѣ�����������йر�
			}
			//ȷ�����Ǽٻ��ѾͰѶ��е����񴫸�����
			task = tasks.front();
			//���������
			tasks.pop();
			pending--;
			running++;
			return true;
		}
	}
	//�ر��������
	void closeTasks() {
		{
			std::lock_guard<std::mutex> lock(mtx);
			open_tasks = false;
		}
		cv.notify_all();
	}
	void waitForCompletion() {
		std::unique_lock<std::mutex> lock(comple_mtx);
		// �ȴ�������û�еȴ�������Ҳû���������е�����
		comple_cv.wait(lock, [this] {
			return pending == 0 && running == 0;
			});
	}
};

//�̳߳ز���
class ThreadPool
{
private:
	//�̳߳�
	std::vector<std::thread> works;
	//�̳߳ش�����������
	Task& tasks;
	//�̳߳�����״̬
	bool open_pool;

public:
	//��������
	ThreadPool(Task& task, int num_works) :tasks(task), open_pool(true) {
		//ѭ�������߳�
		for (int i = 0; i < num_works; i++) {
			//��emplace_back���߳�ѹ��������
			works.emplace_back([this] {
				//�̲߳���ѭ��
				while (open_pool) {
					//����һ������
					std::function<void()> task;
					//����������л�ȡ������
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
	//�ر��̳߳�
	void closePool() {
		open_pool = false;
		tasks.closeTasks();
		//��������߳��Ƿ��������
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