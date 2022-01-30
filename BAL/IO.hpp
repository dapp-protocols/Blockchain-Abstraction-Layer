#pragma once

#include <string_view>

namespace BAL {

// Abort execution of the contract with the provided message as the error
void Abort(std::string_view message);
// Verify that the condition is true, or else abort the execution of the contract and log message as the error.
void Verify(bool condition, std::string_view message);
// Log the provided message without interrupting the contract.
void Log(std::string_view message);

// Convenience templates to invoke the above with various types in the message. Implementations are platform specific
template<typename T, typename... Ts>
inline void Abort(const T& first, const Ts&... others);
template<typename T, typename... Ts>
inline void Verify(bool condition, const T& first, const Ts&... others);
template<typename T, typename... Ts>
inline void Log(T first, Ts... others);

} // namespace BAL
