#include <thread>
#include <atomic>
#include <vector>
#include <queue>
#include <mutex>
#ifndef __THREADS_WORKERS_H__
#define __THREADS_WORKERS_H__

class ThreadPool
{
private:
    std::mutex               m_queue_mutex; //controla la cola de threads
    std::mutex               m_results_mutex; // controla resultados
    std::vector<std::thread> m_threads;
    unsigned int             m_thread_count;
public:
    ThreadPool(unsigned int pool_size = std::thread::hardware_concurrency()) :
        //hardware_concurrency() -> n� de threads que tiene nuestro procesador disponible en ese momento (4 core -> 4, amd ripper 32 -> 32)
        m_queue_mutex(),
        m_thread_count(pool_size),
        m_threads()
    {

    };
    template <class F, class R, class A>
    std::vector<R> map(const F& f, std::vector<A> args)
    {
        std::queue <std::function<R()>> tasks; //funciones -> para cada thread
        std::vector<R> results; //rdo de cada thread
        for (auto& arg : args)
        {
            auto task = [&f, &arg]()
            {
                return f(arg);
            };
            tasks.push(std::move(task));
        }
        for (unsigned int i = 0; i < m_thread_count; ++i)
        {
            std::thread t
            (
                [this, &tasks, &results]() //acceso a esta clase mutex_task y Results
                {
                    while (true)                    
                    {//espera activa hasta que pueda acceder al recurso
                        if (m_queue_mutex.try_lock())   //bloquea cola hasta que accede a recurso
                        {
                            //false si otro thread ha accedido a recurso
                            if (!tasks.empty()) {
                                auto task = tasks.front();
                                tasks.pop();
                                m_queue_mutex.unlock();
                                auto res = task();
                                std::lock_guard<std::mutex> resutls_lock(m_results_mutex);
                                results.push_back(res);
                            }
                            else
                            {
                                m_queue_mutex.unlock(); //desbloquea recurso y para thread
                                break;
                            }
                        }
                    }
                }
            );
            m_threads.push_back(std::move(t));  //guardamos thread para saber cuales son los activos
        }
        for (auto& t : m_threads) t.join(); //esperamos a que terminen
        return results;
    }

};
#endif //THREADS_WORKERS_H