lib_LTLIBRARIES = libxscript.la

libxscript_la_SOURCES = args.cpp authorizer.cpp block.cpp component.cpp \
	config.cpp context.cpp cookie.cpp encoder.cpp cookie_param.cpp \
	extension.cpp extension_list.cpp header_param.cpp http_user_param.cpp \
	loader.cpp logger.cpp object.cpp param.cpp param_factory.cpp \
	query_arg_param.cpp request.cpp request_param.cpp request_data.cpp \
	request_data_param.cpp response.cpp request_impl.cpp parser.cpp request_response.cpp \
	sanitizer.cpp script.cpp script_factory.cpp state.cpp \
	state_arg_param.cpp state_impl.cpp state_param.cpp state_value.cpp \
	stylesheet.cpp stylesheet_factory.cpp tag.cpp \
	tag_param.cpp tagged_block.cpp remote_tagged_block.cpp \
	threaded_block.cpp util.cpp xml_util.cpp writer.cpp xml.cpp xslt_extension.cpp \
	vhost_data.cpp protocol_arg_param.cpp checking_policy.cpp \
	logger_factory.cpp syslog_logger.cpp file_logger.cpp server.cpp \
	control_extension.cpp operation_mode.cpp \
	dummy_thread_pool.cpp dummy_script_cache.cpp \
	dummy_stylesheet_cache.cpp \
	stat_builder.cpp \
	status_info.cpp \
	memory_statistic.cpp \
	doc_cache.cpp doc_cache_strategy.cpp \
	profiler.cpp xslt_profiler.cpp \
	default_average_counter_factory.cpp \
	default_simple_counter_factory.cpp \
	default_cache_counter_factory.cpp \
	average_counter_impl.cpp \
	simple_counter_impl.cpp \
	cache_counter_impl.cpp \
	dummy_average_counter.cpp \
	dummy_simple_counter.cpp \
	dummy_cache_counter.cpp \
	default_request_response.cpp \
	validator_factory.cpp validator_base.cpp validator_exception.cpp

libxscript_la_LDFLAGS = -lpthread @LTLIBICONV@ -lboost_thread -lboost_regex -lboost_filesystem

AM_CXXFLAGS = -Wall -fexceptions -frtti -ftemplate-depth-128 -finline -pthread -I@top_srcdir@/include -I@builddir@ @yandex_platform_CFLAGS@
AM_LDFLAGS = -lboost_thread -lboost_regex -lboost_filesystem -I@top_srcdir@/include @yandex_platform_LIBS@

SUBDIRS = details
