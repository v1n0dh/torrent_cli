#ifndef SHARED_QUEUE
#define SHARED_QUEUE

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
class Shared_Queue {
public:
	Shared_Queue<T>& operator<<(const T& item) {
		std::unique_lock<std::mutex> _lock(_mutex);

		_queue.push(item);

		_cond.notify_one();

		return *this;
	}

	Shared_Queue<T>& operator>>(T& item) {
		std::unique_lock<std::mutex> _lock(_mutex);

		// wait until queue is not empty
		_cond.wait(_lock, [this]() { return !this->_queue.empty(); });

		item = std::move(_queue.front());

		_queue.pop();

		return *this;
	}

	size_t size() {
		std::unique_lock<std::mutex> _lock(_mutex);
		return _queue.size();
	}

	bool empty() {
		std::unique_lock<std::mutex> _lock(_mutex);
		return _queue.empty();
	}

	void clean() {
		std::unique_lock<std::mutex> _lock(_mutex);
		while (!this->_queue.empty()) {
			if constexpr (std::is_pointer<T>::value)
				delete this->_queue.front();
			this->_queue.pop();
		}
	}


private:
	std::queue<T> _queue;
	std::mutex _mutex;
	std::condition_variable _cond;
};

template <typename T>
class Shared_Vector {
public:
	void push_back(const T& item) {
		std::unique_lock<std::shared_mutex> _lock(_mutex);
		_vector.push_back(item);
	}

	template <typename... Args>
	void emplace_back(Args... args) {
		std::unique_lock<std::shared_mutex> _lock(_mutex);
		_vector.emplace_back(std::forward<Args>(args)...);
	}

	T& operator[](size_t index) {
		std::unique_lock<std::shared_mutex> _lock(_mutex);
		if (index >= _vector.size())
			throw std::out_of_range("Index out of range");

		return _vector[index];
	}

	const T& operator[](size_t index) const {
		std::unique_lock<std::shared_mutex> _lock(_mutex);
		if (index >= _vector.size())
			throw std::out_of_range("Index out of range");

		return _vector[index];
	}

	size_t size() {
		std::shared_lock<std::shared_mutex> _lock(_mutex);
		return _vector.size();
	}

	void resize(size_t new_cap) {
		std::unique_lock<std::shared_mutex> _lock(_mutex);
		_vector.resize(new_cap);
	}

	// Iterators for range based loops
	typename std::vector<T>::const_iterator begin() const {
		return _vector.begin();
	}

	typename std::vector<T>::const_iterator end() const {
		return _vector.end();
	}

	typename std::vector<T>::iterator begin() {
		return _vector.begin();
	}

	typename std::vector<T>::iterator end() {
		return _vector.end();
	}
private:
	std::vector<T> _vector;
	std::shared_mutex _mutex;
};

#endif
