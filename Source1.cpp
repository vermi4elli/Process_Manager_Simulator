#include <experimental/coroutine>
#include <iostream>
#include <assert.h>
#include <Windows.h>
#include <concurrent_queue.h>
#include <mutex>
#include <thread>
#include <chrono>
#include <typeinfo>
#include <list>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::experimental;

#define MAX_TIME_QUANT 10
#define N 10

using coro_t = coroutine_handle<>;

class ProcessManager {
    struct awaiter;

    unordered_map<int, int> processToQueueNumber;
    vector<list<awaiter>> queues_;
    list<awaiter> lst_;
    bool set_;
    int counter = 0;
    int lastRunnedProcess = 0;

    struct awaiter {
        ProcessManager& event_;
        coro_t coro_ = nullptr;
        awaiter(ProcessManager& event) noexcept : event_(event) {}

        bool await_ready() const noexcept { return event_.is_set(); }

        void await_suspend(coro_t coro) noexcept {
            coro_ = coro;
            event_.push_awaiter(*this);
        }

        void await_resume() noexcept { event_.reset(); }
    };

public:
    ProcessManager(bool set = false) : set_{ set } 
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
        cout << "The pushed queue address: " << (int)a.coro_.address() << endl;
        queues_.at(processToQueueNumber.at(lastRunnedProcess)).push_back(a); 
    }
    void change_process_queue(int process, int queue)
    {
        processToQueueNumber[process] = queue;
    }
    const int get_counter()
    {
        return ++counter;
    }
    void change_last_runned_process(int processNumber)
    {
        lastRunnedProcess = processNumber;
    }
    void dump();
    void remove_queue_process(int queue)
    {
        queues_.at(processToQueueNumber.at(lastRunnedProcess)).pop_front();
    }

    awaiter operator co_await() noexcept { return awaiter{ *this }; }

    void set() noexcept {
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

        auto s = (*it).front();
        s.coro_.resume();
        (*it).pop_front();
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
    
    processManager.set();
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
    };

    using coro_handle = coroutine_handle<promise_type>;
    resumable_no_own(coro_handle handle) {}
    resumable_no_own(resumable_no_own&) {}
    resumable_no_own(resumable_no_own&& rhs) {}
};

resumable_no_own runProcess(int runForMSecs)
{
    int processNumber = processManager.get_counter();
    processManager.change_process_queue(processNumber, 1);
    processManager.change_last_runned_process(processNumber);
    co_await processManager;

    cout << "> STARTED PROCESS #" << processNumber << ". WANT TO RUN FOR " << runForMSecs << " mSecs" << endl;

    int lastQueueNum = 1;
    for (int i = 1; i <= runForMSecs; i++)
    {
        cout << "Running mSec: " << i << endl;
        this_thread::sleep_for(chrono::milliseconds(1));
        if (i % MAX_TIME_QUANT == 0 && i != runForMSecs)
        {
            lastQueueNum++;
            cout << "\n> MAX TIME QUANT LIMIT REACHED" << endl;
            cout << "> MOVING THE PROCESS #" << processNumber << " TO THE QUEUE #" << lastQueueNum << "...\n" << endl;
            processManager.change_process_queue(processNumber, lastQueueNum);
            processManager.change_last_runned_process(processNumber);
            co_await processManager;
            cout << "> RESUMED PROCESS #" << processNumber << ". YET TO RUN FOR " << runForMSecs - i  << " mSecs" << endl;
        }
    }
    cout << "> ENDED PROCESS #" << processNumber << endl << endl;
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
        if (i < tasks_weights.size())
        {
            runProcess(tasks_weights[i++]);
        }

        processManager.dump();
    }
}

int main() {
    //test1();
    test2();
}