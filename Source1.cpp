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
#define QUEUES_AMOUNT 10
#define TASKS_AMOUNT 12

using coro_t = coroutine_handle<>;
int g_counter;

struct resumable_no_own {
    struct promise_type {
        int number;
        bool active = true;
        using coro_handle = coroutine_handle<promise_type>;
        auto get_return_object() { return coro_handle::from_promise(*this); }
        auto initial_suspend() { return suspend_never(); }

        // this one is critical: no suspend on final suspend
        // effectively means "destroy your frame"
        auto final_suspend() noexcept { return suspend_never(); }
        void return_void() {}
        void unhandled_exception() { terminate(); }
        promise_type()
        {
            number = ++g_counter;
        }
        ~promise_type()
        {
            active = false;
            cout << "> ENDED PROCESS #" << number << "\n" << endl;
        }
    };

    using coro_handle = coroutine_handle<promise_type>;
    resumable_no_own(coro_handle handle) {}
    resumable_no_own(resumable_no_own&) {}
    resumable_no_own(resumable_no_own&& rhs) {}
};

class ProcessManager {
    struct awaiter;

    unordered_map<int, int> processToQueueNumber_;
    vector<list<awaiter>> queues_;
    bool empty = true;

    struct awaiter {
        ProcessManager& manager_;
        resumable_no_own::coro_handle coro_ = nullptr;
        awaiter(ProcessManager& event) noexcept : manager_(event) {}

        bool await_ready() const noexcept { return false; }

        void await_suspend(resumable_no_own::coro_handle coro) noexcept {
            coro_ = coro;
            manager_.push_awaiter(*this);
        }

        void await_resume() noexcept {}
    };

public:
    ProcessManager(bool update = false)
    {
        for (int i = 0; i < QUEUES_AMOUNT; i++)
        {
            list<awaiter> list_temp;
            queues_.push_back(list_temp);
        }
    }
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    void push_awaiter(awaiter a) {        
        if (!a.coro_.done() && processToQueueNumber_[a.coro_.promise().number] != QUEUES_AMOUNT - 1)
        {
            ++processToQueueNumber_[a.coro_.promise().number];
            queues_.at(processToQueueNumber_[a.coro_.promise().number]).push_back(a);


            empty = false;
            if (processToQueueNumber_[a.coro_.promise().number] > 1)
            {
                cout << "> MAX TIME QUANT LIMIT REACHED!" << endl;
            }
            cout << "> " << (processToQueueNumber_[a.coro_.promise().number] == 1 ? "ADDING" : "MOVING") << " PROCESS #" << a.coro_.promise().number << " TO THE QUEUE #" << processToQueueNumber_[a.coro_.promise().number] << "...\n" << endl;
        }

    }
    void dump(ProcessManager& a);
    void remove_process(int number)
    {
        processToQueueNumber_.erase(number);
    }

    awaiter operator co_await() noexcept { return awaiter{ *this }; }

    void update() noexcept {

        for (int i = 0; i < queues_.size(); i++)
        {
            if (queues_[i].empty())
            {
                if (i == queues_.size() - 1)
                {
                    cout << "THERE ARE NO PROCESSES!!!" << endl;
                    empty = true;
                }
                continue;
            }

            auto s = queues_[i].front();

            cout << "> " << (i == 1 ? "STARTED" : "RESUMED") << " PROCESS #" << s.coro_.promise().number << " FROM QUEUE #" << i << endl;


            if (i != queues_.size() - 1)
            {
                if (!s.coro_.done()) s.coro_.resume();
            }
            else
            {
                while (!s.coro_.done() && s.coro_.promise().active) s.coro_.resume();
            }
            
            queues_[i].pop_front();

            break;
        }
    }
};

void ProcessManager::dump(ProcessManager& a) {
    if (!empty)
    {
        cout << "\n===== MANAGER DUMP =====" << endl;

        for (int i = 1; i < queues_.size(); i++)
        {
            cout << "QUEUE #" << i << ": " << queues_.at(i).size() << " processes" << endl;
            for (const auto& task : queues_[i])
            {
                cout << "   |__ PROCESS #" << task.coro_.promise().number << endl;
            }
        }

        cout << "===== MANAGER DUMP END =====\n" << endl;

        a.update();
    }
}

ProcessManager processManager;

resumable_no_own runProcess(int runForMSecs)
{
    co_await processManager;
    cout << "> WANT TO RUN FOR " << runForMSecs << " mSecs" << endl;

    for (int i = 0; i <= runForMSecs; i++)
    {
        cout << "Running mSec: " << i << endl;
        this_thread::sleep_for(chrono::milliseconds(1));
        if (i % MAX_TIME_QUANT == 0 && i != 0 && i != runForMSecs)
        {
            co_await processManager;
            if (i != runForMSecs) cout << "> LEFT TO RUN FOR " << runForMSecs - i  << " mSecs FROM INITIAL " << runForMSecs << endl;
        }
    }
}

vector<int> GenerateTasksWeights(int highest, int fault)
{
    srand(time(0));

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
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
    processManager.dump(processManager);
}
void test2()
{
    vector<int> tasks_weights = {10, 110, 125};
    
    int i = 0;
    while (true)
    {
        if (i < tasks_weights.size())
        {
            runProcess(tasks_weights[i]);
            ++i;
        }

        processManager.dump(processManager);
    }
}
void test3()
{
    vector<int> tasks_weights = GenerateTasksWeights(16, 9);

    int counter = 0;
    int i = 0;
    while (true)
    {
        ++counter;
        
        if (i < tasks_weights.size() && counter % (rand() % 3 + 1) == 0)
        {
            runProcess(tasks_weights[i]);
            ++i;
        }

        processManager.dump(processManager);
    }
}

int main() {
    //test1();
    //test2();
    test3();
}