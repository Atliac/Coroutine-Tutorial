#include <iostream>
#include <coroutine>

using namespace std;
template<> struct std::coroutine_traits<void>
{
    struct promise_type
    {
        void get_return_object() noexcept {}
        auto initial_suspend() noexcept { return std::suspend_never{}; }
        auto final_suspend() noexcept { return std::suspend_never{}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };
};

void hello_coroutine()
{
    cout << "Hello coroutine" << endl;
    co_return;
}

int main()
{
    hello_coroutine();
	return 0;
}
