#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include "lib/log.h"
#include <cpp/cpp_c_include_start.hpp>

#include <string.h>

#include <cpp/cpp_c_include_end.hpp>

#include <lib/ui/ui.hpp>
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
        {
            logd("ptr:%p", func);
            return (*((T *)(func)))(a...);
        };
        logd("call_lambda in constructor:%p", call_lambda);
    }

    inline R operator()(Args... args) const
    {
        return call_lambda(ptr, args...);
    }
};

class ButtonElement : public Element
{
  private:
    struct image img;
    View *child;

  public:
    function<void(ButtonElement *)> onclick;
    u32 color = 0xFF;
    ButtonElement(View *child);
    void render() override;
    void handle_element_click() override;
    void set_onclick(function<void(ButtonElement *)>);
};

#endif
