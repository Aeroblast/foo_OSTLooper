#include"stdafx.h"
#include "looper.h"

void looper::open(service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort) 
{

	m_file = p_filehint;
	input_open_file_helper(m_file, p_path, p_reason, p_abort);

}
void looper::get_info(file_info & p_info, abort_callback & p_abort) 
{
	bool result=GetOriginItem()->get_info(p_info);
	
	p_info.set_length(-1);
}
t_filestats looper::get_file_stats(abort_callback & p_abort)
{
	return m_file->get_stats(p_abort);
}
void looper::decode_initialize(unsigned p_flags, abort_callback & p_abort)
{
	INT32 temp4B;
	m_file->read(&temp4B, 4,p_abort);//'RIFF'
	if (temp4B != 0x46464952) { return; }

	m_file->read(&temp4B, 4, p_abort);//FileSize - 8

	m_file->read(&temp4B, 4, p_abort);//'WAVE'
	if (temp4B != 0x45564157) {return ; }

	m_file->read(&temp4B, 4, p_abort);//'fmt '
	if (temp4B != 0x20746D66) { return ; }

	m_file->read(&temp4B, 4, p_abort);//size of head
	if (temp4B == 0x10)
	{
		m_file->read(&waveFormat, sizeof(WAVEFORMATEX) - 2, p_abort);
	}
	else if (temp4B == 0x12)
	{
		m_file->read(&waveFormat, sizeof(WAVEFORMATEX) , p_abort);
		m_file->seek_ex(waveFormat.cbSize,foobar2000_io::file::t_seek_mode::seek_from_current ,p_abort);//跳过其他信息
	}
	else
	{
		 return;
	}
	m_file->read(&temp4B, 4, p_abort);//'data'
	if (temp4B != 0x61746164) { return ; }
	m_file->read(&temp4B, 4, p_abort);
	fullCount = temp4B / waveFormat.nBlockAlign;
	pcmStart = m_file->get_position(p_abort);
	buffer = (char*)malloc(1024*waveFormat.nBlockAlign);
	currentIndex = 0;
}
bool looper::decode_run(audio_chunk & p_chunk, abort_callback & p_abort)
{
	int position = PlaybackPosition(currentIndex);
	if (position >= 0) 
	{
		currentIndex = position;
		m_file->seek(pcmStart +currentIndex*waveFormat.nBlockAlign, p_abort);
	}
	sampleIndex t = GetLoopEnd() - (currentIndex+1024);
	if (t >= 0) 
	{
		m_file->read(buffer, 1024 * waveFormat.nBlockAlign, p_abort);
		currentIndex += 1024;
	}
	else 
	{
		m_file->read(buffer, (1024+t) * waveFormat.nBlockAlign, p_abort);
		m_file->seek_ex(pcmStart + GetLoopStart()*waveFormat.nBlockAlign,m_file->seek_from_beginning,p_abort);
		m_file->read(buffer+(1024+t)* waveFormat.nBlockAlign, (-t) * waveFormat.nBlockAlign, p_abort);
		currentIndex = GetLoopStart() -t;
	}

	p_chunk.set_data_fixedpoint(buffer, 1024 * waveFormat.nBlockAlign,
		waveFormat.nSamplesPerSec,
		waveFormat.nChannels,
		waveFormat.wBitsPerSample,//你tm就写个bps鬼知道是byte per second还是bit per sample啊
		audio_chunk::g_guess_channel_config(waveFormat.nChannels)
	);

	return true;
}
bool looper::decode_run_raw(audio_chunk & p_chunk, mem_block_container & p_raw, abort_callback & p_abort)
{
	// TODO Implement method.
	throw pfc::exception_not_implemented();
}

void looper::decode_seek(double p_seconds, abort_callback & p_abort)
{
	m_file->ensure_seekable();//throw exceptions if someone called decode_seek() despite of our input having reported itself as nonseekable.
	t_uint64 sample = audio_math::time_to_samples(p_seconds, waveFormat.nSamplesPerSec);
	m_file->seek(sample, p_abort);
}

bool looper::decode_can_seek()
{
	return false;
}

bool looper::decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta)
{
	return false;
}

bool looper::decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta)
{
	return false;
}

void looper::decode_on_idle(abort_callback & p_abort)
{
	m_file->on_idle(p_abort);
}

void looper::retag(const file_info & p_info, abort_callback & p_abort)
{
}

bool looper::g_is_our_content_type(const char * p_content_type)
{
	return false;
}

bool looper::g_is_our_path(const char * p_path, const char * p_extension)
{
	return stricmp_utf8(p_extension, "OSTLooper") == 0;
}

void looper::set_logger(event_logger::ptr ptr)
{

}
static input_singletrack_factory_t<looper> g_mylooper;