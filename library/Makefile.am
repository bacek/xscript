lib_LTLIBRARIES = libxscript.la

libxscript_la_SOURCES = args.cpp authorizer.cpp block.cpp component.cpp \
	config.cpp context.cpp cookie.cpp encoder.cpp cookie_param.cpp \
	extension.cpp extension_list.cpp header_param.cpp http_user_param.cpp \
	loader.cpp logger.cpp object.cpp param.cpp param_factory.cpp \
	query_arg_param.cpp request.cpp request_param.cpp request_data.cpp \
	request_data_param.cpp response.cpp request_impl.cpp parser.cpp \
	sanitizer.cpp script.cpp script_factory.cpp state.cpp \
	state_arg_param.cpp state_impl.cpp state_param.cpp state_value.cpp \
	stylesheet.cpp stylesheet_factory.cpp tag.cpp \
	tag_param.cpp tagged_block.cpp tagged_cache.cpp \
	threaded_block.cpp util.cpp writer.cpp xml.cpp xslt_extension.cpp \
	vhost_data.cpp protocol_arg_param.cpp checking_policy.cpp \
	logger_factory.cpp syslog_logger.cpp file_logger.cpp server.cpp \
	control_extension.cpp \
	dummy_tagged_cache.cpp dummy_thread_pool.cpp dummy_script_cache.cpp \
	dummy_stylesheet_cache.cpp \
	stat_builder.cpp average_counter.cpp simple_counter.cpp \
	status_info.cpp \
	memory_statistic.cpp

noinst_HEADERS = details/expect.h details/extension_list.h details/loader.h \
	details/param_factory.h details/phoenix_singleton.h \
	details/state_impl.h details/writer_impl.h details/xml_config.h \
	details/syslog_logger.h details/file_logger.h \
	details/dummy_tagged_cache.h details/dummy_thread_pool.h details/dummy_script_cache.h \
	details/dummy_stylesheet_cache.h

libxscript_la_LDFLAGS = -lpthread -ldl

AM_CPPFLAGS = -I../include -I../config -D_REENTRANT
AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem -lcrypto
