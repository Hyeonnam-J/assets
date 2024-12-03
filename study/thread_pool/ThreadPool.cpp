#include "ThreadPool.h"

#include <iostream>

ThreadPool::ThreadPool(size_t totalThreadSize)
	: totalThreadSize(totalThreadSize)
	, is_stop(false)
{
	// ���� �ʱ�ȭ (�뷮 �Ҵ� �� ���� �޼��� ����). 
	threads.reserve(totalThreadSize);
	for(size_t i = 0; i < totalThreadSize; i++){
		threads.emplace_back( [this] () { this -> workJob(); } );
	}
}	// constructor

ThreadPool::~ThreadPool() {
	is_stop = true;
	cv.notify_all();
	
	// thread ��ü�� ���簡 �Ұ����ϱ� ������ ������ ���. 
	for(auto& t : threads){
		t.join();
	}
}
	
void ThreadPool::workJob(){
	while(true) {
		// ���� ���ؽ��� �����ϰ�, cv üũ.
		// ���α׷��� ��� ������� 1���� cv ���� ������ ���ؽ� ������ ��ٸ��� ������� ������ �ȴ�. 
		// cv notify_one()����  ��� �� �ִ� ������� �� ���� ��� ���� �����尡 ����. 
		std::unique_lock<std::mutex> lock(m);
		
		// ���� �ƴϸ� ��� ���·� ����.
		// cv.notify_one()�� ���� ����� �ٽ� ������ ������ üũ. 
		cv.wait(lock, [this] () { return !this -> jobs.empty() || is_stop; });
		
		if(is_stop && this -> jobs.empty()) return;

		std::function<void()> job = std::move(jobs.front());	
		jobs.pop();
		
		lock.unlock();
		
		job();
	}
}	// workJob()

template<class F, class... Args>
auto ThreadPool::pushJob(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
	if(is_stop) throw std::runtime_error("�۾� ����");
	
	// using - Ÿ�Կ� ���� ��Ī. 
	using return_type = typename std::result_of<F(Args...)>::type;
	
	// bind - �Լ� ��ü ����.
	// packaged_task - �Լ��� �����Ͽ� �ᱣ���� �����ϴ� Ÿ��. 
	// packaged_task - ���� ���� �����ϴ� Ÿ��. 
	// packaged_task<int(int)> - int�� �����ϰ� ���ڷ� int�� �޴� �Լ�. 
	// make_shared - ����� ������ �������� job�� �����.
	// �Ŀ� jobs ��ü�� job�� ������ �� ������ ����. 
	// make_shared�� �Ἥ ����� ������ shared-ptr�� ���� �����ǵ��� ��. 
	// ����� ������ job�� ����Ű�� �޸𸮴� shared_ptr�� ���� �����Ǹ�, jobs ť�� �۾��� �߰��� ���Ŀ��� ���� ����. 
	// forward - �Ϻ��� ����. 
	auto job = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	
	// �۾��� ����� �ޱ� ���� future ��ü �غ�.
	// �Ŀ� job()�� ����� �� ����� ����. 
    std::future<return_type> job_result_future = job -> get_future();
    
	{
		std::lock_guard<std::mutex> lock(m);
		jobs.push([job]() { (*job)(); });
	}
	
	// cv notify_one()�� �����ϸ� �����ٷ����� ��ȣ�� ���� �����ٷ��� ���Ƿ� ��� ���� ������ �ϳ��� �����. 
	cv.notify_one();
	
	return job_result_future;
}	// pushJob()

void work_1(int i) {
	printf("%d work_1... \n", i);
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

int work_2(int i) {
	printf("%d work_2... \n", i);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	return i;
}

std::string work_3(int i) {
	printf("%d work_3... \n", i);
	std::this_thread::sleep_for(std::chrono::seconds(1));
	return std::to_string(i);
}

int main(){
	std::cout << "main start" << std::endl;
	
	ThreadPool pool(3);
	
	std::vector<std::future<void>> void_futures;
    std::vector<std::future<int>> int_futures;
    std::vector<std::future<std::string>> string_futures;
    
    // �۾��� Ǯ�� �߰�
    for (int i = 0; i < 10; i++) {
        if (i % 3 == 0) {
            void_futures.push_back(pool.pushJob([i]() { work_1(i); }));
        } else if (i % 3 == 1) {
            int_futures.push_back(pool.pushJob([i]() { return work_2(i); }));
        } else if (i % 3 == 2) {
            string_futures.push_back(pool.pushJob([i]() { return work_3(i); }));
        }
    }
	
    // ����� ���
    for (auto& future : void_futures) {
        future.get(); // �۾��� �Ϸ�� ������ ��ٸ�
    }
    for (auto& future : int_futures) {
        std::cout << "Returned value from work_2: " << future.get() << std::endl;
    }
    for (auto& future : string_futures) {
        std::cout << "Returned value from work_3: " << future.get() << std::endl;
    }
	
	return 0;
}
