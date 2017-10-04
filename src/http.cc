#include <string>
#include <map>
#include <cstdlib>
#include <stdexcept>
#include "xil_printf.h"
#include "http.h"
#include "singleton.h"
#include "event.h"
#include "config.h"
#include "http_parser.h"

using std::string;
using std::map;

string creat_http_head(const HttpRequestType type,
		const string& addr, const string& url, const unsigned port = 0) {
	string head;
	// request line
	switch (type) {
	case HttpRequestType::HttpGet:
		head += "GET ";
		break;
	case HttpRequestType::HttpPost:
		head += "POST ";
		break;
	}
	head += "http://";
	head += addr;
	if (port != 0) {
		char tmp[10];
		head += ":";
		head += itoa(port, tmp, 10);
	}
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

	LOG("TCP connection started.\n\r");

	return err;
}

bool HttpConnection::get(const std::string& url,
		const std::map<std::string, std::string>& contents) {
	if (state != HttpState::SHttpReady)
		return false;
	string encode;
	encode = url + "?";
	for (auto ele: contents) {
		encode += ele.first + "=" + ele.second + "&";
	}
	encode.pop_back();

	auto head = creat_http_head(HttpRequestType::HttpGet,
				addr, encode, dest_port);
	err_t err = tcp_write(tpcb, head.c_str(),
				head.length() * sizeof(char), TCP_WRITE_FLAG_COPY);
	if (err == ERR_OK) {
		LOG("Http get require sent. contents:\n%s", head.c_str());
		return true;
	} else {
		return false;
	}
}

bool HttpConnection::post(const std::string& url, const char* contents, size_t length) {
	if (state != HttpState::SHttpReady)
		return false;
	auto head = creat_http_head(HttpRequestType::HttpPost,
			addr, url, dest_port);

	err_t err = tcp_write(tpcb, head.c_str(),
			head.length() * sizeof(char), TCP_WRITE_FLAG_COPY);
	err_t err2 = tcp_write(tpcb, contents, length, TCP_WRITE_FLAG_COPY);
	if (err == ERR_OK && err2 == ERR_OK) {
		LOG("Http post require sent. contents:\n%s%s", head.c_str(), contents);
		return true;
	} else {
		return false;
	}
}

bool HttpConnection::post(const std::string& url,
		const std::map<std::string, std::string>& contents) {
	if (state != HttpState::SHttpReady)
		return false;
	auto head = creat_http_head(HttpRequestType::HttpPost,
			addr, url, dest_port);
	string cts;
	for (auto ele: contents) {
		cts += ele.first + '=' + ele.second + '\n';
	}

	err_t err = tcp_write(tpcb, head.c_str(),
			head.length() * sizeof(char), TCP_WRITE_FLAG_COPY);
	err_t err2 = tcp_write(tpcb, cts.c_str(), cts.length(), TCP_WRITE_FLAG_COPY);
	if (err == ERR_OK && err2 == ERR_OK) {
		LOG("Http post require sent. contents:\n%s%s", head.c_str(), contents);
		return true;
	} else {
		return false;
	}
}

bool HttpConnection::get_image(const unsigned id) {
	if (state != HttpState::SHttpReady)
		return false;
	const string url = "/image";
	state = HttpState::SHttpGetting;
	get_id = id;

	map<string, string> c;
	c["token"] = string(token);
	char tmp[10];
	c["image_name"] = itoa(id, tmp, 10);
	bool res = post(url, c);
	if (res) {
		return true;
	} else {
		state = HttpState::SHttpReady;
		get_id = -1;
		return false;
	}
}

bool HttpConnection::post_result(const string& url,
		const std::map<std::string, std::string>& contents){
	if (state != HttpState::SHttpReady)
		return false;
	state = HttpState::SHttpPosting;
	get_id = -1;

	auto c = contents;
	c["token"] = token;
	bool res = post(url, c);
	if (res) {
		return true;
	} else {
		state = HttpState::SHttpReady;
		return false;
	}
}

bool HttpConnection::login(const string& username, const string& password) {
	map<string, string> c;
	c["username"] = username;
	c["password"] = password;
	if (state != HttpState::SHttpReady)
			return false;
	state = HttpState::SHttpLogging;
	get_id = -1;
 	LOG("Log in ing");

	bool res = post("/login", c);
	if (res) {
		return true;
	} else {
		state = HttpState::SHttpReady;
		return false;
	}
}

std::string HttpConnection::print_ip(const ip_addr_t *ip) const {
	char tmp[4];
	string res;
	res += itoa(ip4_addr1(ip), tmp, 10) + '.';
	res += itoa(ip4_addr2(ip), tmp, 10) + '.';
	res += itoa(ip4_addr3(ip), tmp, 10) + '.';
	res += itoa(ip4_addr4(ip), tmp, 10);
	return res;
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
		LOG("Connection closed. reconnecting...");
		connect();
		return ERR_OK;
	}
	LOG("Response received.");
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
		data_len = length + 1;
		memcpy(data, at, length);
		data[data_len - 1] = '\0';
	} else {
		data = (char*)realloc(data, data_len + length);
		memcpy(((char*)data + data_len), at, length);
		data_len += length;
		data[data_len - 1] = '\0';
	}
	return 0;
}

int HttpConnection::_parse_on_message_complete_callback(http_parser *parser) {
	auto& event = get_instance<EventManager>();
	if (parser->status_code != 200) {
		event.emit(E_HttpResponseError);
		return -1;
	}
	LOG("Received http response:\n%s", data);
	switch (state) {
	case HttpState::SHttpLogging: {
		token = data;
		token_len = data_len;
		data = NULL;
		data_len = 0;
		break;
	}
	case HttpState::SHttpPosting: {
		free(data);
		data_len = 0;
		break;
	}
	case HttpState::SHttpGetting: {
		jpeg_stream jpg;
		jpg.data = data;
		jpg.length = data_len;
		jpg.id = get_id;
		auto& jpg_que = get_instance<jpg_q>();
		jpg_que.push(jpg);
		data = NULL;
		break;
	}
	default: {
		FATAL("Unsupported state in receive http.");
		break;
	}
	}
	get_id = -1;
	state = HttpState::SHttpReady;
	return 0;
}

void HttpConnection::_close() {
	err_t err = tcp_close(tpcb);
	if (err != ERR_OK)
		FATAL("Tcp connection close failed.");
}
