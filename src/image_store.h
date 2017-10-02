#ifndef IMAGE_STORE_H_
#define IMAGE_STORE_H_

#include <memory>

struct jpeg_stream {
	void* data;
	size_t length;
	unsigned id;
};

template <typename T, size_t Size>
class rgb_image {
public:
	rgb_image(): data(std::shared_ptr<T>(new T[Size], std::default_delete<T[]>())) {
		if (!data)
            throw std::runtime_error("No enough memory when construct rbg_imge.");
	}
	~rgb_image() {}

public:
	const std::shared_ptr<T> data;
	unsigned id;
};

#endif //IMAGE_STORE_H_
