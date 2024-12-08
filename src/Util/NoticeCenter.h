#ifndef _NOTICE_CENTER_H_
#define _NOTICE_CENTER_H_

#include <functional>
#include "util.h"
#include "function_traits.h"

namespace FFZKit {

class EventDispatcher {
public:
	using Ptr = std::shared_ptr<EventDispatcher>;

	~EventDispatcher() = default;

private:
	using MapType = std::unordered_multimap<void*, Any>;

	EventDispatcher() = default;

	class InterruptException : public std::runtime_error {
	public:
		InterruptException() : std::runtime_error("InterruptException") {}
		~InterruptException() {}
	};

	template <typename... ArgsType>
	int emitEvent()

};



};  / / namespace FFZKit

#endif // _NOTICE_CENTER_H_