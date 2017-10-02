#ifndef DETECT_H_
#define DETECT_H_

typedef enum img_class{
	BackGround = 0
} img_class;

struct detect_res {
	unsigned picture_id;
	double tl_x, tl_y, br_x, br_y;
	img_class class_id;
	double p;
};

#endif //DETECT_H_
