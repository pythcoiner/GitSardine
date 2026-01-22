#ifndef RESULT_H
#define RESULT_H

#include <QString>
#include <variant>
#include <utility>

/**
 * Result<T, E> - Rust-like error handling for C++
 *
 * Usage:
 *   Result<QString, QString> result = Result<QString, QString>::Ok("success");
 *   if (result.isOk()) {
 *       qDebug() << result.value();
 *   }
 *
 *   Result<QString, QString> error = Result<QString, QString>::Err("failed");
 *   if (error.isErr()) {
 *       qDebug() << error.error();
 *   }
 */
template<typename T, typename E = QString>
class Result {
public:
    static Result Ok(T value) {
        Result r;
        r.m_data = std::move(value);
        r.m_isOk = true;
        return r;
    }

    static Result Err(E error) {
        Result r;
        r.m_error = std::move(error);
        r.m_isOk = false;
        return r;
    }

    bool isOk() const { return m_isOk; }
    bool isErr() const { return !m_isOk; }

    const T& value() const { return m_data; }
    T& value() { return m_data; }

    const E& error() const { return m_error; }
    E& error() { return m_error; }

    // Monadic map operation
    template<typename F>
    auto map(F&& f) const -> Result<decltype(f(std::declval<T>())), E> {
        using U = decltype(f(std::declval<T>()));
        if (isOk()) {
            return Result<U, E>::Ok(f(m_data));
        }
        return Result<U, E>::Err(m_error);
    }

    // Monadic mapErr operation
    template<typename F>
    auto mapErr(F&& f) const -> Result<T, decltype(f(std::declval<E>()))> {
        using U = decltype(f(std::declval<E>()));
        if (isErr()) {
            return Result<T, U>::Err(f(m_error));
        }
        return Result<T, U>::Ok(m_data);
    }

    // Unwrap with default value
    T valueOr(T defaultValue) const {
        if (isOk()) {
            return m_data;
        }
        return defaultValue;
    }

private:
    Result() : m_isOk(false) {}

    T m_data;
    E m_error;
    bool m_isOk;
};

// Specialization for void-like results using std::monostate
struct VoidValue {};

using VoidResult = Result<VoidValue, QString>;

// Helper to create void ok result
inline VoidResult OkVoid() {
    return VoidResult::Ok(VoidValue{});
}

#endif // RESULT_H
