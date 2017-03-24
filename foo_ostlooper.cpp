// foo_ostlooper.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "LoopSeeker.h"
#include "looper.h"
#include "resource.h"

DECLARE_COMPONENT_VERSION(
"OST Looper",
"0.0.1",
"OST Looper\n"
);
VALIDATE_COMPONENT_FILENAME("foo_ostlooper.dll");
//全局变量
bool looping = false;
HINSTANCE hInst;
HWND hLooperDlg;
HWND hLooperPlayDlg;
char tempFilePath[MAX_PATH+1];
char historyFilePath[MAX_PATH + 1];
size_t playlistID;
sampleIndex playbackPosition;
bool seeked = false;
metadb_handle_ptr originItem;
//对应控件
sampleIndex e_1_start=0,e_1_select=0,e_1_end=0,e_2_start=0,e_2_select=0,e_2_end=0,e_f_start=0,e_f_end=0;
//前向声明
INT_PTR CALLBACK LooperDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK LooperPlayDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
DWORD WINAPI DrawWaveThread(PVOID);
void ConvertToWav(metadb_handle_ptr &item);
metadb_handle_ptr GetOriginItem() 
{
	return originItem;
}
sampleIndex GetLoopStart() 
{
	return e_f_start;
}
sampleIndex GetLoopEnd() 
{
	return e_f_end;
}
sampleIndex PlaybackPosition(sampleIndex setValue) 
{
	if (seeked) 
	{
		seeked = false;
		return playbackPosition;
	}
	else 
	{
		playbackPosition = setValue;
		return -1;
	}
}
//From MSDN
// Description:
//   Creates a tooltip for an item in a dialog box. 
// Parameters:
//   idTool - identifier of an dialog box item.
//   nDlg - window handle of the dialog box.
//   pszText - string to use as the tooltip text.
// Returns:
//   The handle to the tooltip.
//
HWND CreateToolTip(int toolID, HWND hDlg, PTSTR pszText)
{
	if (!toolID || !hDlg || !pszText)
	{
		return FALSE;
	}
	// Get the window of the tool.
	HWND hwndTool = GetDlgItem(hDlg, toolID);

	// Create the tooltip. g_hInst is the global instance handle.
	HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL,
	    hInst, NULL);

	if (!hwndTool || !hwndTip)
	{
		return (HWND)NULL;
	}

	// Associate the tooltip with the tool.
	TOOLINFO toolInfo = { 0 };
	toolInfo.cbSize = sizeof(toolInfo);
	toolInfo.hwnd = hDlg;
	toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	toolInfo.uId = (UINT_PTR)hwndTool;
	toolInfo.lpszText = pszText;
	SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

	return hwndTip;
}

void HistoryUpdate(const char* name) 
{
	FILE*logFile = 0;
	if (_access(historyFilePath, 06) != -1) 
	{
		fopen_s(&logFile, historyFilePath, "ab");
	}
	else 
	{
		fopen_s(&logFile,historyFilePath,"wb");
		unsigned char utf8head[3] = {0xEF,0xBB,0xBF};
		fwrite(utf8head,1,3,logFile);
	}

	if (logFile)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		fseek(logFile, 0, SEEK_END);
		fprintf(logFile, "%d-%2d-%2d %2d:%2d:%2d,%ld,%ld,\"", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
			e_f_start ,e_f_end);
		size_t size=0;
		while (name[size]) 
		{
			size++;
		}
		fwrite(name,1,size,logFile);
		fprintf(logFile, "\"\r\n");
		fclose(logFile);
	}
	else
	{
		MessageBox(0, _T("不知道为啥不能写Log，请注意手动保存结果"), _T("警告"), 0);
	}
}
bool HistoryCheck(const char*name,sampleIndex&start,sampleIndex&end) 
{
	FILE*logFile = 0;
	fopen_s(&logFile, historyFilePath, "rb");
	if (!logFile)return false;
	fseek(logFile,0,SEEK_END);
	size_t length = ftell(logFile);
	char *txt = (char*)malloc(length);
	fseek(logFile,0,SEEK_SET);
	fread(txt,1,length,logFile);
	fclose(logFile);
	if (txt[0]== (char)0xEF && txt[1] == (char)0xBB && txt[2] == (char)0xBF)
	{
		size_t lineStart=3;
		for (size_t i = 3;i<length;i++)
		{
			if (txt[i] == '\n') 
			{
				//日期时间应该19个字符 
				//XXXX-XX-XX XX:XX:XX,[loopstart],[loopend],"name"
				if (txt[lineStart + 19] == ',') 
				{
					size_t j = i - 3;
					for (;j>lineStart+20;j--) 					
					{
						if (txt[j] == '\"') 
						{
							break;
						}
					}
					int delta = 0;
					while (name[delta] && txt[j + 1 + delta] != '\"'&&name[delta] == txt[j + 1 + delta])
					{
						if (name[delta + 1] == 0 && txt[j + 2 + delta] == '\"')
						{
							//名字一样，开始搞end
							sampleIndex temp=0;
							long tens=1;
							while (txt[j] != ',')j--;
							j--;
							while (txt[j] != ',') 
							{
								if (txt[j]<'0' || txt[j]>'9') { temp = -1; break; }
								temp += (txt[j] - '0')*tens;
								j--; 
								tens *= 10;
							}
							if (temp >= 0) { end = temp; }
							//一样搞一遍start
							temp = 0;
							tens = 1;
							while (txt[j] != ',')j--;
							j--;
							while (txt[j] != ',')
							{
								if (txt[j]<'0' || txt[j]>'9') { temp = -1; break; }
								temp += (txt[j] - '0')*tens;
								j--;
								tens *= 10;
							}
							if (temp >= 0) { start = temp; return true; }
						}
						delta++;
					}
				}
				lineStart = i + 1;
			}
		}
	
	}
	return false;
}

class LoopEntry : public contextmenu_item_simple {
public:
	enum {
		cmd_test1 = 0,
	};
	//GUID get_parent() { return guid_mygroup; }
	unsigned get_num_items() { return 1; }
	void get_item_name(unsigned p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_test1: p_out = "Try to Loop"; break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller) {
		switch (p_index) {
		case cmd_test1:
		{
			if (hLooperDlg)return;
			originItem = p_data.get_item(0);	
			
			GetModuleFileNameA(0, tempFilePath, MAX_PATH);
			for (int i = strlen(tempFilePath); i > 0; i--)if (tempFilePath[i] == '\\') { tempFilePath[i + 1] = 0; break; }
			strcpy_s(historyFilePath,tempFilePath);
			strcat_s(tempFilePath, "OSTLooperTemp.OSTLooper");
			strcat_s(historyFilePath, "OSTLooper-history");
			
			hInst = GetModuleHandle(L"foo_ostlooper.dll");
			hLooperDlg= CreateDialog(hInst, MAKEINTRESOURCE(IDD_LOOPER),
				core_api::get_main_window(), LooperDialog);
			ShowWindow(hLooperDlg, SW_SHOW);
			looping = false;
		}
		break;
		default:
			uBugCheck();
		}
	}
	// Overriding this is not mandatory. We're overriding it just to demonstrate stuff that you can do such as context-sensitive menu item labels.
	bool context_get_display(unsigned p_index, metadb_handle_list_cref p_data, pfc::string_base & p_out, unsigned & p_displayflags, const GUID & p_caller) {
		switch (p_index) {
		case cmd_test1:
			if (!__super::context_get_display(p_index, p_data, p_out, p_displayflags, p_caller)) return false;
			// Example context sensitive label: append the count of selected items to the label.
		//	p_out << " : " << p_data.get_count() << " item";
			//if (p_data.get_count() != 1) p_out << "s";
			//p_out << " selected";
			if (p_data.get_count() != 1) { return false; }
			return true;
		default: uBugCheck();
		}
	}
	GUID get_item_guid(unsigned p_index) {
		// These GUIDs identify our context menu items. Substitute with your own GUIDs when reusing code.
		static const GUID ostlooperGuid =
		{ 0xa31fe848, 0x7dc4, 0x4006,{ 0x92, 0x9d, 0xf9, 0xb4, 0x94, 0x76, 0x2d, 0x5f } };

		switch (p_index) {
		case cmd_test1: return ostlooperGuid;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}

	}
	bool get_item_description(unsigned p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_test1:
			p_out = "Open OST Looper Dialog.";
			return true;
		default:
			uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};
static contextmenu_item_factory_t<LoopEntry> g_myitem_factory;

class Looper_initquit : public initquit {
public:
	void on_init() {
	}
	void on_quit() {
		static_api_ptr_t<playlist_manager>()->remove_playlist(playlistID);
		uDeleteFile(tempFilePath);
	}
};

static initquit_factory_t<Looper_initquit> looper_initquit_factory;

class convert_wav_process : public threaded_process_callback {
public:
	convert_wav_process(metadb_handle_ptr item) : m_item(item){}
	void on_init(HWND p_wnd) {}
	void run(threaded_process_status & p_status, abort_callback & p_abort) {
		try {
			const t_uint32 decode_flags = input_flag_no_seeking | input_flag_no_looping; // tell the decoders that we won't seek and that we don't want looping on formats that support looping.
			input_helper input;
				p_abort.check(); // in case the input we're working with fails at doing this
				p_status.set_progress(0, 100);
				p_status.set_progress_secondary(0);
				p_status.set_item_path(m_item->get_path());
				input.open(NULL,m_item, decode_flags, p_abort);

				double length;
				    // fetch the track length for proper dual progress display;
					file_info_impl info;
					// input.open should have preloaded relevant info, no need to query the input itself again.
					// Regular get_info() may not retrieve freshly loaded info yet at this point (it will start giving the new info when relevant info change callbacks are dispatched); we need to use get_info_async.
					if (m_item->get_info_async(info)) length = info.get_length();
					else length = 0;
				audio_chunk_impl_temporary l_chunk;
				double decoded = 0;
				while (input.run(l_chunk, p_abort)) { // main decode loop
					m_peak = l_chunk.get_peak(m_peak);
					if (!wavW.is_open()) 
					{
						wavWriterSetup_t wavSetup;
						wavSetup.initialize(l_chunk,info.info_get_decoded_bps(),0,false);
						wavW.open(tempFilePath, wavSetup, p_abort);
					}
					else 
					{
						wavW.write(l_chunk,p_abort);
					}
					if (length > 0) { // don't bother for unknown length tracks
						decoded += l_chunk.get_duration();
						if (decoded > length) decoded = length;
						p_status.set_progress_secondary_float(decoded / length);
					}
					p_abort.check(); // in case the input we're working with fails at doing this
				}
				wavW.finalize(p_abort);
				wavW.close();
		}
		catch (std::exception const & e) {
			m_failMsg = e.what();
		}
	}
	void on_done(HWND p_wnd, bool p_was_aborted) {
		if (!p_was_aborted) {
			if (!m_failMsg.is_empty()) {
				popup_message::g_complain("Failed to convert to  temp:", m_failMsg);
			}
			else {
				playlistID = static_api_ptr_t<playlist_manager>()->create_playlist("OST Looper", INFINITE, INFINITE);
			    metadb_handle_ptr  convertedWav=static_api_ptr_t<metadb>()->handle_create(tempFilePath, 0);
				//static_api_ptr_t<playlist_manager>()->set_active_playlist();
				static_api_ptr_t<playlist_manager>()->set_playing_playlist(playlistID);
				static_api_ptr_t<playlist_manager>()->playlist_set_focus_item(playlistID,1);
				metadb_handle_list list;
				list.add_item(convertedWav);
				static_api_ptr_t<playlist_manager>()->playlist_add_items(playlistID,list,bit_array_true());
				SendMessage(hLooperDlg,WM_COMMAND,MAKELONG(WAV_CONVERTED,0),0);
			}
		}
		else 
		{
			wavW.close();
		}
	}
private:
	audio_sample m_peak;
	pfc::string8 m_failMsg;
	const metadb_handle_ptr m_item;
	CWavWriter wavW;
};
void ConvertToWav(metadb_handle_ptr &data) {
	try {
		service_ptr_t<threaded_process_callback> cb = new service_impl_t<convert_wav_process>(data);
		static_api_ptr_t<threaded_process>()->run_modeless(
			cb,
            threaded_process::flag_show_item | threaded_process::flag_show_abort,
			core_api::get_main_window(),
			"Convert To WAV");
	}
	catch (std::exception const & e) {
		popup_message::g_complain("Could not start process", e);
	}
}

//用于调戏进度条的临时变量,WM_HSCROLL设置，WM_LBUTTONUP检查
LONG sliderValue;
bool sliderChanged = false;
bool sampleIndexDisplay = false;
void SetTimeEdit(HWND hDlg, sampleIndex i, int ID)
{
		HWND hEdit = GetDlgItem(hDlg, ID);
		char temp[11];
		if (sampleIndexDisplay) 
		{
			_ltoa_s(i,temp,10);
		}
		else 
		{
			SampleIndexToStr(i, temp, 10);
		}
		SetWindowTextA(hEdit, temp);
}

bool enableLD = false;//初始化完毕后设置为true以允许用户操作
bool waveDisplay = false;

struct DrawWaveTemp 
{
	HDC hTempDC= 0;
	HBITMAP hBitmap= 0;
	int x;
	int y;
	int width;
	int height;
	bool drawing;
	bool needDraw;
	HANDLE proc;
	bool abort;
	sampleIndex start=0, end=0;
}drawWaveTemp[3];
bool writeToHistory;
void OnEndDialog() 
{
	static_api_ptr_t<playback_control>()->stop();
	static_api_ptr_t<playlist_manager>()->remove_playlist(playlistID);
	ReleaseMemory();
	for (int i = 0; i<3; i++)
	{
		ReleaseDC(hLooperDlg, drawWaveTemp[i].hTempDC);
		DeleteObject(drawWaveTemp[i].hBitmap);
	}
	hLooperDlg = 0;
	hLooperPlayDlg = 0;
	uDeleteFile(tempFilePath);
	looping = false;
}
INT_PTR CALLBACK LooperDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (!looping)
		{
			ConvertToWav(originItem); //完成后发wm-cmd带wav_converted
		}
		else 
		{
		}
		return (INT_PTR)TRUE;
	}
	break;
	case WM_PAINT: 
	{
		if (waveDisplay)
		{

			PAINTSTRUCT ps;
			HDC hdc=BeginPaint(hDlg,&ps);
			for (int i = 0; i < 3; i++) 
			{
				if (drawWaveTemp[i].hTempDC == 0)
				{
					RECT rectC;
					RECT rectD;
					//rescoruce.h中：#define IDC_WAVE_DISPLAY_1 1001然后#define IDC_WAVE_DISPLAY_2 1002然后#define IDC_WAVE_FULL  1000
					HWND picCtrl = GetDlgItem(hLooperDlg,1000+i);
					GetClientRect(picCtrl, &rectC);
					GetClientRect(hLooperDlg, &rectD);
					ClientToScreen(hLooperDlg, (POINT*)(&rectD));
					ScreenToClient(picCtrl, (POINT*)(&rectD));
					drawWaveTemp[i].x = rectC.left - rectD.left;
					drawWaveTemp[i].y = rectC.top - rectD.top;
					drawWaveTemp[i].width = rectC.right - rectC.left;
					drawWaveTemp[i].height = rectC.bottom - rectC.top;
					drawWaveTemp[i].hTempDC = CreateCompatibleDC(hdc);
					drawWaveTemp[i].hBitmap = CreateCompatibleBitmap(hdc, rectC.right - rectC.left,
						rectC.bottom - rectC.top);
					SelectObject(drawWaveTemp[i].hTempDC, drawWaveTemp[i].hBitmap);
					drawWaveTemp[i].drawing = false;
					drawWaveTemp[i].needDraw = true;
				}
				if (drawWaveTemp[i].needDraw)
				{
					if (drawWaveTemp[i].drawing)
					{
						drawWaveTemp[i].abort = true;
					}
					else
					{
						drawWaveTemp[i].proc = CreateThread(0, 0, DrawWaveThread, (LPVOID)i, 0, 0);
					}
				}
				else
				{
					if (!drawWaveTemp[i].drawing)
					{
						sampleIndex start, end;
						switch (i)
						{
						case 0:
							start = 0;
							end = GetSampleNum();
							break;
						case 1:
							start = e_1_start;
							end = e_1_end;
							break;
						case 2:
							start = e_2_start;
							end = e_2_end;
							break;
						}
						StretchBlt(hdc,
							drawWaveTemp[i].x,
							drawWaveTemp[i].y,
							drawWaveTemp[i].width,
							drawWaveTemp[i].height,
							drawWaveTemp[i].hTempDC, 
							(int)(drawWaveTemp[i].width*((float)(drawWaveTemp[i].start - start))/(drawWaveTemp[i].end- drawWaveTemp[i].start)),
							0,
							(int)(drawWaveTemp[i].width*((float)(end-start)) / (drawWaveTemp[i].end - drawWaveTemp[i].start)),
							drawWaveTemp[i].height,
							SRCCOPY);
						/*
												BitBlt(hdc,
							drawWaveTemp[i].x,
							drawWaveTemp[i].y,
							drawWaveTemp[i].width,
							drawWaveTemp[i].height,
							drawWaveTemp[i].hTempDC,
							0, 0, SRCCOPY);
						*/
					}
				}


			}
			HPEN hPen = CreatePen(PS_SOLID, 1,0);
			SelectObject(hdc,hPen);
			MoveToEx(hdc,
				drawWaveTemp[1].x,
				drawWaveTemp[1].y,
				NULL);
			LineTo(hdc,
				drawWaveTemp[0].x+(int)(  drawWaveTemp[0].width*(e_1_start/  (float)GetSampleNum()  )  ),
				drawWaveTemp[0].y+ drawWaveTemp[0].height);
			MoveToEx(hdc,
				drawWaveTemp[1].x+drawWaveTemp[1].width,
				drawWaveTemp[1].y,
				NULL);
			LineTo(hdc,
				drawWaveTemp[0].x + (int)(drawWaveTemp[0].width*(e_1_end / (float)GetSampleNum())),
				drawWaveTemp[0].y + drawWaveTemp[0].height);
			MoveToEx(hdc,
				drawWaveTemp[2].x,
				drawWaveTemp[2].y,
				NULL);
			LineTo(hdc,
				drawWaveTemp[0].x + (int)(drawWaveTemp[0].width*(e_2_start / (float)GetSampleNum())),
				drawWaveTemp[0].y + drawWaveTemp[0].height);
			MoveToEx(hdc,
				drawWaveTemp[2].x + drawWaveTemp[2].width,
				drawWaveTemp[2].y,
				NULL);
			LineTo(hdc,
				drawWaveTemp[0].x + (int)(drawWaveTemp[0].width*(e_2_end / (float)GetSampleNum())),
				drawWaveTemp[0].y + drawWaveTemp[0].height);
			HPEN hPenRed = CreatePen(PS_SOLID, 1, RGB(0xFF,0,0));
			SelectObject(hdc, hPenRed);
			int delta = (int)(drawWaveTemp[1].width*((e_1_select - e_1_start) / (float)(e_1_end - e_1_start)));
			MoveToEx(hdc,
				drawWaveTemp[1].x +delta,
				drawWaveTemp[1].y,
				NULL);
			LineTo(hdc,
				drawWaveTemp[1].x +delta,
				drawWaveTemp[1].y+drawWaveTemp[1].height);
			delta = (int)(drawWaveTemp[2].width*((e_2_select - e_2_start) / (float)(e_2_end - e_2_start)));
			MoveToEx(hdc,
				drawWaveTemp[2].x + delta,
				drawWaveTemp[2].y,
				NULL);
			LineTo(hdc,
				drawWaveTemp[2].x + delta,
				drawWaveTemp[2].y + drawWaveTemp[2].height);
			EndPaint(hDlg,&ps);
			DeleteObject(hPen);
			DeleteObject(hPenRed);
		}
	}
	break;
	case WM_COMMAND:
		if (LOWORD(wParam) == WAV_CONVERTED)//最优先处理
		{
			if (OpenWave(tempFilePath) >= 0)
			{
				//读完头部数据就关了
				CloseWave();
				sampleIndex max = GetSampleNum();
				e_1_end = max / 2;
				e_1_start = max / 4;
				e_1_select = (e_1_end+e_1_start) / 2;
				e_2_end = max;
				e_2_start = max / 4*3;
				e_2_select = (e_2_end + e_2_start) / 2;
				const char * name = "title";
				file_info_impl info;
				if (originItem->get_info(info)) {
					const t_size meta_index = info.meta_find(name);
					if (meta_index != pfc::infinite_size) {
						const t_size value_count = info.meta_enum_value_count(meta_index);
						for (t_size value_index = 0; value_index < value_count; ++value_index) {
							const char * value = info.meta_enum_value(meta_index, value_index);
							if (!HistoryCheck(value, e_f_start, e_f_end)) 
							{ e_f_start = 0; e_f_end = 0; writeToHistory = true; }
							else 
							{
								writeToHistory = false;						
							}
						}
					}
				}
				SetTimeEdit(hDlg, e_1_end, IDC_E_TIME_1_END);
				SetTimeEdit(hDlg, e_1_start, IDC_E_TIME_1_START);
				SetTimeEdit(hDlg, e_1_select, IDC_E_TIME_1_SELECT);
				SetTimeEdit(hDlg, e_2_end, IDC_E_TIME_2_END);
				SetTimeEdit(hDlg, e_2_start, IDC_E_TIME_2_START);
				SetTimeEdit(hDlg, e_2_select, IDC_E_TIME_2_SELECT);
				SetTimeEdit(hDlg, e_f_end, IDC_E_TIME_F_END);
				SetTimeEdit(hDlg, e_f_start, IDC_E_TIME_F_START);
				CreateToolTip(IDC_E_TIME_1_END,hDlg,L"结束时间1");
				CreateToolTip(IDC_E_TIME_2_END, hDlg, L"结束时间2");
				CreateToolTip(IDC_E_TIME_1_START, hDlg, L"开始时间1");
				CreateToolTip(IDC_E_TIME_2_START, hDlg, L"开始时间2");
				CreateToolTip(IDC_E_TIME_1_SELECT, hDlg, L"选定时间1");
				CreateToolTip(IDC_E_TIME_2_SELECT, hDlg, L"选定时间2");
				SendDlgItemMessage(hDlg, IDC_C_USE_WAVE_DISPLAY, BM_SETCHECK, waveDisplay, 0);
				SendDlgItemMessage(hDlg, IDC_C_USE_SAMPLE_INDEX_DISPLAY, BM_SETCHECK, sampleIndexDisplay, 0);
				enableLD = true;
				return true;
			}
			else
			{
				//what居然打不开那就是出错了

			}
		}
		if (LOWORD(wParam) == IDOK)
		{
			OnEndDialog();
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (!enableLD)return 0;
		if (HIWORD(wParam) == EN_KILLFOCUS)
		{
			sampleIndex*pSI = 0;
			char dwtIndex=0;
			switch (LOWORD(wParam))
			{
			case IDC_E_TIME_1_END:pSI = &e_1_end; dwtIndex = 1; break;
			case IDC_E_TIME_1_SELECT:pSI = &e_1_select; dwtIndex = 1; break;
			case IDC_E_TIME_1_START:pSI = &e_1_start; dwtIndex = 1; break;
			case IDC_E_TIME_2_END:pSI = &e_2_end; dwtIndex = 1; break;
			case IDC_E_TIME_2_SELECT:pSI = &e_2_select; dwtIndex = 2; break;
			case IDC_E_TIME_2_START:pSI = &e_2_start; dwtIndex = 2; break;
			case IDC_E_TIME_F_END:pSI = &e_f_end; dwtIndex = -1; break;
			case IDC_E_TIME_F_START:pSI = &e_f_start; dwtIndex = -1; break;
			default: return 0;
			}
			char tempStr[21];
			GetWindowTextA((HWND)lParam, tempStr, 20);
			sampleIndex tempSI=-1;
			if(!sampleIndexDisplay)tempSI=TimeStrToSampleIndex(tempStr);
			else tempSI = atol(tempStr);			
			if (tempSI >= 0) *pSI = tempSI;
			SetTimeEdit(hDlg,*pSI,LOWORD(wParam));
			if (dwtIndex > 0) 
			{
				drawWaveTemp[dwtIndex].needDraw = true;
				InvalidateRect(hDlg,0,1);
				UpdateWindow(hDlg);
			}
			return true;
		}
		
		if (LOWORD(wParam) == IDC_B_SEEK_AUTO) //V1
		{
			if (OpenWave(tempFilePath) > 0) 
			{
				sampleIndex num = GetSampleNum();
				sampleIndex temp = e_1_select;
				service_ptr_t<threaded_process_callback> cb = new service_impl_t<find_by_sample_process>(&e_1_select,num/4,num/2,num/4*3,0.2);
				static_api_ptr_t<threaded_process>()->run_modal(
					cb,
					 threaded_process::flag_show_item | threaded_process::flag_show_abort,
					core_api::get_main_window(),
					"Searching Loop Part by Default Set");
				if (e_1_select == temp) { popup_message::g_complain("Failed to find by default set."); }
				else
				{
					SetTimeEdit(hDlg, e_1_select, IDC_E_TIME_1_SELECT);
					e_2_select = num / 4 * 3; SetTimeEdit(hDlg, e_2_select, IDC_E_TIME_2_SELECT);
				}
				cb.release();
			}
			CloseWave();
			return true;
		}


		if (LOWORD(wParam) == IDC_B_SEEK_FADE_OUT)//V2
		{
			if (OpenWave(tempFilePath) > 0)
			{
				sampleIndex temp =0;
				service_ptr_t<threaded_process_callback> cb = new service_impl_t<find_fade_out_process>(&temp);
				static_api_ptr_t<threaded_process>()->run_modal(
					cb,
					threaded_process::flag_show_item | threaded_process::flag_show_abort,
					core_api::get_main_window(),
					"Searching Fade-out Part");
				cb.release();
				e_2_select = temp;
				SetTimeEdit(hDlg, e_2_select, IDC_E_TIME_2_SELECT);
			}
			CloseWave();
			return true;
		}
		if (LOWORD(wParam) == IDC_B_SEEK)
		{

			if (OpenWave(tempFilePath) > 0)
			{
				sampleIndex num = GetSampleNum();
				sampleIndex temp = e_1_select;
				service_ptr_t<threaded_process_callback> cb = new service_impl_t<find_by_sample_process>(&e_1_select,e_1_start, e_1_end, e_2_select, 0.2);
				static_api_ptr_t<threaded_process>()->run_modal(
					cb,
					threaded_process::flag_show_progress_dual | threaded_process::flag_show_item | threaded_process::flag_show_abort,
					core_api::get_main_window(),
					"Searching Loop Part by Setting Time");
				if (e_1_select == temp) { popup_message::g_complain("Failed to find by set time."); }
				else
				{
					SetTimeEdit(hDlg, e_1_select, IDC_E_TIME_1_SELECT);
				}
			}
			CloseWave();
			return true;
		}
		if (LOWORD(wParam) == IDC_B_USE_SELECTED) 
		{
			e_f_start = e_1_select;
			e_f_end = e_2_select;
			SetTimeEdit(hDlg,e_f_start,IDC_E_TIME_F_START);
			SetTimeEdit(hDlg, e_f_end, IDC_E_TIME_F_END);
			return true;
		}
		if (LOWORD(wParam) == IDC_B_PLAY)
		{
			//大概应该在这里检查一下循环开始结束的合法性
			if (e_f_end - e_f_start < 1024) 
			{
				popup_message::g_complain("Loop start and end not valid");
				return true; 
			}
			if (writeToHistory) 
			{
				const char * name = "title";
				file_info_impl info;
				if (originItem->get_info(info))
				{
					const t_size meta_index = info.meta_find(name);
					if (meta_index != pfc::infinite_size)
					{
						const t_size value_count = info.meta_enum_value_count(meta_index);
						for (t_size value_index = 0; value_index < value_count; ++value_index) 
						{
							const char * value = info.meta_enum_value(meta_index, value_index);
							HistoryUpdate(value);
						}
					}
				}
			}


			looping = true;
			hLooperPlayDlg=CreateDialog(hInst, MAKEINTRESOURCE(IDD_LOOPER_PLAYING),
				hDlg, LooperPlayDialog);

			ShowWindow(hLooperPlayDlg, SW_SHOW);
			EndDialog(hDlg, LOWORD(wParam));
		}
		if (LOWORD(wParam) == IDC_C_USE_WAVE_DISPLAY&&HIWORD(wParam) == BN_CLICKED)
		{
			waveDisplay= (bool)SendDlgItemMessage(hDlg,IDC_C_USE_WAVE_DISPLAY, BM_GETCHECK, 0, 0);
			if (waveDisplay)
			{
				service_ptr_t<threaded_process_callback> cb = new service_impl_t<read_wave_to_memory_process>((char*)tempFilePath);
				static_api_ptr_t<threaded_process>()->run_modal(
					cb,
					threaded_process::flag_show_item | threaded_process::flag_show_abort,
					core_api::get_main_window(),
					"Load to memory");
			}
			else
			{
				ReleaseMemory();
				for (int i=0;i<3;i++) 
				{
					ReleaseDC(hDlg,drawWaveTemp[i].hTempDC);
					DeleteObject(drawWaveTemp[i].hBitmap);
				}
			}
			InvalidateRect(hDlg,0,0);
			UpdateWindow(hDlg);
			return  true;
		}
		if (LOWORD(wParam) == IDC_C_USE_SAMPLE_INDEX_DISPLAY&&HIWORD(wParam) == BN_CLICKED)
		{
			sampleIndexDisplay = (bool)SendDlgItemMessage(hDlg, IDC_C_USE_SAMPLE_INDEX_DISPLAY, BM_GETCHECK, 0, 0);
			SetTimeEdit(hDlg, e_1_end, IDC_E_TIME_1_END);
			SetTimeEdit(hDlg, e_1_start, IDC_E_TIME_1_START);
			SetTimeEdit(hDlg, e_1_select, IDC_E_TIME_1_SELECT);
			SetTimeEdit(hDlg, e_2_end, IDC_E_TIME_2_END);
			SetTimeEdit(hDlg, e_2_start, IDC_E_TIME_2_START);
			SetTimeEdit(hDlg, e_2_select, IDC_E_TIME_2_SELECT);
			SetTimeEdit(hDlg, e_f_end, IDC_E_TIME_F_END);
			SetTimeEdit(hDlg, e_f_start, IDC_E_TIME_F_START);
			return  true;
		}
	}
	return (INT_PTR)FALSE;
}
INT_PTR CALLBACK LooperPlayDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		if (!looping)
		{
		}
		else
		{
			static_api_ptr_t<playback_control>()->stop();
			static_api_ptr_t<playback_control>()->play_start();
			HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER); 
			SendMessageW(hSlider, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, 1000));
			CHAR tempStr[100]="Loop Part:";
			CHAR tempstr[10];
			SampleIndexToStr(e_f_start,tempstr,9);
			strcat_s(tempStr,tempstr);
			strcat_s(tempStr," ~ ");
			SampleIndexToStr(e_f_end, tempstr, 9);
			strcat_s(tempStr, tempstr);
			SetDlgItemTextA(hDlg,IDC_T_INFO,tempStr);
 			SetTimer(hDlg, 12450, 1000, NULL);
		}
		return (INT_PTR)TRUE;
	}
	case WM_TIMER:
	{
		if (wParam == 12450 && looping)
		{
			HWND    hSlider = GetDlgItem(hDlg, IDC_SLIDER);
			SendMessageW(hSlider, TBM_SETPOS, (WPARAM)1, (LPARAM)(int)(playbackPosition/(float)e_f_end * 1000));
			HWND hEdit = GetDlgItem(hDlg, IDC_T_POSITION);
			char tempStr[11];
			SampleIndexToStr(playbackPosition, tempStr, 10);
			for (int i = 0; i < 10; i++) if (tempStr[i] == '.')tempStr[i] = 0;
			SetWindowTextA(hEdit, tempStr);
		}
	}
	break;
	case WM_HSCROLL:
	{
		HWND    hSlider = GetDlgItem(hDlg, IDC_SLIDER);
		sliderValue = (LONG)SendMessageW(hSlider, TBM_GETPOS, 0, 0);
		sliderChanged = true;
		
	}
	break;
	case WM_LBUTTONUP:
	{
		if (sliderChanged) 
		{
			sliderChanged = false;
			playbackPosition = (sliderValue)*e_f_end / 1000;
			seeked = true;
		}
	}
	break;
	case WM_COMMAND:

		if (LOWORD(wParam) == IDOK)
		{
			OnEndDialog();
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_B_EDIT)
		{
			static_api_ptr_t<playback_control>()->stop();
			looping = false;
			hLooperDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_LOOPER),
				core_api::get_main_window(), LooperDialog);
			ShowWindow(hLooperDlg, SW_SHOW);
			hLooperPlayDlg = 0;
			EndDialog(hDlg, LOWORD(wParam));
		}
	}
	return (INT_PTR)FALSE;
}
DWORD WINAPI DrawWaveThread(PVOID p) 
{
	if ((int)p < 0 || (int)p>2)return -2;
	if (!drawWaveTemp[(int)p].hTempDC)return -1;
	drawWaveTemp[(int)p].drawing = true;
	drawWaveTemp[(int)p].needDraw = false;
	drawWaveTemp[(int)p].abort = false;
	sampleIndex start,end;
	switch ((int)p)
	{
	case 0:
		start = 0;
		end = GetSampleNum();
		break;
	case 1:
		start = e_1_start;
		end = e_1_end;
		break;
	case 2:
		start = e_2_start;
		end = e_2_end; 
		break;
	default:
		drawWaveTemp[(int)p].drawing = false;
		return -1;
	}
	DrawWave(drawWaveTemp[(int)p].hTempDC,
		drawWaveTemp[(int)p].x,
		drawWaveTemp[(int)p].y,
		drawWaveTemp[(int)p].width,
		drawWaveTemp[(int)p].height,
		start,
		end,
		drawWaveTemp[(int)p].abort);
	drawWaveTemp[(int)p].drawing=false;
	drawWaveTemp[(int)p].start = start;
	drawWaveTemp[(int)p].end = end;
	InvalidateRect(hLooperDlg,0,0);
	UpdateWindow(hLooperDlg);
	return 0;
}
