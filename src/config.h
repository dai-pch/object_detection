#ifndef CONFIG_H_
#define CONFIG_H_

#include "cycque.h"
#include "image_store.h"
#include "detect.h"

//config
using img_t = unsigned char;
constexpr unsigned ImageHight = 300;
constexpr unsigned ImageWigth = 300;
constexpr unsigned ImageSize = ImageHight * ImageWigth;

using jpg_q = cycque<jpeg_stream>;
using img_q = cycque<rgb_image<img_t, ImageSize> >;
using res_q = cycque<detect_res>;

enum events_id {
	E_TcpFastTimmer,
	E_TcpSlowTimmer,
	E_TcpConnected,
	E_TcpConnectionError,
	E_HttpResponseError,
	E_HwRequireData,
	E_HwRequireDataResponsed,
	E_HwRequireResAddr,
	E_HwRequireResAddrResponsed,
	E_HttpNeedSendRes,
	E_HttpResSent,
	E_HttpReceivedJpg,
	E_JpgDecode,
	E_JpgDecoded,
	E_EventsNum
};

#endif //CONFIG_H_
