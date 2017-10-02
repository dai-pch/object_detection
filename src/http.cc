#include <string>
#include <cstdlib>
#include <stdexcept>
#include "xil_printf.h"
#include "http.h"
#include "singleton.h"
#include "event.h"
#include "config.h"
#include "http_parser.h"

std::string creat_http_head(const HttpRequestType type,
		const char* addr, const char* url, const unsigned port = 0) {
	std::string head;
	// request line
	switch (type) {
	case HttpRequestType::HttpGet:
		head += "GET ";
		break;
	case HttpRequestType::HttpPost:
		head += "POST ";
		break;
	}
	/*head += "http://";
	head += addr;
	if (port != 0) {
		char tmp[10];
		head += ":";
		head += itoa(port, tmp, 10);
	}*/
	head += url;
	head += " HTTP/1.1\r\n";
	// header line
	head = head + "Host: " + addr + "\r\n";
	head += "Accept: */*\r\n";
	head += "Accept-Language: en\r\n";
	// header end
	head += "\r\n";
	return head;
}

err_t HttpConnection::connect() {
	err_t err;

	/* create new TCP PCB structure */
	tpcb = tcp_new();
	if (!tpcb) {
		throw std::runtime_error("Error creating PCB. Out of Memory\n\r");
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(tpcb, (void*)this);

	tcp_err(tpcb, &error_callback);
	tcp_sent(tpcb, &sent_callback);
	tcp_recv(tpcb, &recv_callback);
	/* listen for connections */
	err = tcp_connect(tpcb, &dest_ip, dest_port, &connected_callback);

#ifdef OD_DEBUG
	xil_printf("TCP connection started.\n\r");
#endif

	return err;
}

bool HttpConnection::get_image(const unsigned id) {
	if (is_waiting)
		return false;
	auto contents = creat_http_head(HttpRequestType::HttpGet,
			addr, url, dest_port);
	err_t err = tcp_write(tpcb, contents.c_str(),
			contents.length() * sizeof(char), TCP_WRITE_FLAG_COPY);
	if (err == ERR_OK) {
		is_waiting = true;
		is_get = true;
		return true;
	} else
		return false;
}

bool HttpConnection::post(const char* url, const void* contents,
		const unsigned len){
	if (is_waiting)
		return false;
	auto data = creat_http_head(HttpRequestType::HttpPost,
			addr, url, dest_port);
	err_t err = tcp_write(tpcb, data.c_str(),
			data.length() * sizeof(char), TCP_WRITE_FLAG_COPY);
	err_t err2 = tcp_write(tpcb, contents, len, TCP_WRITE_FLAG_COPY);
	if (err == ERR_OK && err2 == ERR_OK) {
		is_waiting = true;
		is_get = false;
		return true;
	} else
		return false;
}

err_t HttpConnection::_connected_callback(struct tcp_pcb *tpcb, err_t err) {
	auto& em = get_instance<EventManager> ();
	em.emit(E_TcpConnected);
	return err;
}

void HttpConnection::_error_callback(err_t err) {
	auto& em = get_instance<EventManager> ();
	em.emit(E_TcpConnectionError);
	connect();
	return;
}

err_t HttpConnection::_sent_callback(struct tcp_pcb *tpcb, u16_t len) {
	return ERR_OK;
}

err_t HttpConnection::_recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
	if (p == NULL) { // connection closed
		connect();
		return ERR_OK;
	}
	auto tmp = p;
	size_t t_len = 0;
	do {
		auto nparsed = http_parser_execute(parser,
				&parser_settings, (char*)tmp->payload, tmp->len);
		t_len += nparsed;
		tmp = tmp->next;
	} while (tmp != NULL);
	pbuf_free(p);
	tcp_recved(tpcb, t_len);
	return ERR_OK;
}

int HttpConnection::_parse_on_body_callback(http_parser *parser,
		const char *at, size_t length) {
	if (data == NULL) {
		data = (char*)malloc(length);
		data_len = length;
		memcpy(data, at, length);
	} else {
		data = (char*)realloc(data, data_len + length);
		memcpy(((char*)data + data_len), at, length);
		data_len += length;
	}
	return 0;
}

int HttpConnection::_parse_on_message_complete_callback(http_parser *parser) {
	auto& event = get_instance<EventManager>();
	if (parser->status_code != 200) {
		event.emit(E_HttpResponseError);
		return -1;
	}
	if (is_get) {
		jpeg_stream jpg;
		jpg.data = data;
		jpg.length = data_len;
		jpg.id = 0;
		auto& jpg_que = get_instance<jpg_q>();
		jpg_que.push(jpg);
	}
	data = NULL;
	return 0;
}

void HttpConnection::_close() {
	err_t err = tcp_close(tpcb);
	if (err != ERR_OK)
		throw std::runtime_error("Tcp connection close failed.");
}
