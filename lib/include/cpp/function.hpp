#ifndef CPP_FUNCTION_LAMBDA
#define CPP_FUNCTION_LAMBDA
#include <utility>

template <typename> class function;

template <typename R, typename... Args> class function<R(Args...)>
{
  private:
    void *ptr = nullptr;
    R (*call_lambda)(void *f, Args...) = nullptr;

  public:
    function() = default;

    template <typename F> inline function(F &&f)
    {

        /* decay will remove refrences */
        using T = std::decay_t<F>;
        ptr = new T(f);
        call_lambda = [](void *func, Args... a) -> R
        { return (*((T *)(func)))(a...); };
    }

    inline R operator()(Args... args) const
    {
        return call_lambda(ptr, args...);
    }
};

#endif
