#pragma once
#include "stdafx.h"

	typedef double real;

	sampleIndex GetSampleNum();
	INT64 GetSample(sampleIndex i);
	sampleIndex TimeStrToSampleIndex(char *str);
	void SampleIndexToStr(sampleIndex i, char*str, size_t length);
	sampleIndex TimeToSampleIndex(int min, int sec, float msec);
	double SampleIndexToSec(sampleIndex i);
	int OpenWave(CHAR*path);
	int CloseWave();
	void DrawWave(HDC hdc, int x, int y, int width, int height, sampleIndex start, sampleIndex end,bool &abort);
	//int ReadWaveToMemory(CHAR*path);
	int ReleaseMemory();
	class find_by_sample_process : public threaded_process_callback 
	{
	public:
		find_by_sample_process(sampleIndex*_loopStart, sampleIndex _seekStart, sampleIndex _seekEnd, sampleIndex _seekBase, float _sensitivity)
			:loopStart( _loopStart), seekStart(_seekStart), seekEnd(_seekEnd), seekBase(_seekBase), sensitivity(_sensitivity){}
		void on_init(HWND p_wnd);
		void run(threaded_process_status & p_status, abort_callback & p_abort);
		void on_done(HWND p_wnd, bool p_was_aborted);
	private:
		sampleIndex seekStart, seekEnd, seekBase;
		sampleIndex *loopStart;
		float sensitivity;
	};

	class find_fade_out_process : public threaded_process_callback
	{
	public:
		find_fade_out_process(sampleIndex*_result)
			:result(_result) {}
		void on_init(HWND p_wnd);
		void run(threaded_process_status & p_status, abort_callback & p_abort);
		void on_done(HWND p_wnd, bool p_was_aborted);
	private:
		sampleIndex *result;
		float sensitivity;
	};

	class read_wave_to_memory_process : public threaded_process_callback
	{
	public:
		read_wave_to_memory_process(char* _path):path(_path){}
		void on_init(HWND p_wnd);
		void run(threaded_process_status & p_status, abort_callback & p_abort);
		void on_done(HWND p_wnd, bool p_was_aborted);
	private:
		char* path;
	};
	//sampleIndex FindBySample(sampleIndex seekStart, sampleIndex seekEnd, sampleIndex seekBase, float sensitivity = 0.2);//�Ѿ����滻����foobar2000��process�汾
   #define N_SAMPLE 4096
