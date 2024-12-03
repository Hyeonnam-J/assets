#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <mutex>

#include <future>

class ThreadPool {
	
public:
    ThreadPool(size_t totalThreadSize);
    ~ThreadPool();
    
	// trailing return type.
	template<class F, class... Args>
	auto pushJob(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
private:
    size_t totalThreadSize;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;
    std::condition_variable cv;	// cv�� ������ �ʱ�ȭ ���� �ʰ� ����� ����� ��� �ڵ����� ����. 
    std::mutex m;
    bool is_stop;
    
    void workJob();
    
};	// class

#endif // THREADPOOL_H
