#include "settings.h"

#include <set>
#include <cctype>
#include <cassert>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/bind.hpp>

#include "parser.h"
#include "request.h"
#include "xscript/range.h"
#include "xscript/encoder.h"
#include "internal/algorithm.h"

#ifdef HAVE_DMALLOC_H
#include <dmalloc.h>
#endif

namespace xscript
{

const Range Parser::RN_RANGE = createRange("\r\n");
const Range Parser::NAME_RANGE = createRange("name");
const Range Parser::FILENAME_RANGE = createRange("filename");
const Range Parser::HEADER_RANGE = createRange("HTTP_");
const Range Parser::COOKIE_RANGE = createRange("HTTP_COOKIE");
const Range Parser::EMPTY_LINE_RANGE = createRange("\r\n\r\n");
const Range Parser::CONTENT_TYPE_RANGE = createRange("CONTENT_TYPE");

const char*
Parser::statusToString(short status) {
	
	switch (status) {
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";

		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";

		case 400: return "Bad request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";

		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
	}
	return "Unknown status";
}

std::string
Parser::getBoundary(const Range &range) {
	
	Range head, tail;
	split(range, ';', head, tail);
	
	tail = trim(tail);
	if (strncasecmp("boundary", tail.begin(), sizeof("boundary") - 1) == 0) {
		Range key, value;
		split(tail, '=', key, value);
		Range boundary = trim(value);
		return std::string("--").append(boundary.begin(), boundary.end());
	}
	throw std::runtime_error("no boundary found");
}

void
Parser::addCookie(ServerRequest *req, const Range &range, Encoder *encoder) {
	
	Range part = trim(range), head, tail;
	split(part, '=', head, tail);
	if (!head.empty()) {
		std::string key = StringUtils::urldecode(head), value = StringUtils::urldecode(tail);
		if (!xmlCheckUTF8((const xmlChar*) key.c_str())) {
			encoder->encode(key).swap(key);
		}
		if (!xmlCheckUTF8((const xmlChar*) value.c_str())) {
			encoder->encode(value).swap(value);
		}
		req->cookies_[key].swap(value);
	}
}

void
Parser::addHeader(ServerRequest *req, const Range &key, const Range &value, Encoder *encoder) {
	std::string header = normalizeInputHeaderName(key);
	if (!xmlCheckUTF8((const xmlChar*) value.begin())) {
		encoder->encode(value).swap(req->headers_[header]);
	}
	else {
		std::string &str = req->headers_[header];
		str.reserve(value.size());
		str.assign(value.begin(), value.end());
	}
}

void
Parser::parse(ServerRequest *req, char *env[], Encoder *encoder) {
	for (int i = 0; NULL != env[i]; ++i) {
		Range key, value;
		split(createRange(env[i]), '=', key, value);
		if (COOKIE_RANGE == key) {
			parseCookies(req, value, encoder);
			addHeader(req, truncate(key, HEADER_RANGE.size(), 0), trim(value), encoder);
		}
		else if (CONTENT_TYPE_RANGE == key) {
			addHeader(req, key, trim(value), encoder);
		}
		else if (startsWith(key, HEADER_RANGE)) {
			addHeader(req, truncate(key, HEADER_RANGE.size(), 0), trim(value), encoder);
		}
		else {
			std::string name(key.begin(), key.end());
			if (!xmlCheckUTF8((const xmlChar*) value.begin())) {
				encoder->encode(value).swap(req->vars_[name]);
			}
			else {
				std::string &str = req->vars_[name];
				str.reserve(value.size());
				str.assign(value.begin(), value.end());
			}
		}
	}
}

void
Parser::parseCookies(ServerRequest *req, const Range &range, Encoder *encoder) {
	Range part = trim(range), head, tail;
	while (!part.empty()) {
		split(part, ';', head, tail);
		addCookie(req, head, encoder);
		part = trim(tail);
	}
}

void
Parser::parseLine(Range &line, std::map<Range, Range, RangeCILess> &m) {
	
	Range head, tail, name, value;
	while (!line.empty()) {
		split(line, ';', head, tail);
		split(head, '=', name, value);
		if (NAME_RANGE == name || FILENAME_RANGE == name) {
			value = truncate(value, 1, 1);
		}
		m.insert(std::make_pair(name, value));
		line = trim(tail);
	}
}

void
Parser::parsePart(ServerRequest *req, Range &part, Encoder *encoder) {
	
	Range headers, content, line, tail;
	std::map<Range, Range, RangeCILess> params;
	
	split(part, EMPTY_LINE_RANGE, headers, content);
	while (!headers.empty()) {
		split(headers, RN_RANGE, line, tail);
		parseLine(line, params);
		headers = tail;
	}
	std::map<Range, Range, RangeCILess>::iterator i = params.find(NAME_RANGE);
	if (params.end() == i) {
		return;
	}
	
	std::string name(i->second.begin(), i->second.end());
	if (!xmlCheckUTF8((const xmlChar*) name.c_str())) {
		encoder->encode(name).swap(name);
	}

	if (params.end() != params.find(FILENAME_RANGE)) {
		req->files_.insert(std::make_pair(name, File(params, content)));
	}
	else {
		std::pair<std::string, std::string> p;
		p.first.swap(name);
		p.second.assign(content.begin(), content.end());
		if (!xmlCheckUTF8((const xmlChar*) p.second.c_str())) {
			encoder->encode(p.second).swap(p.second);
		}
		req->args_.push_back(p);
	}
}

void
Parser::parseMultipart(ServerRequest *req, Range &data, const std::string &boundary, Encoder *encoder) {
	Range head, tail, bound = createRange(boundary);
	while (!data.empty()) {
		split(data, bound, head, tail);
		if (!head.empty()) {
			head = truncate(head, 2, 2);
		}
		if (!head.empty()) {
			parsePart(req, head, encoder);
		}	
		data = tail;
	}
}

std::string
Parser::normalizeInputHeaderName(const Range &range) {
	
	std::string res;
	res.reserve(range.size());
	
	Range part = range, head, tail;
	while (!part.empty()) {
		bool splitted = split(part, '_', head, tail);
		res.append(head.begin(), head.end());
		if (splitted) {
			res.append(1, '-');
		}
		part = tail;
	}
	return res;
}

std::string
Parser::normalizeOutputHeaderName(const std::string &name) {
	
	std::string res;
	Range range = trim(createRange(name)), head, tail;
	res.reserve(range.size());
	
	while (!range.empty()) {
		
		split(range, '-', head, tail);
		if (!head.empty()) {
			res.append(1, toupper(*head.begin()));
		}
		if (head.size() > 1) {
			res.append(head.begin() + 1, head.end());
		}
		range = trim(tail);
		if (!range.empty()) {
			res.append(1, '-');
		}
	}
	return res;
}

} // namespace xscript
