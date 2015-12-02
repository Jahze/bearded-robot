#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

namespace br
{
	template<typename T, typename U>
	bool none_of(T && t, U && u)
	{
		return t != u;
	}

	template<typename T, typename U, typename... V>
	bool none_of(T && t, U && u, V &&... v)
	{
		return t != u && none_of(std::forward<T>(t), std::forward<V>(v)...);
	}

	template<typename T, typename U>
	bool one_of(T && t, U && u)
	{
		return t == u;
	}

	template<typename T, typename U, typename... V>
	bool one_of(T && t, U && u, V &&... v)
	{
		return t == u || one_of(std::forward<T>(t), std::forward<V>(v)...);
	}

	template<typename T, class = std::enable_if<std::is_integral<T>::value>::type>
	std::vector<uint8_t> as_bytes(T t)
	{
		std::vector<uint8_t> out;
		out.resize(sizeof(T));
		std::memcpy(out.data(), &t, sizeof(T));
		return out;
	}
}
