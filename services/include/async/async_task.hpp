#pragma once

#include <coroutine>
#include <exception>

template<typename T>
class TAsyncTask {
private:
    struct promise_type {
        T Value;
        std::exception_ptr Exception;
        std::coroutine_handle<> Handle;

        TAsyncTask get_return_object() { 
            return {}; 
        }

        std::suspend_always initial_suspend() noexcept { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        void return_value(T val) {
            Value = std::move(val);
        }

        void unhandled_exception() {
            Exception = std::current_exception();
        }
    };

};
