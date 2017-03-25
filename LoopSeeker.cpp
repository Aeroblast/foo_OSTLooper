#include "stdafx.h"
#include "LoopSeeker.h"
#include<dsound.h>
//��һ���Ӻ����������������Щ����Ĳ�����ⲿ����(�������û�����fb2k)���������������ú��鷳������
//TODO�������foobar��utf8ϵͳ������

	WAVEFORMATEX waveFormat;
	FILE* waveFile = 0;
	INT32 fileSize;
	INT32 pcmSize;
	long pcmStart;
	char*pcmData = 0;
	bool inMemory = false;
	//real*fftw_input;
	//fftw_complex*fftw_output;
	//fftw_plan fftw_p;
	sampleIndex TimeStrToSampleIndex(char *str) 
	{
		int i=0;
		int lasti=-1;
		int min=0, sec=0;
		float ms=0;
		while (str[i])
		{
			if (str[i] == ':') 
			{
				//��⵽ð�ţ���ǰ������ָ�ɷ���
				if (lasti != -1)return -1;//����Ѿ���ð�ż�¼�򲻺Ϸ�
				int j = 1;
				while (i-j>=0)
				{
					int t = 1;
					for (int k = 1; k < j; k++)t *= 10;
					min += (str[i-j]-'0')*t;
					j++;
				}
				lasti = i;
			}
			if (str[i] == '.') 
			{
				//����е�Ӧ�ú�ð�Ÿ���������
				if (i - lasti != 3)return -1;
			//ǰ������
                 sec = ((str[i-1] - '0')+ (str[i - 2] - '0')*10);
				 //����
				 int j=1;
				 while (str[i+j])
				 {
					 float t = 100;
					 for (int k = 1; k < j; k++)t /= 10;
					 ms += (str[i + j] - '0')*t;
					 j++;
				 }
				 return TimeToSampleIndex(min,sec,ms);
			}
			if (str[i+1] == 0&&lasti&&i-lasti==2) 
			{
				sec = ((str[i] - '0') + (str[i - 1] - '0') * 10);
				return TimeToSampleIndex(min, sec, ms);
			}
			//���Ƿ��ַ�
			if (str[i]<'0'&&str[i]>'9'&&str[i]!='.'&&str[i]!=':')return -1;
			i++;
		}
		return  -1;
	}
	void SampleIndexToStr(sampleIndex i,char*str,size_t length) 
	{
		double sec=SampleIndexToSec(i);
		int min=0;
		while (sec>60)
		{
			sec -= 60;
			min++;
		}
		int r=sprintf_s(str,length,"%d:%6.3lf",min,sec);
		for (int i=0;i<(int)strlen(str);i++) 
		{
			if (str[i] == ' ')str[i] = '0';
		}
	}
	sampleIndex GetSampleNum() 
	{
		if (!waveFormat.nChannels)return -1;
		return pcmSize / waveFormat.nBlockAlign;	
	}
	INT64 GetSample(sampleIndex i)
	{
		if (i > pcmSize / waveFormat.nBlockAlign)return 0;
		if (inMemory) 
		{
			INT64 t = 0;

				memcpy(&t, &pcmData[waveFormat.wBitsPerSample / 8 * i ],waveFormat.wBitsPerSample/8);

			if (t&(     ((INT64)1)    << (waveFormat.wBitsPerSample - 1)  ))//��ֵ
			{
				t = t ^ (((INT64)1) << (waveFormat.wBitsPerSample - 1)); //ȥ������λ
				t ^= 0;
				t -= (((INT64)1) << (waveFormat.wBitsPerSample - 1));//������ż�Ȩ
			}
			return  t;
		}
		else 
		{
			if (!waveFile)return 0;
			INT64 t=0;
			fseek(waveFile, pcmStart + (i)*waveFormat.nBlockAlign, SEEK_SET);
			fread_s(&t, waveFormat.wBitsPerSample / 8, waveFormat.wBitsPerSample / 8, 1, waveFile);
			if (t&(((INT64)1) << (waveFormat.wBitsPerSample - 1)))//��ֵ
			{
				t = t ^ (((INT64)1) << (waveFormat.wBitsPerSample - 1)); //ȥ������λ
				t ^=0;
				t -= (((INT64)1) << (waveFormat.wBitsPerSample - 1));//������ż�Ȩ
			}
			return t;
			
		
		}
	}
	sampleIndex TimeToSampleIndex(int min, int sec, float msec)
	{
		if (!waveFormat.nChannels)return -1;
		if (sec < 60 && msec < 1000)
		{
			return (sampleIndex)((min * 60 + sec)*waveFormat.nSamplesPerSec + msec*waveFormat.nSamplesPerSec / 1000);
		}
		return -2;
	}
	double SampleIndexToSec(sampleIndex i) 
	{
		return i / (double)waveFormat.nSamplesPerSec;
	}
	FILE* OpenWave(CHAR*path)
	{
		if (waveFile)return waveFile;
		FILE*fp = 0;
		fopen_s(&fp, path, "rb");
		if (!fp)return 0;
		INT32 temp4B;
		fread_s(&temp4B, 4, 4, 1, fp);//'RIFF'
		if (temp4B != 0x46464952) { fclose(fp); return 0; }

		fread_s(&fileSize, 4, 4, 1, fp);//FileSize - 8

		fread_s(&temp4B, 4, 4, 1, fp);//'WAVE'
		if (temp4B != 0x45564157) { fclose(fp); return 0; }

		fread_s(&temp4B, 4, 4, 1, fp);//'fmt '
		if (temp4B != 0x20746D66) { fclose(fp); return 0; }

		fread_s(&temp4B, 4, 4, 1, fp);//size of head
		if (temp4B == 0x10)
		{
			fread_s(&waveFormat, sizeof(WAVEFORMATEX) - 2, sizeof(WAVEFORMATEX) - 2, 1, fp); 
		}
		else if (temp4B == 0x12)
		{
			fread_s(&waveFormat, sizeof(WAVEFORMATEX), sizeof(WAVEFORMATEX), 1, fp);
			fseek(fp, waveFormat.cbSize, SEEK_CUR);//����������Ϣ
		}
		else 
		{
			fclose(fp); return 0;
		}
		fread_s(&temp4B, 4, 4, 1, fp);//'data'
		if (temp4B != 0x61746164) { fclose(fp); return 0; }
		fread_s(&pcmSize, 4, 4, 1, fp);
		pcmStart = ftell(fp);
		waveFile = fp;
		return waveFile;
	}
    int CloseWave() 
	{
		int r=fclose(waveFile);
		waveFile = 0;
		return r;
	}



	/*
	int ReadWaveToMemory(CHAR*path)//�ȽϷ���
	{
		if (inMemory)return 1;
		OpenWave(path);
		sampleIndex maxI = pcmSize / waveFormat.nBlockAlign;
		pcmData = (char*)malloc(maxI*waveFormat.wBitsPerSample/8);
		if (pcmData == 0)return -1;
		for (sampleIndex i=0;i<maxI;i++) 
		{
			fseek(waveFile, pcmStart+i*waveFormat.nBlockAlign, SEEK_SET);
			fread(pcmData+i*waveFormat.wBitsPerSample/8, waveFormat.wBitsPerSample / 8,1,waveFile);
		}
		inMemory = true;
		CloseWave();
		return 0;
	}
	*/
	int ReleaseMemory()
	{
		if (pcmData)free(pcmData);
		pcmData = 0;
		inMemory = false;
		return 0;
	}
	//�ǵ�ʹ��ǰ��WAV��֮��ص�WAV,
	void DrawWave(HDC hdc,int x,int y,int width,int height,sampleIndex start,sampleIndex end,bool &abort) 
	{
		if (waveFormat.nChannels&&waveFile&&inMemory) 
		{
			//HDC hdcMem = CreateCompatibleDC(hdc);
			//HBITMAP hB = CreateCompatibleBitmap(hdc, width, height);
			//SelectObject(hdc,hB);
			int samplePerPixel=(end-start)/width;
			INT64 max_sample_value = (((INT64)1) << (waveFormat.wBitsPerSample-1));
			unsigned char colorDelta = 0xFF / samplePerPixel;
			if (colorDelta == 0)colorDelta =0x1;
			unsigned char* temp;
			temp = new unsigned char[height];
			for (int i = 0; i < width;i++) 
			{
				memset(temp,0,height);
				for (int j = 0; j < samplePerPixel; j++) 
				{
					long value = GetSample(start + i*samplePerPixel + j);
					int h = (int)(height  *( (GetSample(start + i*samplePerPixel + j) / (double)max_sample_value)/2));
					int d = 1;
					if (h < 0) 
					{
						d = -1; h = -h;
					}
					for (int k=0;k<h;k++) 
					{
						if(temp[k*d + height / 2]!=0xFF)temp[k*d + height / 2] += colorDelta;
						//COLORREF color=GetPixel(hdcMem,i,k*d+height/2);
						//SetPixel(hdcMem,i, k*d + height / 2,color+colorDelta);
					}
				}
				for (int j = 0; j < height; j++) 
				{
					SetPixel(hdc,i,j,temp[j]+(((long)temp[j])<<8)+ (((long)temp[j]) << 16));
					if (abort)
					{
						delete temp;
						abort = false;
						return;
					}
				}
			}

			delete temp;
			//BitBlt(hdc,x,y,width,height,hdcMem,0,0,SRCCOPY);
			//DeleteDC(hdcMem);
		}
	}
	void find_by_sample_process::on_init(HWND p_wnd) {}
	void find_by_sample_process::run(threaded_process_status & p_status, abort_callback & p_abort)
	{
		try {
			if (!waveFile)throw "no file opened.";
			p_status.set_progress(0, 100);
			INT64 max_sample_value = (((INT64)1) << (waveFormat.wBitsPerSample-1));
			int groupSize = waveFormat.nSamplesPerSec / 10;
			real *sample = new real[groupSize];
			for (int i = 0; i < groupSize; i++)
			{
				sample[i] = 0;
				for (int j = 0; j < 10; j++)
				{
					sample[i] += GetSample(j + i * 10 + seekBase) / (real)max_sample_value;
				}
				sample[i] /= 10;
			}
			sampleIndex index = seekStart;
			do {
				//putchar('\n'); PrintTime(index); printf("  %ld", index);
				for (int i = 0; i < groupSize; i++)
				{
					real averSample = 0;
					for (int j = 0; j < 10; j++)
					{
						averSample += GetSample(j + index + 10 * i) / (real)max_sample_value;
					}
					averSample /= 10;
					real c = averSample - sample[i];
					if (c < 0)c = -c;
					//c /= sample[i];//ʹ����Բ��Ч���������룬Ϊɶ��
					//	if(i>100)printf("  %5d  %lf\n",i,c);
					if (c > sensitivity) { break; }
					if (i == groupSize - 1) { goto P_F; }
				}
				index += 5;
				p_status.set_progress_float(1-(seekEnd - index)/(double)(seekEnd-seekStart));
				p_abort.check();
			} while (index<seekEnd);
			return;
		P_F://����ȷһЩ��������Ҫ��
			index -= 4;
			real minVal = 99; int min = 0;
			for (int k = 0; k < 9; k++)
			{
				real sum = 0;
				for (int i = 0; i < groupSize; i++)
				{
					real averSample = 0;
					for (int j = 0; j < 10; j++)
					{
						averSample += GetSample(j + index + 10 * i) / (real)max_sample_value;
					}
					averSample /= 10;
					real c = averSample - sample[i];
					if (c < 0)c = -c;
					if (c > sensitivity) { break; }
					sum += c;
					if (i == groupSize - 1) { if (sum < minVal) { minVal = sum; min = k; } }
				}
			}
			delete sample;
			*loopStart = index + min;
		}
		catch (std::exception &e)
		{

		}
	}
	void find_by_sample_process::on_done(HWND p_wnd, bool p_was_aborted) {
		if (!p_was_aborted) {

		}
		else
		{
		}
	}

	void read_wave_to_memory_process::on_init(HWND p_wnd) {}
	void read_wave_to_memory_process::run(threaded_process_status & p_status, abort_callback & p_abort)
	{
		try {
			if (inMemory)return;
			p_status.set_progress(0, 100);
			OpenWave(path);
			sampleIndex maxI = pcmSize / waveFormat.nBlockAlign;
			pcmData = (char*)malloc(maxI*waveFormat.wBitsPerSample / 8);
			if (pcmData == 0)throw  "No enough memory.";
			for (sampleIndex i = 0; i<maxI; i++)
			{
				fseek(waveFile, pcmStart + i*waveFormat.nBlockAlign, SEEK_SET);
				fread(pcmData + waveFormat.wBitsPerSample / 8*i, waveFormat.wBitsPerSample / 8, 1, waveFile);
				p_status.set_progress_float(i/(float)maxI);
				p_abort.check();
			}
			inMemory = true;
			CloseWave();
		}
		catch (std::exception &e)
		{

		}
	}
	void read_wave_to_memory_process::on_done(HWND p_wnd, bool p_was_aborted) {
		if (!p_was_aborted) {

		}
		else
		{
			CloseWave();
			if(pcmData)free(pcmData);
			pcmData = 0;
		}
	}

	//��ֲ������ǰ�Ĵ��룬�Ѿ�����ɶ�߼���
	void find_fade_out_process::on_init(HWND p_wnd) {}
	void find_fade_out_process::run(threaded_process_status & p_status, abort_callback & p_abort)
	{
		try {
			if (!waveFile)throw "no file opened.";
			p_status.set_progress(0, 100);
			INT64 max_sample_value = (((INT64)1) << (waveFormat.wBitsPerSample - 1));
			long mi = 0;//�����������
			int con = 0;
			INT64 t;
			INT64 last = 0;
			int count = 0;
			INT64 max_Y = 0; long max_X = 0;
			INT64 lastMax_Y = 0; long lastMax_X = 0;
			std::vector<INT64>ks;
			long searchEnd = waveFormat.nSamplesPerSec * 60;
			int searchFeq = waveFormat.nSamplesPerSec / 2;
			while (mi <= searchEnd)
			{
				t = GetSample(GetSampleNum()-mi-1);
				if (con == 0 && t >100) { con = 1; }
				if (con)
				{
					if (t < 0) { t = -t; }
					if (t > max_Y) { max_Y = t; max_X = mi; }
					count++;
					if (count == searchFeq)
					{
						count = 0;
						ks.push_back((max_Y - lastMax_Y));
						lastMax_X = max_X; lastMax_Y = max_Y;
						max_Y = 0;
						if (ks.size()>3 && ks[ks.size() - 1] < 10 && ks[ks.size() - 2]<10 && ks[ks.size() - 3]<10)
						{
							mi -= (2 * searchFeq);
							break;
						}
					}
				}
				mi++;
				if (mi == waveFormat.nSamplesPerSec * 60)
				{
					return;
				}
			}
			//�˴���ȷ��Fade-out���ִ��Կ�ʼλ��,����΢׼һ���
			int more_acc = 0;
			for (int i = 0; i<searchFeq; i++)
			{
				t = GetSample(GetSampleNum() - mi - 1+i);
				INT64 delta = t - max_Y;
				if (delta < 0)delta = -delta;
				if (delta < 10)more_acc = i;
			}
			mi -= more_acc;
			//�Դ�Ϊ��׼���бȶ�

			*result = pcmSize / waveFormat.nBlockAlign - mi + waveFormat.nSamplesPerSec;
		}
		catch (std::exception &e)
		{

		}
	}
	void find_fade_out_process::on_done(HWND p_wnd, bool p_was_aborted) {
		if (!p_was_aborted) {

		}
		else
		{
		}
	}

	DWORD WINAPI TestLoopPlayback(PVOID p)
	{
		TestLoopInfo *info = (TestLoopInfo*)p;
#define MAX_AUDIO_BUF 4   
#define BUFFERNOTIFYSIZE 44100   
		int i;
		if (!waveFile)return -1;
		FILE * fp = waveFile;
		fseek(fp, pcmStart, SEEK_SET);
		IDirectSound8 *m_pDS = NULL;
		IDirectSoundBuffer8 *m_pDSBuffer8 = NULL;
		IDirectSoundBuffer *m_pDSBuffer = NULL;
		IDirectSoundNotify8 *m_pDSNotify = NULL;
		DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
		HANDLE m_event[MAX_AUDIO_BUF];
		DWORD waitmsec = BUFFERNOTIFYSIZE * 1000 / waveFormat.nAvgBytesPerSec;
		if (FAILED(DirectSoundCreate8(NULL, &m_pDS, NULL)))
			return FALSE;
		if (FAILED(m_pDS->SetCooperativeLevel(info->hWnd, DSSCL_NORMAL)))
			return FALSE;


		DSBUFFERDESC dsbd;
		memset(&dsbd, 0, sizeof(dsbd));
		dsbd.dwSize = sizeof(dsbd);
		dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
		dsbd.dwBufferBytes = MAX_AUDIO_BUF*BUFFERNOTIFYSIZE;
		dsbd.lpwfxFormat = &waveFormat;

		if (FAILED(m_pDS->CreateSoundBuffer(&dsbd, &m_pDSBuffer, NULL))) {
			return FALSE;
		}
		if (FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8))) {
			return FALSE;
		}
		//Get IDirectSoundNotify8  
		if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify))) {
			return FALSE;
		}
		for (i = 0; i<MAX_AUDIO_BUF; i++) {
			m_pDSPosNotify[i].dwOffset = i*BUFFERNOTIFYSIZE;
			m_event[i] = ::CreateEvent(NULL, false, false, NULL);
			m_pDSPosNotify[i].hEventNotify = m_event[i];
		}
		m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF, m_pDSPosNotify);
		m_pDSNotify->Release();

		//Start Playing  
		BOOL isPlaying = TRUE;
		LPVOID buf = NULL;
		DWORD  buf_len = 0;
		DWORD res = WAIT_OBJECT_0;
		DWORD offset = BUFFERNOTIFYSIZE;
		int r=fseek(fp, pcmStart + info->loopEnd*waveFormat.nBlockAlign - (int)(info->sec*waveFormat.nAvgBytesPerSec), SEEK_SET);
		if (r < 0)return -1;
		m_pDSBuffer8->SetCurrentPosition(0);
		m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
		bool looped = false;
		WCHAR infoText[10] = {0};
		//Loop  
		while (isPlaying) {
			if ((res >= WAIT_OBJECT_0) && (res <= WAIT_OBJECT_0 + 3)) {
				m_pDSBuffer8->Lock(offset, BUFFERNOTIFYSIZE, &buf, &buf_len, NULL, NULL, 0);
				//if (BUFFERNOTIFYSIZE != buf_len) { MessageBox(0, 0, 0, 0); }
				if (looped) 
				{
					fread(buf, 1, buf_len, fp);
					long count = info->loopStart+(int)(info->sec*waveFormat.nAvgBytesPerSec) - (ftell(fp) + buf_len - pcmStart) / waveFormat.nBlockAlign;
					if (count < 0)break;
				}
				else
				{
					long loopCount = info->loopEnd - (ftell(fp) + buf_len - pcmStart) / waveFormat.nBlockAlign;
					if (loopCount < 0)
					{
						size_t written = fread(buf, 1, buf_len + loopCount*waveFormat.nBlockAlign, fp);
						fseek(fp, pcmStart + info->loopStart*waveFormat.nBlockAlign, SEEK_SET);
						written += fread(((char*)buf) + written, 1, -loopCount*waveFormat.nBlockAlign, fp);
						looped = true;
						wcscpy_s(infoText,L"����ת");
						SetWindowTextW(info->hWnd, infoText);
					}
					else
					{
						float sec = loopCount / (float)waveFormat.nSamplesPerSec;
						_stprintf_s(infoText,_T("%.1f"),sec);
						SetWindowTextW(info->hWnd, infoText);
						fread(buf, 1, buf_len, fp);
					}
				}

				m_pDSBuffer8->Unlock(buf, buf_len, NULL, 0);
				offset += buf_len;
				offset %= (BUFFERNOTIFYSIZE * MAX_AUDIO_BUF);
				//			printf("this is %7d of buffer\n", offset);
			}
			res = WaitForMultipleObjects(MAX_AUDIO_BUF, m_event, FALSE, waitmsec);
			if (WAIT_TIMEOUT == res)
			{
				//res=1;
			}
		}
		m_pDSBuffer8->Release();
		m_pDSBuffer->Release();
		m_pDS->Release();
		CloseWave();
		return 0;
	}
	/*�Ѿ����滻����foobar2000��process�汾
		sampleIndex FindBySample(sampleIndex seekStart, sampleIndex seekEnd, sampleIndex seekBase, float sensitivity)
	{
		if (!waveFile)return 0;
		INT64 max_sample_value = (1 << waveFormat.wBitsPerSample);
		int groupSize = waveFormat.nSamplesPerSec / 10;
		real *sample = new real[groupSize];
		for (int i = 0; i < groupSize; i++)
		{
			sample[i] = 0;
			for (int j = 0; j < 10; j++)
			{
				sample[i] += GetSample(j + i * 10 + seekBase) / (real)max_sample_value;
			}
			sample[i] /= 10;
		}
		long index = seekStart;
		do {
			//putchar('\n'); PrintTime(index); printf("  %ld", index);
			for (int i = 0; i < groupSize; i++)
			{
				real averSample = 0;
				for (int j = 0; j < 10; j++)
				{
					averSample += GetSample(j + index + 10 * i)/ (real)max_sample_value;
				}
				averSample /= 10;
				real c = averSample - sample[i];
				if (c < 0)c = -c;
				//	if(i>100)printf("  %5d  %lf\n",i,c);
				if (c > sensitivity) { break; }
				if (i == groupSize - 1) { goto P_F; }
			}
			index += 5;
		} while (index<seekEnd);
		return 0;
	P_F://����ȷһЩ��������Ҫ��
		index -= 4;
		real minVal = 99; int min = 0;
		for (int k = 0; k < 9; k++)
		{
			real sum = 0;
			for (int i = 0; i < groupSize; i++)
			{
				real averSample = 0;
				for (int j = 0; j < 10; j++)
				{
					averSample += GetSample(j + index + 10 * i) / (real)max_sample_value;
				}
				averSample /= 10;
				real c = averSample - sample[i];
				if (c < 0)c = -c;
				if (c > sensitivity) { break; }
				sum += c;
				if (i == groupSize - 1) { if (sum < minVal) { minVal = sum; min = k; } }
			}
		}
		delete sample;
		return index + min;
	}
	*/



