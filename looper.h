#pragma once
#include "stdafx.h"
metadb_handle_ptr GetOriginItem();
class looper
{
public:
	void open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort);

	void get_info(file_info & p_info, abort_callback & p_abort);

	t_filestats get_file_stats(abort_callback & p_abort);

	void decode_initialize(unsigned p_flags, abort_callback & p_abort);

	bool decode_run(audio_chunk & p_chunk, abort_callback & p_abort);

	bool decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort);

	void set_logger(event_logger::ptr ptr);

	void decode_seek(double p_seconds, abort_callback & p_abort);

	bool decode_can_seek();

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta);
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta);

	void decode_on_idle(abort_callback & p_abort);

	void retag(const file_info & p_info, abort_callback & p_abort);

	static bool g_is_our_content_type(const char * p_content_type);
	static bool g_is_our_path(const char * p_path, const char * p_extension);

private:
	service_ptr_t<file> m_file;
	WAVEFORMATEX waveFormat;
	foobar2000_io::t_sfilesize pcmStart;
	sampleIndex currentIndex;
	sampleIndex fullCount;
	char*buffer;
};

sampleIndex GetLoopStart();
sampleIndex GetLoopEnd();
sampleIndex PlaybackPosition(sampleIndex setValue);