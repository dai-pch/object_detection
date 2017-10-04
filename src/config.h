#ifndef CONFIG_H_
#define CONFIG_H_

#include <stdexcept>
#include "xparameters.h"
#include "xil_printf.h"
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
	E_EMAC,
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
	EventsNum
};

#define OD_DEBUG_INFO 1

#if (OD_DEBUG_INFO == 1)
#define LOG(...) do {xil_printf("Log info in file: %s, line: %d, function: %s",\
		__FILE__, __LINE__, __func__);xil_printf(__VA_ARGS__);} while(0)
#define FATAL(...) do {xil_printf("Log info in file: %s, line: %d, function: %s",\
		__FILE__, __LINE__, __func__);xil_printf(__VA_ARGS__);\
		throw std::runtime_error("");} while(0)
#else
#define LOG(...)
#define FATAL(...)
#endif // OD_DEBUG_INFO

#endif //CONFIG_H_
