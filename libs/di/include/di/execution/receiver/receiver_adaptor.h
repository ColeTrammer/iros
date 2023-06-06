#pragma once

#include <di/concepts/class_type.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_stopped.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/prelude.h>
#include <di/function/tag_invoke.h>
#include <di/util/store_if.h>
#include <di/vocab/error/prelude.h>

namespace di::execution {
namespace receiver_interface_ns {
    namespace fake_receiver {
        struct FakeReceiver {
            using is_receiver = void;
        };
        void tag_invoke(SetValue, FakeReceiver&&);
        void tag_invoke(SetError, FakeReceiver&&, Error);
        void tag_invoke(SetStopped, FakeReceiver&&);
    }

    template<typename T, typename U>
    requires(concepts::DecaysTo<T, T>)
    meta::Like<U&&, T> c_style_cast(U&& u) noexcept {
        static_assert(concepts::BaseOf<T, meta::RemoveReference<U>>);
        return (meta::Like<U&&, T>) util::forward<U>(u);
    }

    template<typename Self, typename Base>
    struct ReceiverAdaptor {
        struct Type {
        private:
            friend Self;

            constexpr static bool has_base = !concepts::SameAs<Base, fake_receiver::FakeReceiver>;

            template<typename S>
            using GetDerivedBase = decltype(util::declval<S>().base());

            using BaseTypeImpl =
                meta::Conditional<has_base, meta::BindBack<meta::Quote<meta::Like>, Base>, meta::Quote<GetDerivedBase>>;

            template<typename S>
            using BaseType = meta::Invoke<BaseTypeImpl, S&&>;

            // The base receiver is either stored in the adaptor itself, or in the
            // derived class. The type of this function must be explicitly provided,
            // since it cannot be deduced (in the case where there is a member base()
            // function, since Self is currently incomplete).
            template<typename S>
            static BaseType<S> get_base(S&& self) {
                if constexpr (!has_base) {
                    return util::forward<S>(self).base();
                } else {
                    return c_style_cast<Type>(util::forward<S>(self)).base();
                }
            }

            Base& base() &
                requires(has_base)
            {
                return this->m_base.value;
            }
            Base const& base() const&
            requires(has_base)
            {
                return this->m_base.value;
            }
            Base&& base() &&
                requires(has_base)
            {
                return util::move(this->m_base).value;
            }
            Base const&& base() const&&
            requires(has_base)
            {
                return util::move(this->m_base).value;
            }

        public:
            using is_receiver = void;

            Type() = default;

            template<typename T>
            requires(has_base && concepts::ConstructibleFrom<Base, T>)
            explicit Type(T&& value) : m_base(util::forward<T>(value)) {}

        private:
            // The following mechanism is used to forward the set_value, set_error,
            // set_stopped, and get_env CPOs to either the base receiver or a
            // member function in Self. Forwarding directly to the base receiver
            // is only done when the corresponding member function is not defined.
            // This is detected by having these variables of the same name in this
            // class, and then seeing if they are properly accessible through the
            // derived Self class. Additionally, calls to the Self member functions
            // must be done in a static templated method to prevent the compiler
            // from interacting with them while Self is still an incomplete type.
            // Additionally, the actually forwarding logic needs multiple redundant
            // template parameters to defer evaluation of the function's constraints
            // while Self is an incomplete type.
            constexpr static int set_value = 1;
            constexpr static int set_error = 1;
            constexpr static int set_stopped = 1;
            constexpr static int set_next = 1;
            constexpr static int get_env = 1;

            template<typename S>
            constexpr static bool missing_set_value() {
                return requires { requires bool(int(S::set_value)); };
            }
            template<typename S>
            constexpr static bool missing_set_error() {
                return requires { requires bool(int(S::set_error)); };
            }
            template<typename S>
            constexpr static bool missing_set_stopped() {
                return requires { requires bool(int(S::set_stopped)); };
            }
            template<typename S>
            constexpr static bool missing_set_next() {
                return requires { requires bool(int(S::set_next)); };
            }
            template<typename S>
            constexpr static bool missing_get_env() {
                return requires { requires bool(int(S::get_env)); };
            }

            template<typename S, typename... Args>
            static auto do_set_value(S&& self, Args&&... args)
                -> decltype(util::forward<S>(self).set_value(util::forward<Args>(args)...)) {
                return util::forward<S>(self).set_value(util::forward<Args>(args)...);
            }
            template<typename S, typename... Args>
            static auto do_set_error(S&& self, Args&&... args)
                -> decltype(util::forward<S>(self).set_error(util::forward<Args>(args)...)) {
                return util::forward<S>(self).set_error(util::forward<Args>(args)...);
            }
            template<typename S, typename... Args>
            static auto do_set_stopped(S&& self, Args&&... args)
                -> decltype(util::forward<S>(self).set_stopped(util::forward<Args>(args)...)) {
                return util::forward<S>(self).set_stopped(util::forward<Args>(args)...);
            }
            template<typename S, typename... Args>
            static auto do_set_next(S& self, Args&&... args) -> decltype(self.set_next(util::forward<Args>(args)...)) {
                return self.set_next(util::forward<Args>(args)...);
            }
            template<typename S, typename... Args>
            static auto do_get_env(S&& self, Args&&... args)
                -> decltype(util::forward<S>(self).get_env(util::forward<Args>(args)...)) {
                return util::forward<S>(self).get_env(util::forward<Args>(args)...);
            }

            template<concepts::SameAs<SetValue> Tag, typename S = Self, typename... Args>
            friend void tag_invoke(Tag, Self&& self, Args&&... args)
            requires(
                requires { Type::do_set_value(util::move(self), util::forward<Args>(args)...); } ||
                requires {
                    requires missing_set_value<S>();
                    execution::set_value(Type::get_base(util::move(self)), util::forward<Args>(args)...);
                })
            {
                if constexpr (requires { Type::do_set_value(util::move(self), util::forward<Args>(args)...); }) {
                    Type::do_set_value(util::move(self), util::forward<Args>(args)...);
                } else {
                    execution::set_value(Type::get_base(util::move(self)), util::forward<Args>(args)...);
                }
            }

            template<concepts::SameAs<SetError> Tag, typename Arg, typename S = Self>
            friend void tag_invoke(Tag, Self&& self, Arg&& arg)
            requires(
                requires { Type::do_set_error(util::move(self), util::forward<Arg>(arg)); } ||
                requires {
                    requires missing_set_error<S>();
                    execution::set_error(Type::get_base(util::move(self)), util::forward<Arg>(arg));
                })
            {
                if constexpr (requires { Type::do_set_error(util::move(self), util::forward<Arg>(arg)); }) {
                    Type::do_set_error(util::move(self), util::forward<Arg>(arg));
                } else {
                    execution::set_error(Type::get_base(util::move(self)), util::forward<Arg>(arg));
                }
            }

            template<concepts::SameAs<SetStopped> Tag, typename S = Self>
            friend void tag_invoke(Tag, Self&& self)
            requires(
                requires { Type::do_set_stopped(util::move(self)); } ||
                requires {
                    requires missing_set_stopped<S>();
                    execution::set_stopped(Type::get_base(util::move(self)));
                })
            {
                if constexpr (requires { Type::do_set_stopped(util::move(self)); }) {
                    Type::do_set_stopped(util::move(self));
                } else {
                    execution::set_stopped(Type::get_base(util::move(self)));
                }
            }

            template<concepts::SameAs<types::Tag<execution::set_next>> Tag, typename N, typename S = Self>
            friend decltype(auto) tag_invoke(Tag, Self& self, N&& next)
            requires(
                requires { Type::do_set_next(self, util::forward<N>(next)); } ||
                requires {
                    requires missing_set_next<S>();
                    execution::set_next(Type::get_base(self), util::forward<N>(next));
                })
            {
                if constexpr (requires { Type::do_set_next(self, util::forward<N>(next)); }) {
                    return Type::do_set_next(self, util::forward<N>(next));
                } else {
                    return execution::set_next(Type::get_base(self), util::forward<N>(next));
                }
            }

            template<concepts::SameAs<types::Tag<execution::get_env>> Tag, typename S = Self>
            friend decltype(auto) tag_invoke(Tag, Self const& self)
            requires(
                requires { Type::do_get_env(util::move(self)); } ||
                requires {
                    requires missing_get_env<S>();
                    execution::get_env(Type::get_base(self));
                })
            {
                if constexpr (requires { Type::do_get_env(util::move(self)); }) {
                    return Type::do_get_env(util::move(self));
                } else {
                    return make_env(execution::get_env(Type::get_base(self)));
                }
            }

            [[no_unique_address]] util::StoreIf<Base, has_base> m_base;
        };
    };
}

template<concepts::ClassType Self, concepts::Receiver Base = receiver_interface_ns::fake_receiver::FakeReceiver>
using ReceiverAdaptor = meta::Type<receiver_interface_ns::ReceiverAdaptor<Self, Base>>;
}
