lib_LTLIBRARIES = libxscript.la

libxscript_la_SOURCES = args.cpp authorizer.cpp block.cpp component.cpp cached_object.cpp \
	config.cpp context.cpp cookie.cpp encoder.cpp cookie_param.cpp \
	extension.cpp extension_list.cpp external.cpp \
	header_param.cpp simple_param.cpp http_user_param.cpp \
	loader.cpp logger.cpp object.cpp param.cpp param_factory.cpp \
	query_arg_param.cpp renamed_block.cpp request.cpp request_param.cpp request_data.cpp \
	request_arg_param.cpp request_file.cpp request_data_param.cpp response.cpp \
	sanitizer.cpp script.cpp script_factory.cpp state.cpp policy.cpp \
	state_arg_param.cpp state_impl.cpp state_param.cpp \
	stylesheet.cpp stylesheet_factory.cpp tag.cpp \
	tag_param.cpp tagged_block.cpp remote_tagged_block.cpp xslt_conv.cpp \
	threaded_block.cpp util.cpp xml_util.cpp writer.cpp xml.cpp xslt_extension.cpp \
	vhost_data.cpp protocol_arg_param.cpp punycode_utils.cpp \
	logger_factory.cpp syslog_logger.cpp file_logger.cpp server.cpp \
	control_extension.cpp operation_mode.cpp \
	thread_pool.cpp script_cache.cpp stylesheet_cache.cpp \
	stat_builder.cpp hostname_param.cpp status_info.cpp \
	protocol.cpp http_helper.cpp \
	doc_cache.cpp doc_cache_strategy.cpp cache_strategy.cpp cache_strategy_collector.cpp \
	profiler.cpp xslt_profiler.cpp \
	average_counter_factory.cpp simple_counter_factory.cpp \
	cache_counter_factory.cpp cache_usage_counter_factory.cpp \
	tagged_cache_usage_counter_factory.cpp \
	average_counter_impl.cpp simple_counter_impl.cpp cache_counter_impl.cpp \
	cache_usage_counter_impl.cpp tagged_cache_usage_counter_impl.cpp \
	dummy_average_counter.cpp dummy_simple_counter.cpp \
	dummy_cache_counter.cpp dummy_cache_usage_counter.cpp \
	dummy_tagged_cache_usage_counter.cpp \
	validator_factory.cpp validator.cpp validator_exception.cpp \
	string_utils.cpp refresher.cpp vhost_arg_param.cpp guard_checker.cpp \
	message_interface.cpp message_errors.cpp exception.cpp \
	parser.cpp request_impl.cpp http_utils.cpp \
	dummy_response_time_counter.cpp response_time_counter_impl.cpp response_time_counter_factory.cpp \
	response_time_counter_block.cpp invoke_context.cpp typed_map.cpp local_arg_param.cpp meta_block.cpp \
	block_helpers.cpp meta.cpp json2xml.cpp

libxscript_la_LDFLAGS = @VERSION_INFO@ -lcurl

libxscript_la_LIBADD = -lpthread @LTLIBICONV@ @BOOST_THREAD_LDFLAGS@ @BOOST_FILESYSTEM_LDFLAGS@

AM_CXXFLAGS = -W -Wall -fexceptions -frtti -fPIC -ftemplate-depth-128 -finline -pthread -I@top_srcdir@/include -I@builddir@ @yandex_platform_CFLAGS@
AM_LDFLAGS = @BOOST_THREAD_LDFLAGS@ @BOOST_FILESYSTEM_LDFLAGS@ @yandex_platform_LIBS@

SUBDIRS = details
