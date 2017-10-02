#ifndef SINGLETON_H_
#define SINGLETON_H_

template <typename T>
inline T& get_instance() {
	static T single_instance;
	return single_instance;
}

#endif //SINGLETON_H_
