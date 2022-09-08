#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>

template <typename T>
class ring_buffer
{
    private:
        T *buffer;
        size_t buffer_size;
        size_t write_ptr, read_ptr;
        std::mutex mtx;
        std::condition_variable cv;
        bool is_empty_without_lock(void);
        bool is_full_without_lock(void);
        bool enqueue_signle_without_lock(const T *data);
        bool dequeue_signle_without_lock(T *data);
    public:
        ring_buffer(const size_t size);
        ~ring_buffer();
        bool is_empty(void);
        size_t enqueue(const T *data, size_t length);
        size_t dequeue(T *data, size_t max_length);
        bool wait(const std::chrono::steady_clock::time_point &timeout_at);
        void notify_one(void);
};

template <typename T>
ring_buffer<T>::ring_buffer(const size_t size)
{
    buffer = new T[size];
    buffer_size = size;
    write_ptr = 0;
    read_ptr = 0;
}

template <typename T>
ring_buffer<T>::~ring_buffer()
{
    delete buffer;
}

template <typename T>
bool ring_buffer<T>::is_empty_without_lock(void)
{
    return write_ptr == read_ptr;
}

template <typename T>
bool ring_buffer<T>::is_full_without_lock(void)
{
    const auto next_write_ptr = (write_ptr + 1) % buffer_size;
    return next_write_ptr == read_ptr;
}

template <typename T>
bool ring_buffer<T>::is_empty(void)
{
    std::lock_guard<std::mutex> lock(mtx);

    return is_empty_without_lock();
}

template <typename T>
bool ring_buffer<T>::enqueue_signle_without_lock(const T *data)
{
    if (is_full_without_lock()) {
        return false;
    }

    buffer[write_ptr] = *data;
    write_ptr = (write_ptr + 1) % buffer_size;

    return true;
}

template <typename T>
size_t ring_buffer<T>::enqueue(const T *data, size_t length)
{
    std::lock_guard<std::mutex> lock(mtx);

    size_t ptr = 0;
    while(ptr < length && enqueue_signle_without_lock(&data[ptr])) {ptr++;}

    return ptr;
}

template <typename T>
bool ring_buffer<T>::dequeue_signle_without_lock(T *data)
{
    if (is_empty_without_lock()) {
        return false;
    }

    *data = buffer[read_ptr];
    read_ptr = (read_ptr + 1) % buffer_size;

    return true;
}

template <typename T>
size_t ring_buffer<T>::dequeue(T *data, size_t max_length)
{
    std::lock_guard<std::mutex> lock(mtx);

    size_t ptr = 0;
    while(ptr < max_length && dequeue_signle_without_lock(&data[ptr])) {ptr++;}

    return ptr;
}

template <typename T>
bool ring_buffer<T>::wait(const std::chrono::steady_clock::time_point &timeout_at)
{
    std::unique_lock<std::mutex> lock(mtx);

    if (!is_empty_without_lock()) {
        return false;
    }

    return cv.wait_until(lock, timeout_at, [&]{return !is_empty_without_lock();});
}

template <typename T>
void ring_buffer<T>::notify_one(void)
{
    cv.notify_one();

    return;
}