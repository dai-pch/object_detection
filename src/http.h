/*
 * http.h
 *
 *  Created on: Sep 23, 2017
 *      Author: pengcheng
 */

#ifndef SRC_HTTP_H_
#define SRC_HTTP_H_

#include <string>
#include <map>
#include "network.h"
#include "http_parser.h"

enum class HttpRequestType {
	HttpGet,
	HttpPost
};

enum class HttpState {
	SHttpReady,
	SHttpLogging,
	SHttpGetting,
	SHttpPosting
};

class HttpConnection {
public:
	HttpConnection() {
		parser_settings.on_message_complete
					= parse_on_message_complete_callback;
		parser_settings.on_body
					= parse_on_body_callback;
		parser = (http_parser*)malloc(sizeof(http_parser));
		http_parser_init(parser, HTTP_REQUEST);
		parser->data = (void*)this;

		addr = print_ip(&dest_ip);

		connect();
	}
	~HttpConnection() {
		_close();
		free(parser);
		free(token);
	}

public:
	bool get_image(const unsigned id);
	bool post_result(const std::string& url, const std::map<std::string, std::string>& contents);
	bool login(const std::string& username, const std::string& password);
	std::string print_ip(const ip_addr_t *ip) const;

public:
	static err_t connected_callback(void* class_ptr,
			struct tcp_pcb *tpcb, err_t err) {
		HttpConnection* this_ptr = (HttpConnection*)class_ptr;
		return this_ptr->_connected_callback(tpcb, err);
	}
	static void error_callback(void* class_ptr, err_t err) {
		HttpConnection* this_ptr = (HttpConnection*)class_ptr;
		this_ptr->_error_callback(err);
		return;
	}
	static err_t sent_callback(void* class_ptr, struct tcp_pcb *tpcb,
		u16_t len) {
		HttpConnection* this_ptr = (HttpConnection*)class_ptr;
		return this_ptr->_sent_callback(tpcb, len);
	}
	static err_t recv_callback(void* class_ptr, struct tcp_pcb *tpcb,
		struct pbuf *p, err_t err) {
		HttpConnection* this_ptr = (HttpConnection*)class_ptr;
		return this_ptr->_recv_callback(tpcb, p, err);
	}
	static int parse_on_message_complete_callback(http_parser *parser) {
		HttpConnection* this_ptr = (HttpConnection*)parser->data;
		return this_ptr->_parse_on_message_complete_callback(parser);
	}
	static int parse_on_body_callback(http_parser *parser,
			const char *at, size_t length) {
		HttpConnection* this_ptr = (HttpConnection*)parser->data;
		return this_ptr->parse_on_body_callback(parser, at, length);
	}
private:
	err_t _connected_callback(struct tcp_pcb *tpcb, err_t err);
	void _error_callback(err_t err);
	err_t _sent_callback(struct tcp_pcb *tpcb, u16_t len);
	err_t _recv_callback(struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
	int _parse_on_message_complete_callback(http_parser *parser);
	int _parse_on_body_callback(http_parser *parser, const char *at, size_t length);

private:
	void _close();
	err_t connect();
	bool get(const std::string& url, const std::map<std::string, std::string>& contents);
	bool post(const std::string& url, const char* contents, size_t length);
	bool post(const std::string& url, const std::map<std::string, std::string>& contents);

private:
	ip_addr_t dest_ip;
	std::string addr;
	u16_t dest_port;

	HttpState state;
	int get_id;
	tcp_pcb* tpcb;
	char* data;
	size_t data_len;
	char* token;
	size_t token_len;

	http_parser_settings parser_settings;
	http_parser *parser;
};

#endif /* SRC_HTTP_H_ */
