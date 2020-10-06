#include <experimental/coroutine>
#include <future>
#include <iostream>
#include <assert.h>
#include <Windows.h>
#include <concurrent_queue.h>
#include <mutex>
#include <thread>
#include <chrono>
#include <typeinfo>
#include <list>
#include <vector>

using namespace std;
using namespace std::experimental;

#define MAX_TIME_QUANT 10
#define N 10

using coro_t = coroutine_handle<>;

class evt_awaiter_t {
    struct awaiter;

    vector<list<awaiter>> queues;
    list<awaiter> lst_;
    bool set_;

    struct awaiter {
        evt_awaiter_t& event_;
        coro_t coro_ = nullptr;
        awaiter(evt_awaiter_t& event) noexcept : event_(event) {}

        bool await_ready() const noexcept { return event_.is_set(); }

        void await_suspend(coro_t coro) noexcept {
            coro_ = coro;
            event_.push_awaiter(*this);
        }

        void await_resume() noexcept { event_.reset(); }
    };

public:
    evt_awaiter_t(bool set = false) : set_{ set } {}
    evt_awaiter_t(const evt_awaiter_t&) = delete;
    evt_awaiter_t& operator=(const evt_awaiter_t&) = delete;
    evt_awaiter_t(evt_awaiter_t&&) = delete;
    evt_awaiter_t& operator=(evt_awaiter_t&&) = delete;

    bool is_set() const noexcept { return set_; }
    void push_awaiter(awaiter a) { lst_.push_back(a); }

    awaiter operator co_await() noexcept { return awaiter{ *this }; }

    void set() noexcept {
        set_ = true;

        // making the new list with splice (O(1)) to get rid of the mistake
        // of continuosly adding tasks (resumed -> pushed_back to the list -> resumed -> ...)
        list<awaiter> toresume;
        toresume.splice(toresume.begin(), lst_);
        for (auto s : toresume)
            s.coro_.resume();
    }

    void reset() noexcept { set_ = false; }
};

struct resumable_no_own {
    struct promise_type {
        using coro_handle = coroutine_handle<promise_type>;
        auto get_return_object() { return coro_handle::from_promise(*this); }
        auto initial_suspend() { return suspend_never(); }

        // this one is critical: no suspend on final suspend
        // effectively means "destroy your frame"
        auto final_suspend() { return suspend_never(); }
        void return_void() {}
        void unhandled_exception() { terminate(); }
    };

    using coro_handle = coroutine_handle<promise_type>;
    resumable_no_own(coro_handle handle) {}
    resumable_no_own(resumable_no_own&) {}
    resumable_no_own(resumable_no_own&& rhs) {}
};

int g_value;
evt_awaiter_t g_evt;

void producer() {
    cout << "Producer started" << endl;
    this_thread::sleep_for(chrono::seconds(1));
    g_value = 42;
    cout << "Value ready" << endl;
    g_evt.set();
}

resumable_no_own runProcess(int runForMSecs)
{
    cout << "STARTED PROCESS. WANT TO RUN FOR " << runForMSecs << " mSecs" << endl;

    int lastIterNum = 1;
    for (int i = 1; i <= runForMSecs; i++)
    {
        cout << "Running mSec: " << i << endl;
        this_thread::sleep_for(chrono::milliseconds(1));
        if (i % MAX_TIME_QUANT == 0)
        {
            lastIterNum = i;
            cout << "Max time quant limit reached" << endl;
            cout << "Passing control to the calling function" << endl;
            co_await g_evt;
        }
    }

    cout << "ENDED PROCESS" << endl;
}

resumable_no_own consumer1() {
    cout << "Consumer1 started" << endl;
    co_await g_evt;
    cout << "Consumer1 resumed" << endl;
}

resumable_no_own consumer2() {
    cout << "Consumer2 started" << endl;
    co_await g_evt;
    cout << "Consumer2 resumed" << endl;
}

resumable_no_own consumer3() {
    cout << "Consumer3 started" << endl;
    co_await g_evt;
    cout << "Consumer3 resumed" << endl;
    co_await g_evt;
    cout << "Consumer3 resumed again" << endl;
}

void test1()
{
    consumer1();
    consumer2();
    consumer3();
    producer();
    consumer1();
    producer();
}
void test2()
{
    runProcess(10);
    runProcess(39);
    runProcess(5);
    runProcess(8);
    producer();
    producer();
    producer();
    producer();
}

int main() {
    //test1();
    test2();
}