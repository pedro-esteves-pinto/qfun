#pragma once

#include <type_traits>
#include <utility>

namespace qfun {

namespace detail {
template <class T>
struct handler_traits;

template <class T, class Re>
struct handler_traits<Re T::*> {
   using HandlerType = T;
   using HandlerReturnType = Re;
};
} // namespace detail
  //
// Provides a fast and small equivalent to std::function. Unlike
// std::function instances of QuickFunction do not copy the callable they
// are bound to and consequently never allocate.
template <typename Signature>
class QuickFunction;

template <typename R, typename... Arg>
class QuickFunction<R(Arg...)> {
 public:
   template <class C>
   explicit QuickFunction(C & /*c*/);
   QuickFunction();
   ~QuickFunction() = default;
   QuickFunction(QuickFunction const &d) = default;
   QuickFunction(QuickFunction &&d) noexcept = delete;
   QuickFunction &operator=(QuickFunction const &d);
   QuickFunction &operator=(QuickFunction const &&d) = delete;

   template <class C>
   static QuickFunction make(C &handler_obj);

   template <class C>
   void bind(C &handler_obj);

   template <auto handler_member>
   static QuickFunction make(
       typename detail::handler_traits<decltype(handler_member)>::HandlerType
           &handler_obj);

   template <auto handler_member>
   void bind(
       typename detail::handler_traits<decltype(handler_member)>::HandlerType
           &handler_obj);

   template <auto static_handler>
   static QuickFunction make();

   template <auto static_handler>
   void bind();

   R operator()(Arg... arg);

 private:
   template <auto handler_member, typename Re, typename... Args>
   static R invoke_member_handler(void *handler_obj, Args... arg);

   template <auto handler, typename Re, typename... Args>
   static R invoke_static_handler(void * /*unused*/, Args... arg);

   static R null_target(Arg... /*unused*/);

   void *target_obj_{};
   R (*invoke_)(void *, Arg...);
};

template <typename R, typename... Arg>
template <class C>
QuickFunction<R(Arg...)>::QuickFunction(C &c) {
   bind(c);
}

template <typename R, typename... Arg>
QuickFunction<R(Arg...)>::QuickFunction() : invoke_() {
   bind<&QuickFunction::null_target>();
}

template <typename R, typename... Arg>
QuickFunction<R(Arg...)> &QuickFunction<R(Arg...)>::operator=(
    QuickFunction const &d) {
   if (&d != this) {
      target_obj_ = d.target_obj_;
      invoke_ = d.invoke_;
   }
   return *this;
}

template <typename R, typename... Arg>
template <auto handler_member, typename Re, typename... Args>
R QuickFunction<R(Arg...)>::invoke_member_handler(
    void *handler_obj, Args... arg) {
   using HandlerMethod = decltype(handler_member);
   static_assert(std::is_member_function_pointer<HandlerMethod>::value);
   using HandlerType =
       typename detail::handler_traits<HandlerMethod>::HandlerType;
   return (((HandlerType *)handler_obj)->*handler_member)(
       std::forward<Arg>(arg)...);
}

template <typename R, typename... Arg>
template <auto handler, typename Re, typename... Args>
R QuickFunction<R(Arg...)>::invoke_static_handler(
    void * /*unused*/, Args... arg) {
   return handler(std::forward<Arg>(arg)...);
}

template <typename R, typename... Arg>
R QuickFunction<R(Arg...)>::null_target(Arg... /*unused*/) {
   if constexpr (std::is_same<void, R>::value) {
      return;
   } else {
      return {};
   }
}

template <typename R, typename... Arg>
template <class C>
QuickFunction<R(Arg...)> QuickFunction<R(Arg...)>::make(C &handler_obj) {
   QuickFunction result;
   result.bind(handler_obj);
   return result;
}

template <typename R, typename... Arg>
template <class C>
void QuickFunction<R(Arg...)>::bind(C &handler_obj) {
   target_obj_ = &handler_obj;
   bind<&C::operator()>(handler_obj);
}

template <typename R, typename... Arg>
template <auto handler_member>
QuickFunction<R(Arg...)> QuickFunction<R(Arg...)>::make(
    typename detail::handler_traits<decltype(handler_member)>::HandlerType
        &handler_obj) {
   QuickFunction result;
   result.bind<handler_member>(handler_obj);
   return result;
}

template <typename R, typename... Arg>
template <auto handler_member>
void QuickFunction<R(Arg...)>::bind(
    typename detail::handler_traits<decltype(handler_member)>::HandlerType
        &handler_obj) {
   target_obj_ = &handler_obj;
   invoke_ = invoke_member_handler<handler_member, R, Arg...>;
}

template <typename R, typename... Arg>
template <auto static_handler>
QuickFunction<R(Arg...)> QuickFunction<R(Arg...)>::make() {
   QuickFunction result;
   result.bind<static_handler>();
   return result;
}

template <typename R, typename... Arg>
template <auto static_handler>
void QuickFunction<R(Arg...)>::bind() {
   target_obj_ = nullptr;
   invoke_ = invoke_static_handler<static_handler, R, Arg...>;
}

template <typename R, typename... Arg>
R QuickFunction<R(Arg...)>::operator()(Arg... arg) {
   return invoke_(target_obj_, std::forward<Arg>(arg)...);
}

} // namespace qfun
