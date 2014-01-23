#pragma once
#include <mutex>
#include <queue>

template <class T>
class SafeQueue
{
public:
	void push(const T& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(item);
	}
	
	void pop() {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.pop();
	}
	
	T& front() {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.front();
	}

	bool empty() {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.empty();
	}

	void clear() {
		std::queue<T> empty;
		std::swap(queue_, empty);
	}

private:
	std::queue<T> queue_;
	std::mutex mutex_;
};

