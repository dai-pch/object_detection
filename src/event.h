#ifndef EVENT_H_
#define EVENT_H_

#include <queue>
#include <map>
#include "cycque.h"

using event_id_t = unsigned;
using callback_t = void(void);

class trigger {
public:
	trigger(const event_id_t EventId = 0, const unsigned Priority = 500)
			: event_id(EventId), priority(Priority) {}
	friend bool operator<(const trigger& a, const trigger& b) {
		// the priority of which with smaller '_priority' is higher
		return a.priority > b.priority;
	}

	event_id_t event_id;
	unsigned priority;
};

class EventManager {
public:
	void register_function(event_id_t event_id, callback_t* callback) {
		_events[event_id].push_back(callback);
	}
	void emit(event_id_t event_id, unsigned priority = 500){
		_buffer.push(trigger(event_id, priority));
	}
	void main_loop() {
		while (1) {
			flush();
			if (!_queue.empty()) {
				auto tmp = _queue.top();
				auto id = tmp.event_id;
				auto it = _events.find(id);
				if (it == _events.end()) {
					warn(tmp);
				} else {
					for (auto func: it->second) {
						(*func)();
					}
				}
				_queue.pop();
			}
		}
	}

private:
	void warn(const trigger& trig) {
		return;
	}
	void flush() {
		trigger tmp;
		while (_buffer.pop(tmp))
			_queue.push(tmp);
	}

private:
	std::map<event_id_t, std::vector<callback_t*>> _events;
	std::priority_queue<trigger> _queue;
	cycque<trigger, 64> _buffer;
};

#endif //EVENT_H_
