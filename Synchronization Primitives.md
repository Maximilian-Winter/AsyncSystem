# Synchronization Primitives

1. `std::mutex`:
   - This is a basic synchronization primitive used to protect shared data from being simultaneously accessed by multiple threads.
   - Usage:
     ```cpp
     std::mutex m_mutex;
     
     // To lock:
     m_mutex.lock();
     // Critical section (access shared resource)
     m_mutex.unlock();
     ```

2. `std::unique_lock<std::mutex>`:
   - This is a movable, flexible mutex ownership wrapper.
   - It allows you to lock and unlock the mutex multiple times if needed.
   - It's often used with condition variables.
   - Usage:
     ```cpp
     std::mutex m_mutex;
     
     {
         std::unique_lock<std::mutex> lock(m_mutex);
         // Critical section (access shared resource)
         // lock is automatically released when it goes out of scope
     }
     ```

3. `std::lock_guard<std::mutex>`:
   - This is a simpler version of `unique_lock`. It's a scoped lock that automatically locks the mutex when constructed and unlocks it when destroyed.
   - Usage:
     ```cpp
     std::mutex m_mutex;
     
     {
         std::lock_guard<std::mutex> lock(m_mutex);
         // Critical section (access shared resource)
         // lock is automatically released when it goes out of scope
     }
     ```

4. `std::atomic<bool>`:
   - This is an atomic type that ensures that the variable can be accessed safely from multiple threads without explicit synchronization.
   - Operations on atomic types are indivisible.
   - Usage:
     ```cpp
     std::atomic<bool> m_stopped(false);
     
     // In one thread:
     m_stopped.store(true);
     
     // In another thread:
     while (!m_stopped.load()) {
         // Do work
     }
     ```

5. `std::condition_variable`:
   - This is used for blocking a thread until another thread modifies a shared variable (the condition) and notifies the condition_variable.
   - It's always used with a `std::unique_lock<std::mutex>`.
   - Usage:
     ```cpp
     std::mutex m_mutex;
     std::condition_variable m_condition;
     bool ready = false;
     
     // Consumer thread:
     {
         std::unique_lock<std::mutex> lock(m_mutex);
         m_condition.wait(lock, [&]{ return ready; });
         // Process data
     }
     
     // Producer thread:
     {
         std::unique_lock<std::mutex> lock(m_mutex);
         ready = true;
     }
     m_condition.notify_one();
     ```
