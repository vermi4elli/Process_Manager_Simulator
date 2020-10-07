#include <experimental/coroutine>
#include <concurrent_queue.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <list>
#include <Windows.h>
#include <assert.h>
#include <typeinfo>
#include <cstdlib>

using namespace std;
using namespace std::experimental;

#define MAX_TIME_QUANT 10
#define N 10
#define TASKS_AMOUNT 30

using coro_t = coroutine_handle<>;

class ProcessManager {
    struct awaiter;

    unordered_map<int64_t, int> processToQueueNumber_;
    vector<list<awaiter>> queues_;
    bool set_;
    int counter = 0;
    int lastRunnedProcess = 0;

    struct awaiter {
        ProcessManager& manager_;
        coro_t coro_ = nullptr;
        awaiter(ProcessManager& event) noexcept : manager_(event) {}

        bool await_ready() const noexcept { return manager_.is_set(); }

        void await_suspend(coro_t coro) noexcept {
            coro_ = coro;
            manager_.push_awaiter(*this);
        }

        void await_resume() noexcept { manager_.reset(); }
    };

public:
    ProcessManager(bool update = false) : set_{ update } 
    {
        for (int i = 0; i < N; i++)
        {
            list<awaiter> list_temp;
            queues_.push_back(list_temp);
        }
    }
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    bool is_set() const noexcept { return set_; }
    void push_awaiter(awaiter a) {
        int64_t address = (int64_t)a.coro_.address();
        
        ++processToQueueNumber_[address];
        queues_.at(processToQueueNumber_[address]).push_back(a);

        if (processToQueueNumber_[address] > 1)
        {
            cout << "> MAX TIME QUANT LIMIT REACHED!" << endl;
        }
        cout << "> " << (processToQueueNumber_[address] == 1 ? "ADDING" : "MOVING") << " PROCESS #" << address << " TO THE QUEUE #" << processToQueueNumber_[address] << "...\n" << endl;
    }
    void dump();

    awaiter operator co_await() noexcept { return awaiter{ *this }; }

    void update() noexcept {
        set_ = true;
        auto it = queues_.begin();

        while ((*it).empty())
        {
            it++;
            if (it == queues_.end())
            {
                cout << "THERE ARE NO PROCESSES!!!" << endl;
            }
        }

        int queue_number = it - queues_.begin();
        if (it != queues_.end())
        {
            auto s = (*it).front();

            cout << "> " << (queue_number == 1 ? "STARTED" : "RESUMED") << " PROCESS #" << (int64_t)s.coro_.address() << endl;
            
            s.coro_.resume();
            (*it).pop_front();
        }
    }

    void reset() noexcept { set_ = false; }
};


ProcessManager processManager;

void ProcessManager::dump() {
    cout << "\n===== MANAGER DUMP =====" << endl;

    for (int i = 1; i < queues_.size(); i++)
    {
        cout << "QUEUE #" << i << ": " << queues_.at(i).size() << " processes" << endl;
    }

    cout << "===== MANAGER DUMP END =====\n" << endl;
    
    processManager.update();
}

struct resumable_no_own {
    struct promise_type {
        using coro_handle = coroutine_handle<promise_type>;
        auto get_return_object() { return coro_handle::from_promise(*this); }
        auto initial_suspend() { return suspend_never(); }

        // this one is critical: no suspend on final suspend
        // effectively means "destroy your frame"
        auto final_suspend() noexcept { return suspend_never(); }
        void return_void() {}
        void unhandled_exception() { terminate(); }
        ~promise_type()
        {
            cout << "> ENDED PROCESS\n" << endl;
        }
    };

    using coro_handle = coroutine_handle<promise_type>;
    resumable_no_own(coro_handle handle) {}
    resumable_no_own(resumable_no_own&) {}
    resumable_no_own(resumable_no_own&& rhs) {}
};

resumable_no_own runProcess(int runForMSecs)
{
    co_await processManager;
    cout << "> WANT TO RUN FOR " << runForMSecs << " mSecs" << endl;

    for (int i = 1; i <= runForMSecs; i++)
    {
        cout << "Running mSec: " << i << endl;
        this_thread::sleep_for(chrono::milliseconds(1));
        if (i % MAX_TIME_QUANT == 0 && i != runForMSecs)
        {
            co_await processManager;
            cout << "> LEFT TO RUN FOR " << runForMSecs - i  << " mSecs" << endl;
        }
    }
}

vector<int> GenerateTasksWeights(int highest, int fault)
{
    vector<int> temp;
    cout << "THE TASKS WEIGHTS: ";
    for (int i = 0; i < TASKS_AMOUNT; i++)
    {
        temp.push_back(rand() % highest + fault);
        cout << temp.back() << " ";
    }
    cout << "\n" << endl;
    return temp;
}

void test1()
{
    runProcess(10);
    runProcess(39);
    runProcess(25);
    runProcess(18);
    processManager.dump();
    processManager.dump();
    processManager.dump();
    processManager.dump();
}
void test2()
{
    vector<int> tasks_weights = {10, 39, 25, 18, 17};
    
    int i = 0;
    while (true)
    {
        ++i;
        if (i < tasks_weights.size())
        {
            runProcess(tasks_weights[i]);
        }

        processManager.dump();
    }
}
void test3()
{
    vector<int> tasks_weights = GenerateTasksWeights(16, 9);

    int i = 0;
    while (true)
    {
        ++i;
        if (i < tasks_weights.size() && rand() % 2 == 1)
        {
            runProcess(tasks_weights[i]);
        }

        processManager.dump();
    }
}

int main() {
    //test1();
    //test2();
    test3();
}