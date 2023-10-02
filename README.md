# C++20 Coroutine Tutorial

## C++20 Coroutine

A coroutine is a generalized function that can suspend its execution and be resumed later.

In C++20, a coroutine is a function that satisfies the following requirements:

1. Uses at least one of the following keywords in its body:
   - `co_await`
   - `co_yield`
   - `co_return`
2. Has a concrete return type.

A coroutine can't use `return` in its body, and can't use `auto` as its return type.

For example, the following function is a coroutine:

```cpp
void hello_coroutine() 
{
    std::cout << "Hello, coroutine!" << std::endl;
    co_return;
}
```

## Coroutine Traits

While the previous `hello_coroutine` function is a valid C++ coroutine, it can not be compiled all by itself. We have to have its traits defined first.

We can specilize the [`std::coroutine_traits`](https://en.cppreference.com/w/cpp/coroutine/coroutine_traits) template to define the traits of coroutines.

For exmaple:

```cpp
// The traits for any coroutine function that returns any type, and takes any number of arguments.
// If the coroutine is a non-static member function, then the first type in Args... is the type of
// the implicit object parameter, and the rest are parameter types of the function
template <typename R, typename... Args> struct std::coroutine_traits<R, Args...> {...};
// The traits for any member coroutine function of class T that returns any type, and takes any number of arguments.
template <typename R, typename T, typename... Args> struct std::coroutine_traits<R, T&, Args...> {...};
// The traits for any non-member or static-member coroutine function that returns void and takes no arguments.
template <> struct std::coroutine_traits<void> {...};
```

The `std::coroutine_traits` template has a member type `promise_type`, which implements a specific interface. The `promise_type` is the place where the actual traits definition happens.

```cpp
template <...> struct std::coroutine_traits<...>
{
    struct promise_type
    {
        ...
    }
};
```

Traits are public member functions of the `std::coroutine_traits<...>::promise_type` class. For example, the function `void std::coroutine_traits<...>::promise_type::return_void()` is a trait that is used by the compiler to replace the `co_return;` statement in the coroutine function.

`promise_type` class is defined by the programmer, but is instantiated and used by the compiler automatically.

## The Caller and Resumers of a Coroutine

The code point that calls a coroutine is called the **caller** of that coroutine. The coroutine returns a value of type of return type of the coroutine when the coroutine hits its first suspension point or falling off the end of the coroutine body without hitting any suspension point.

The code point that resumes a coroutine is called the **resumer** of that coroutine. The coroutine returns to the resumer when it hits the next suspension point or falling off the end of the coroutine body without hitting another suspension point.

## The Threading Model of Coroutines

Unlike normal functions, which are bound to the thread of their callers, coroutines starts their execution on the thread of their callers, and resume their execution on the thread of their resumers.

The cool thing is that, even if the threading context may change, the codes in the coroutine function run as if they are in a single thread.

## Awaitables

An **awaitable** is an object of a class type that implements a special interface. Its public member functions are called by complier to handle the suspension and resumption of a coroutine.

Awaitable class is defined and instantiated by the programmer, but is used by the compiler automatically.

## Coroutine Handle

A **coroutine handle** is an object of type [std::coroutine_handle](https://en.cppreference.com/w/cpp/coroutine/coroutine_handle) that represents a coroutine. It is used to resume a coroutine, and to query the status of a coroutine.

## Compiler Transformation

The compiler transform any coroutine,

```cpp
coroutine_return_type coroutine_name(coroutine_parameter_list)
{
    coroutine_body
}
```

, with the following steps roughly:

```cpp
// pseudo helper function: __get_awatiable
awaitable_type __get_awatiable(Exp exp);
// pseudo helper function: __await_routine
auto __await_routine(awaitable_type awaitable);

coroutine_return_type coroutine_name(coroutine_parameter_list)
{
    // instantiate the promise_type with the constructor of the promise_type
    // that takes the coroutine_parameter_list or the default constructor if
    // the promise_type does not have a constructor that takes the coroutine_parameter_list
    promise_type promise_object= promise_type{coroutine_parameter_list} or promise_type{};

    coroutine_return_type coroutine_return_value= promise_object.get_return_object();
    // the coroutine_return_value is returned to the caller when the coroutine hits its first suspension point
    // or falling off the end of the coroutine body without hitting any suspension point

    __await_routine(promise_object.initial_suspend());
    // the return value of previous call is discarded
    
    try
    {
        coroutine_body

        // in the coroutine_body, the compiler applies the following transformation:
        // - co_await exp; -> __await_routine(__get_awatiable(exp));
        // - auto r= co_await exp; -> auto r= __await_routine(__get_awatiable(exp));
        // - co_yield exp; -> co_await promise.yield_value(exp); -> __await_routine(__get_awatiable(promise.yield_value(exp)));
        // - auto r= co_yield exp; -> auto r= co_await promise.yield_value(exp); -> auto r= __await_routine(__get_awatiable(promise.yield_value(exp)));
        // - co_return exp; -> promise.return_value(exp);
        // - co_return; -> promise.return_void();
    }catch(...)
    {
        promise_object.unhandled_exception();
    }

    __await_routine(promise_object.final_suspend());
    // coroutine suspends here can not be resumed by the resume function of the coroutine_handle,
    // but can be resumed by the destroy function of the coroutine_handle

    // if the coroutine falls off the previous steps or be destoryed after a suspension point,
    // it destructs the promise_object as well as any local variables of the coroutine,
    // transfters the control to the caller or resumer
}

awaitable_type __get_awatiable(Exp exp)
{
    std::any awaitable= exp;

    if constexpr(/*if promise_type::await_transform(Exp) exists*/)
    {
        awaitable= promise_object.await_transform(exp);
    }

    if constexpr(/*if the class type of awaitable overloads co_await operator as a member function*/)
    {
        return awaitable.operator co_await();
    }else if constexpr(/* this is a free co_await operator function that takes the type of awaitable as its parameter*/)
    {
        return operator co_await(awaitable);
    }
    
    return awaitable;
}

auto __await_routine(awaitable_type awaitable)
{
    if(awaitable.await_ready())
    {
        return awaitable.await_resume();
    }

    <suspend the coroutine>
    // the coroutine is in the suspended state now, but the control is still kept

    if constexpr(/* the return type of awaitable.await_suspend(std::coroutine_handle<void>) is void*/)
    {
        awaitable.await_suspend(handle_of_the_coroutine);

        <returns control to the caller or resumer>
    }else if(/* the return type of await_suspend function is bool*/)
    {
        if(awaitable.await_suspend(handle_of_the_coroutine))
        {
            <returns control to the caller or resumer>
        }else
        {
            <resume the coroutine>
        }
    }else
    {
        // the return type of await_suspend function is std::coroutine_handle<void>
        auto handle_of_other_coroutine= awaitable.await_suspend(handle_of_the_coroutine);
        handle_of_other_coroutine.resume(); // resume the other coroutine
        <returns control to the caller or resumer>
    }

    <resume point>
    // continue to execute the coroutine on the resumer's thread

    return awaitable.await_resume();
}
```

## Interface of promise_type

```cpp
struct promise_type
{
    coroutine_return_type get_return_object();
    awaitable_type initial_suspend();
    awaitable_type final_suspend();
    void unhandled_exception();

    // one of the following:
    void return_void(); // to co_return;
    void return_value(T); // to co_return expr;

    // optional
    promise_type(Args...args); // constructor that accepts arguments that match the coroutine's arguments
    void yield_value(T); // to co_yield expr;
    class_type await_transform(T); // to co_await expr;
};
```

## Awaitable Interface

```cpp
struct awaitable_type
{
    bool await_ready();
    // R can be void, bool, or std::coroutine_handle<>
    R await_suspend(std::coroutine_handle<>);
    auto await_resume();
};
```

## Example

See [hello_coroutine](hello_coroutine/main.cpp).

## Reference

- [C++20 Coroutine](https://en.cppreference.com/w/cpp/language/coroutines)
- [Coroutine Theory](https://lewissbaker.github.io/)