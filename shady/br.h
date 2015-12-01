#pragma once

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
}
