# C++20 Coroutine Tutorial

## C++20 Coroutine

A coroutine is a generalized function that can suspend its execution and be resumed later. It's more flexible and complicated than a normal function.

From the syntax point of view, a C++ coroutine is a function that uses at least one of the following keywords: `co_await`, `co_yield`, `co_return`, `co_await`, but not `return`.

For example, the following function is a coroutine:

```cpp
void a_coroutine()
{
    co_return;
}
```

## Coroutine Traits

Coroutine traits are used to define the behaviors of coroutines. Before we can define coroutines, we need to define their traits.

In C++20, we define the traits of coroutines by specializing the [std::coroutine_traits](https://en.cppreference.com/w/cpp/coroutine/coroutine_traits) template.

The compiler uses the specialization of std::coroutine_traits based on the return type and parameter types of a coroutine.

For example:
```cpp
// for any coroutine that returns type R and takes no parameters: R func()
template<typename R> struct std::coroutine_traits<R> {...};

// for any coroutine that returns type void and takes two parameters:int and float: void func(int, float)
template<> struct std::coroutine_traits<void, int, float> {...};

// for any coroutine that returns type int and takes any number of parameters: int func(...)
template<typename... Args> struct std::coroutine_traits<int, Args...> {...};

// for any non-static member coroutine of type T that returns type R and takes any number of parameters: R(T::*)(...)
template<typename R,typename T,typename... Args> struct std::coroutine_traits<R, T&, Args...> {...};
```

The actual traits of coroutines are defined by the `promise_type` member type of the specialization of `std::coroutine_traits`. We'll talk about the `std::coroutine_traits::promise_type` later.

## Hello World

Let's write a simple coroutine that prints "Hello, world!" to the console.

```cpp
#include <coroutine>
#include <iostream>

// specify traits for coroutines of signature `void func()`
template <> struct std::coroutine_traits<void>
{
    // the magic promise_type, which we'll talk about later
    struct promise_type
    {
        void get_return_object() noexcept {}
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept {}
    };
};

// define a coroutine
void coroutine()
{
    std::cout << "coroutine: Hello, world!" << std::endl;
    co_return;
}

int main()
{
    coroutine();
    return EXIT_SUCCESS;
}

```

Run the code, and you'll see "`coroutine: Hello, world!`" printed to the console.

You can run this example code on [coliru](https://coliru.stacked-crooked.com/).

## Coroutine Awaitables

Awaitable types are types that implement the `await_ready()`, `await_suspend()`, and `await_resume()` member functions. We don't call these functions directly, but the compiler will generate code that calls them.

```cpp
// awaitable type interface
struct awaitable
{
    bool await_ready() noexcept {}

    [bool, void or std::coroutine_handle] await_suspend(std::coroutine_handle<[promise_type or void]> h) noexcept {} // we'll talk about std::coroutine_handle and promise_type later

    auto await_resume() noexcept {}
};

```

The C++20 standard library provides two trivial awaitable types: [std::suspend_always](https://en.cppreference.com/w/cpp/coroutine/suspend_always) and [std::suspend_never](https://en.cppreference.com/w/cpp/coroutine/suspend_never) for convenience.

Let's create two fake compiler generated functions to explain how the compiler calls the member functions of an awaitable object.

```cpp
awaitable __get_awaitable(T& t);
auto __co_await(awaitable& a);
```

Every time we use `auto r=co_await expr;` in a coroutine, the compiler will treat it as if it were written as follows:

```cpp
auto a= __get_awaitable(expr);
auto r= __co_await(a);
```

The `__get_awaitable()` function is used to convert an expression to an awaitable type. We'll talk about it later.

The `__co_await()` function is used to suspend the coroutine and return the result after the coroutine is resumed. Let's see how the compiler might implement it:

```cpp
auto __co_await(const awaitable& a)
{
    if(!a.await_ready())
    {
        <suspend the coroutine> // We use <...> to represent placeholder code that will be replaced by the compiler with real code

        using await_suspend_return_type= decltype(a.await_suspend({}));

        if constexpr(std::is_void_v<await_suspend_return_type>)
        {
            a.await_suspend(<current_coroutine_handle>); // we'll talk about std::coroutine_handle later

            <returns control to the caller or resumer>

        }else if(std::is_same_v<await_suspend_return_type, bool>)
        {
            if(a.await_suspend(<current_coroutine_handle>))
            {
                <returns control to the caller or resumer>
            }else
            {
                <resume the coroutine>
            }
        }else
        {
            auto handle_of_other_coroutine= a.await_suspend(<current_coroutine_handle>);

            handle_of_other_coroutine.resume(); // resume the other coroutine

            <returns control to the caller or resumer>
        }

        <resume point>
        // continue to execute the coroutine on the resumer's thread, after the coroutine is resumed
    }

    return a.await_resume();
}
```

## std::coroutine_handle

std::coroutine_handle is a template class that can be used to refer to a coroutine. We can use it to resume, or destroy a coroutine.

The standard library provides one primary template and two specializations of [std::coroutine_handle](https://en.cppreference.com/w/cpp/coroutine/coroutine_handle):

```cpp
template<typename promise_type> struct std::coroutine_handle;
template<> struct std::coroutine_handle<void>;
template<> struct std::coroutine_handle<std::noop_coroutine_promise>;
```

The `std::coroutine_handle<void>` is convertible from the other two.

We'll talk about `promise_type` later.

A coroutine can be resumed using the `std::coroutine_handle<>::resume()` or `std::coroutine_handle<>::operator()` function. However,
it is undefined behavior to resume a coroutine that has already completed (the `std::coroutine_handle<>::done()` function returns true) or has not yet suspended.

A coroutine may suspends even if the `std::coroutine_handle<>::done()` function returns true. We'll talk about it later.

A coroutine can be destroyed using the `std::coroutine_handle<>::destroy()` function if it has suspended. It's undefined behavior to destroy a coroutine that has not yet suspended.

A coroutine can be resumed or destroyed on any thread.

std::coroutine_handle is a lightweight type. It's cheap to copy, move, and store it by value. It's operations are not thread-safe.

We can get a `std::coroutine_handle` object of a coroutine by calling the `std::coroutine_handle<promise_type>::from_promise()` function or from `await_suspend()` function of an awaitable object.

## Practise I

Let's practise what we've learned so far.

```cpp
#include <coroutine>
#include <iostream>

// define traits for a coroutine that returns void and takes no arguments
template <> struct std::coroutine_traits<void>
{
    // the magic promise_type, which we'll talk about later
    struct promise_type
    {
        void get_return_object() noexcept {}
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() noexcept {}
        void return_void() {}
    };
};

// For the sake of simplicity, we'll use a global variable to store the handle
std::coroutine_handle<> h1, h2;

struct awaitable1 : public std::suspend_always
{
    void await_suspend(std::coroutine_handle<> h) noexcept { h1 = h; }
};

struct awaitable2 : public std::suspend_always
{
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept
    {
        h2 = h;
        return h1;
    }
};

void coroutine_1()
{
    std::cout << "coroutine_1: started, suspend and return to main" << std::endl;
    co_await awaitable1{};
    std::cout << "coroutine_1: resumed, exit and return to main" << std::endl;
};

void coroutine_2()
{
    std::cout << "coroutine_2: started, suspend and resume coroutine_1" << std::endl;
    co_await awaitable2{};
    std::cout << "coroutine_2: resumed, exit and return to main" << std::endl;
}

int main()
{
    std::cout << "main: start coroutine_1" << std::endl;
    coroutine_1();
    std::cout << "main: start coroutine_2" << std::endl;
    coroutine_2();
    std::cout << "main: resume coroutine_2" << std::endl;
    h2();
    std::cout << "main: exit" << std::endl;
    return EXIT_SUCCESS;
}
```

The output is:
```
main: start coroutine_1
coroutine_1: started, suspend and return to main
main: start coroutine_2
coroutine_2: started, suspend and resume coroutine_1
coroutine_1: resumed, exit and return to main
main: resume coroutine_2
coroutine_2: resumed, exit and return to main
main: exit
```

It's amazing that the three functions, `main()`, `coroutine_1()`, and `coroutine_2()` are cooperatively executed on a single thread.

## promise_type

The `std::coroutine_traits::promise_type` defines the traits of coroutines. It implements a special interface that the compiler uses to generate code for coroutines. We'll cover some of its member functions in this section, and leave the rest for later:

The minimal interface is:
```cpp
struct promise_type
{
    coroutine_return_type get_return_object() noexcept;
    awaitable initial_suspend() noexcept;
    awaitable final_suspend() noexcept;
    void unhandled_exception() noexcept;

    // one of the following:
    void return_void() noexcept; // to co_return;
    void return_value(T value) noexcept; // to co_return value;
};
```

Let's explain how the `promise_type` affects the coroutine execution.

For any coroutine:
```cpp
R coroutine(Args...args)
{
    // coroutine body
}
```

The compiler generates code similar to the following:

```cpp 
R coroutine(Args...args)
{
    auto promise=new std::coroutine_traits<R,Args...>::promise_type(args...);
    // or uses the default constructor: auto promise=new std::coroutine_traits<R,Args...>::promise_type();
    
    auto return_value=promise.get_return_object();
    // return_value is of type R, or can be converted to R implicitly.
    // return_value is returned to the caller when the coroutine reaches its first <returns control to the caller or resumer> point, or at the end of the coroutine if it doesn't have any <returns control to the caller or resumer> point.
    
    __co_await(promise.initial_suspend());

    try
    {
        // coroutine body
        // call atleast one time of return_void() or return_value(T value) according to the use of co_return;
    }
    catch(...)
    {
        promise.unhandled_exception();
    }
    __co_await(promise.final_suspend()); 
    // after this, the coroutine is considered as done, coroutine_handle.done() returns true.
    // it can't be resumed by `coroutine_handle.resume()`, 
    // but can be destroyed by `coroutine_handle.destroy()`

    delete promise;
}
```

## co_await keyword

The `co_await` is a unary operator that is used to suspend the current coroutine. It can be overloaded as a member function of a class, or as a free function. It's signature is:

```cpp
// as a member function
awaitable T::operator co_await() noexcept;

// as a free function
awaitable operator co_await(T) noexcept;
```

## __get_awaitable

It's a good time to explain our fake function `awaitable __get_awaitable(T& t);`.

std::coroutine_traits::promise_type can have a set of `class_type await_transform(T)` member functions optionally.

```cpp
// pseudo code
awaitable __get_awaitable(T& t)
{
    std::any a=t;    
    if constexpr (<promise_type has an applicable await_transform member function>)
    {
        a=promise.await_transform(a);
    }

    if constexpr (<decltype(a) overloads co_await as a member function>)
    {
        a=a.operator co_await();
    }else if(<this is a free 'awaitable operator co_await(decltype(a))' overload>)
    {
        a=operator co_await(a);
    }
    return a;
}
```

For example:
```cpp
strcut Test
{
    std::suspend_always operator co_await() noexcept{return {};}
};

struct promise_type
{
    ...

    Test await_transform(int) noexcept{return Test{};}
    
    ...
};

R coroutine(Args...args)
{
    co_await 1; // equivalent to co_await std::suspend_always{};
    co_await 2; // equivalent to co_await std::suspend_always{};
    ....
}
```

## co_yield keyword

The `co_yield` is a unary operator that is used to suspend the current coroutine and return a value.

For any expression `auto r=co_yield expr`, it will be transformed to `auto r=co_await promise.yield_value(expr);`.

## co_return keyword

The `co_return` statement is used to complete a coroutine.

For any expression `co_return expr;`, it will be transformed to `promise.return_value(expr);`.

For expression `co_return;`, it will be transformed to `promise.return_void();`.

## promise_type Constructors

By default, the default constructor of `promise_type` is used to construct the promise object. But,
if the promise type has a constructor that takes arguments matching the arguments of the coroutine, that
constructor will be used instead.

## Full promise_type interface

```cpp
struct promise_type
{
    coroutine_return_type get_return_object() noexcept;
    awaitable initial_suspend() noexcept;
    awaitable final_suspend() noexcept;
    void unhandled_exception() noexcept;

    // one of the following:
    void return_void() noexcept; // to co_return;
    void return_value(T) noexcept; // to co_return expr;

    // optional
    promise_type(Args...args) noexcept; // constructor that accepts arguments that match the coroutine's arguments
    void yield_value(T) noexcept; // to co_yield expr;
    class_type await_transform(T) noexcept; // to co_await expr;
};
```
 
## Conclusion

The coroutine is a powerful feature that can be used to implement many useful features, such as lazy evaluation, asynchronous programming, and so on. It's a good idea to learn it and use it.

## References

- [C++20 Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)
- [Coroutine Theory](https://lewissbaker.github.io/)

## Acknowledgements

I hope you have enjoyed this tutorial and found it useful. If you have any questions, comments, or feedback, please feel free to share them with me.
I would love to hear from you and improve this article. Thank you for reading and happy coding!