//======================================================================================
//======================================================================================

//-----------------------------------------------------------------------------------
//	インクルード・ヘッダ
//-----------------------------------------------------------------------------------
#include <process.h>
#include <stdint.h>
#include "Common.h"
#include "CMagBoard.h"

//-----------------------------------------------------------------------------------
//	マクロ
//-----------------------------------------------------------------------------------
#define	MAGBOARD_LEFT_ENABLE	1
#define	MAGBOARD_RIGHT_ENABLE	0
#define	MAGPENCIL_ENABLE		0

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

//-----------------------------------------------------------------------------------
//	マクロ
//-----------------------------------------------------------------------------------
CRITICAL_SECTION	cs;					// クリティカルセクション
extern HANDLE		g_hMutex;			// Mutex(スレッド間の衝突保護のため)

unsigned int __stdcall loopThread(void* thisPtr);	// スレッドループ

bool	flgDebug = true;

static double abs(double x) {
	if (x > 0) return x;
	return -x;
}

static double D3DXVec3Length(const D3DXVECTOR3* v) {
	double l = v->x * v->x + v->y * v->y + v->z * v->z;
	return sqrt(l);
}

//======================================================================================
//	コンストラクタ
//======================================================================================
CMagBoard::CMagBoard()
{
	m_hThread = NULL;

	m_pMFDat = NULL;
	m_MFDatBytes = 0;
	m_OnLoop = false;
	m_Count = 0;
	m_FPS = -1;

	m_PortEnable = false;

	m_BufferBytes = 0;

	/**/ fopen_s(&log_fp, "log.txt", "w");
}

//======================================================================================
//	デストラクタ
//======================================================================================
CMagBoard::~CMagBoard()
{
	Cleanup();

	// シリアルポート・クローズ
	m_SerialLeft.ClosePort();
	m_SerialRight.ClosePort();
	m_SerialPen.ClosePort();
	m_PortEnable = false;

	/**/ fclose(log_fp);
}

//======================================================================================
//	初期化
//======================================================================================
HRESULT CMagBoard::Init()
{
	HRESULT hr = S_OK;

	//--------------------------------------------------------
	// シリアルポートの準備
	//--------------------------------------------------------
	hr = OpenAllPort();
	if (FAILED(hr)) { return hr; }

	//--------------------------------------------------------
	// クリティカルセクションの初期化
	//--------------------------------------------------------
	InitializeCriticalSection(&cs);

	//--------------------------------------------------------
	// メモリ確保&初期化
	//--------------------------------------------------------

	// 受信バッファ
	m_BufferBytes = sizeof(SENSOR_3D) * XSENSORS * YSENSORS;
	ZeroMemory(m_pOrig, m_BufferBytes);		// 磁界データオリジナル
	ZeroMemory(m_pOrigPrev, m_BufferBytes);		// 磁界データオリジナル(1回前)
	ZeroMemory(m_pOffset, m_BufferBytes);		// オフセット

	// 磁界データバッファ
	m_pMFDat = new D3DXVECTOR3[XSENSORS * YSENSORS];
	m_MFDatBytes = sizeof(D3DXVECTOR3) * XSENSORS * YSENSORS;
	ZeroMemory(m_pMFDat, m_MFDatBytes);

	return hr;
}


//======================================================================================
//	メモリ解放
//======================================================================================
void CMagBoard::Cleanup()
{
	// 計測スレッド終了
	ThreadStop();

	//// スレッド終了を待ちスレッドクローズ
	//WaitForSingleObject( m_hThread, INFINITE );
	//CloseHandle( m_hThread );

	// データバッファ解放
	SAFE_DELETE_ARRAY(m_pMFDat);

	//	SAFE_DELETE_ARRAY(m_pBuffer);

		// クリティカルセクション終了
	DeleteCriticalSection(&cs);
}

//======================================================================================
//	全デバイスのポートをオープン
//======================================================================================
HRESULT CMagBoard::OpenAllPort()
{
	HRESULT hr = S_OK;
	bool ret;

	CSerialInterface test_serial;

	ULONG	baud_rate = BAUDRATE;
	ULONG	send_time_out = 10;//100;
	ULONG	recv_time_out = 10;//100;

	m_SerialLeft_Enable = FALSE;
	m_SerialRight_Enable = FALSE;
	m_SerialPen_Enable = FALSE;

	//--------------------------------------------------------
	// ポートOPEN
	//--------------------------------------------------------
	BYTE msg[1024];

	// すべてのポートにコマンド送信
	for (int port_number = 0; port_number < 255; port_number++) {

		// テストポートオープン
		ret = test_serial.OpenPort(port_number);
		if (!ret)		continue;

		// ポート設定
		ret = test_serial.PortSetting(baud_rate, send_time_out, recv_time_out);
		if (!ret)		continue;

		Sleep(100);

		// コマンド'C'送信
		ret = test_serial.SendString("C");

		Sleep(100);

		int n = test_serial.RecvData(msg, 1);	// 受信
		test_serial.ClosePort();				// ポート

		Sleep(100);

		switch (msg[0]) {

			// 左側センサー
		case 'L':
			if (m_SerialLeft_Enable)		break;

			ret = m_SerialLeft.OpenPort(port_number);
			if (!ret)	return E_FAIL;

			ret = m_SerialLeft.PortSetting(baud_rate, send_time_out, recv_time_out);
			if (!ret)	return E_FAIL;

			m_SerialLeft_Enable = TRUE;
			m_PortEnable = true;
			m_PortNo_Left = port_number;

			break;

			// 右側センサー
		case 'R':
			if (m_SerialRight_Enable)	break;

			ret = m_SerialRight.OpenPort(port_number);
			if (!ret)	return E_FAIL;

			ret = m_SerialRight.PortSetting(baud_rate, send_time_out, recv_time_out);
			if (!ret)	return E_FAIL;

			m_SerialRight_Enable = TRUE;
			m_PortEnable = true;
			m_PortNo_Right = port_number;

			break;

			// PENデバイス
		case 'P':
			if (m_SerialPen_Enable)		break;

			ret = m_SerialPen.OpenPort(port_number);
			if (!ret)	return E_FAIL;

			ret = m_SerialPen.PortSetting(baud_rate, send_time_out, recv_time_out);
			if (!ret)	return E_FAIL;

			m_SerialPen_Enable = TRUE;
			m_PortNo_Pen = port_number;

			break;
		}
	}

	return hr;
}

void CMagBoard::CloseAllPort()
{
	m_SerialLeft.ClosePort();
	m_SerialRight.ClosePort();
	m_SerialPen.ClosePort();
	m_PortEnable = false;
}


//======================================================================================
//	計測スレッド開始
//======================================================================================
void CMagBoard::ThreadStart()
{
	if (!m_PortEnable)	return;
	if (m_OnLoop == true)	return;

	unsigned int threadID;

	m_OnLoop = true;
	m_Count = 0;

	// スレッド生成
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &loopThread, (void*)this, 0, &threadID);

}

//======================================================================================
//	計測スレッド終了
//======================================================================================
void CMagBoard::ThreadStop()
{
	if (m_OnLoop == false)	return;
	if (!m_hThread)		return;

	m_OnLoop = false;

	// スレッド終了を待つ
	WaitForSingleObject(m_hThread, INFINITE);
}


//======================================================================================
//	計測スレッドリセット
//======================================================================================
bool CMagBoard::Reset()
{

	// スレッド停止
	ThreadStop();

	//--------------------------------------------------------------
	// ポートクローズ
	//--------------------------------------------------------------
	CloseAllPort();

	//--------------------------------------------------------------
	// 再オープン
	//--------------------------------------------------------------
	OpenAllPort();

	//--------------------------------------------------------------
	// オフセット初期化
	//--------------------------------------------------------------
	ZeroMemory(m_pOffset, m_BufferBytes);

	// スレッド開始
	ThreadStart();

	return true;
}

int CMagBoard::GetTakishitaCount() const
{
	return m_SerialLeft.TakishitaCount + m_SerialRight.TakishitaCount;
}

//======================================================================================
//	オフセット値更新
//======================================================================================
bool CMagBoard::SetOffset(bool flgZero)
{
	if (!m_PortEnable)	return false;
	if (!m_OnLoop)		return false;

	static SENSOR_3D	tmpBuf[YSENSORS][XSENSORS];


	// オフセット値をキャンセル(全て0に)
	if (flgZero)	ZeroMemory(m_pOffset, m_BufferBytes);

	else {
		// クリティカルセクションEnter
		EnterCriticalSection(&cs);

		// データコピー
		CopyMemory(tmpBuf, m_pOrig, m_BufferBytes);

		for (int y = 0; y < YSENSORS; y++) {
			for (int x = 0; x < XSENSORS; x++) {
				m_pOffset[y][x].x = tmpBuf[y][x].x - 2047;
				m_pOffset[y][x].y = tmpBuf[y][x].y - 2047;
				m_pOffset[y][x].z = tmpBuf[y][x].z - 2047;
			}
		}

		// クリティカルセクションLeave
		LeaveCriticalSection(&cs);
	}

	return true;
}

//======================================================================================
//	データ取得メソッド(他スレッドからの要求によりデータを返す)
//======================================================================================
bool CMagBoard::GetData(D3DXVECTOR3* buf, SENSOR_3D org[YSENSORS][XSENSORS]) const
{
	if (!m_PortEnable)	return false;
	if (!m_OnLoop)		return false;

	bool ret = false;
	static DWORD count = 0;

	// 前回よりカウント値が増加している(新しいデータが取得されている) → true
	if (m_Count != count)	ret = true;
	else { return ret; }

	// クリティカルセクションEnter
	EnterCriticalSection(&cs);

	// データコピー
	CopyMemory(buf, m_pMFDat, m_MFDatBytes);
	CopyMemory(org, m_pOrig, m_BufferBytes);

	// クリティカルセクションLeave
	LeaveCriticalSection(&cs);

	count = m_Count;

	return ret;
}

//======================================================================================
//	データ取得メソッド(他スレッドからの要求によりデータを返す)
//======================================================================================
bool CMagBoard::GetOrgData(SENSOR_3D buf[YSENSORS][XSENSORS]) const
{
	if (!m_PortEnable)	return false;

	bool ret = false;
	static DWORD count = 0;

	// 前回よりカウント値が増加している(新しいデータが取得されている) → true
	if (m_Count != count)	ret = true;
	else { return ret; }

	// クリティカルセクションEnter
	EnterCriticalSection(&cs);

	// データコピー
	CopyMemory(buf, m_pOrig, m_BufferBytes);

	// クリティカルセクションLeave
	LeaveCriticalSection(&cs);

	count = m_Count;

	return ret;
}

//======================================================================================
//	データ更新(デバイスからデータ読み取り)
//======================================================================================
void CMagBoard::Update()
{
	if (!m_PortEnable)	return;
	if (!m_OnLoop)		return;

	//--------------------------------------------------
	//	前回のデータをコピー
	//--------------------------------------------------
	CopyMemory(m_pOrigPrev, m_pOrig, m_BufferBytes);

	//--------------------------------------------------
	// デバイスからデータ受信
	//--------------------------------------------------
	BYTE msg[10];

	if (m_SerialLeft_Enable) {
		if (m_SerialLeft.ChkRecvBuffer() == DATASIZE) {
			m_SerialLeft.RecvDataRecombWithoutTakishita(m_pOrig, RDT_LEFT);
			//			m_SerialLeft.RecvDataRecomb(m_pOrig, RDT_LEFT);
		}
		while (m_SerialLeft.ChkRecvBuffer())	m_SerialLeft.RecvData(msg, 1);
	}
	if (m_SerialRight_Enable) {
		if (m_SerialRight.ChkRecvBuffer() == DATASIZE) {
			m_SerialRight.RecvDataRecombWithoutTakishita(m_pOrig, RDT_RIGHT);
			//			m_SerialRight.RecvDataRecomb(m_pOrig, RDT_RIGHT);
		}
		while (m_SerialRight.ChkRecvBuffer())	m_SerialRight.RecvData(msg, 1);
	}
	if (m_SerialPen_Enable) {
		if (m_SerialPen.ChkRecvBuffer() == 2) {
			int pressure;

			m_SerialPen.RecvData(msg, 2);
			pressure = msg[0] << 1 | msg[1];
		}
		while (m_SerialPen.ChkRecvBuffer()) m_SerialPen.RecvData(msg, 1);
	}

	if (m_SerialLeft_Enable)	m_SerialLeft.SendString("Z");	// 
	if (m_SerialRight_Enable)	m_SerialRight.SendString("Z");

	if (m_SerialPen_Enable) {
		BYTE snd_data[5] = { 0xff,0x00,0x00,0x00,0x00 };
		switch ((m_Count / 50) % 4) {
		case 0:
			snd_data[1] = m_Count % 50;
			snd_data[2] = m_Count % 50 * 5;
			break;
		case 1:
			snd_data[1] = 50 - m_Count % 50;
			snd_data[2] = 250 - m_Count % 50 * 5;
			break;
		case 2:
			snd_data[1] = -m_Count % 50;
			snd_data[4] = m_Count % 50 * 5;
			break;
		case 3:
			snd_data[1] = m_Count % 50 - 50;
			snd_data[4] = 250 - m_Count % 50 * 5;
			break;
		}

		m_SerialPen.SendData(snd_data, 5);
	}

	m_Count++;


	//--------------------------------------------------
	//	瀧下捜索 (x,y)偶数箇所に出現
	//--------------------------------------------------
	for (int y = 0; y < YSENSORS; y += 2) {
		for (int x = 0; x < XSENSORS; x += 2) {
			//if (m_pOrig[y][x].x==0x546 && 
			//	m_pOrig[y][x].y==0x16B &&
			//	m_pOrig[y][x].z==0x697 ) 
			if (m_pOrig[y][x].x == 0x00)
			{
				m_pOrig[y][x] = m_pOrigPrev[y][x];
				m_pOrig[y][x + 1] = m_pOrigPrev[y][x + 1];
				m_pOrig[y + 1][x] = m_pOrigPrev[y + 1][x];
				m_pOrig[y + 1][x + 1] = m_pOrigPrev[y + 1][x + 1];
			}

		}
	}

	//size_t a = sizeof(INT64);

	//int b = 16;		// 'T' 'a'	まで検索

	//INT64 d = 0;
	//INT64 s = 0x5461;
	//bool flag = false;

	//for (int y=0; y<YSENSORS; y+=1) {
	//	for (int x=0; x<XSENSORS; x+=1) {

	//		flag = false;

	//		d  = (m_pOrig[y][x].x & 0x00000FFF);	d<<=12;
	//		d += (m_pOrig[y][x].y & 0x00000FFF);	d<<=12;
	//		d += (m_pOrig[y][x].z & 0x00000FFF);

	//		for (int i=28; i<64; i++) {
	//			d<<=i;
	//			d>>=64-b;
	//			if ( d==s ) { flag = true;	break; }
	//		}

	//		// 前回のデータで埋める
	//		if ( flag ) {
	//			int X = x/2*2;
	//			int Y = y/2*2;
	//			m_pOrig[Y  ][X  ] = m_pOrigPrev[Y  ][X  ];
	//			m_pOrig[Y  ][X+1] = m_pOrigPrev[Y  ][X+1];
	//			m_pOrig[Y+1][X  ] = m_pOrigPrev[Y+1][X  ];
	//			m_pOrig[Y+1][X+1] = m_pOrigPrev[Y+1][X+1];
	//		}
	//	}
	//}

	/**/
	char binary[16] = "000000000000\0";

	if (m_Count != 1) {
		for (int y = 0; y < YSENSORS; y++) {
			for (int x = 0; x < XSENSORS; x++) {
				if (abs((double)m_pOrig[y][x].x - 2048) > 256) {
					for (int i = 0; i < 12; i++)
						binary[i] = (m_pOrig[y][x].x & 1 << (11 - i)) ? '1' : '0';
					fprintf(log_fp, "%5d : (%2d,%2d)-x : %5X , %s\n", m_Count, y, x, m_pOrig[y][x].x, binary);
				}
				if (abs((double)m_pOrig[y][x].y - 2048) > 256) {
					for (int i = 0; i < 12; i++)
						binary[i] = (m_pOrig[y][x].y & 1 << (11 - i)) ? '1' : '0';
					fprintf(log_fp, "%5d : (%2d,%2d)-y : %5X , %s\n", m_Count, y, x, m_pOrig[y][x].y, binary);
				}
				if (abs((double)m_pOrig[y][x].z - 2048) > 256) {
					for (int i = 0; i < 12; i++)
						binary[i] = (m_pOrig[y][x].z & 1 << (11 - i)) ? '1' : '0';
					fprintf(log_fp, "%5d : (%2d,%2d)-z : %5X , %s\n", m_Count, y, x, m_pOrig[y][x].z, binary);
				}
			}
		}
	}
	/**/



	//--------------------------------------------------
	// m_pMFDatの更新
	//--------------------------------------------------
	for (int y = 0; y < YSENSORS; y++) {
		for (int x = 0; x < XSENSORS; x++) {
			float dx = (float)(m_pOrig[y][x].x - m_pOffset[y][x].x) / 4095 - 0.5f;
			float dy = (float)(m_pOrig[y][x].y - m_pOffset[y][x].y) / 4095 - 0.5f;
			float dz = (float)(m_pOrig[y][x].z - m_pOffset[y][x].z) / 4095 - 0.5f;

			m_pMFDat[y * XSENSORS + x] = D3DXVECTOR3(-dx, -dz, -dy);

			UINT ox = m_pOrig[y][x].x;
			UINT oy = m_pOrig[y][x].y;
			UINT oz = m_pOrig[y][x].z;

			if (flgDebug && y < 14) {
				D3DXVECTOR3 v(dx, dy, dz);
				if (D3DXVec3Length(&v) > 0.3) {
					int a = 100;
				}
			}

		}
	}

	//m_FPS = GetFPS();
}

//======================================================================================
//	計測スレッドループ
//======================================================================================
//void loopThread(void *thisPtr)
unsigned int __stdcall loopThread(void* thisPtr)
{
	CMagBoard* pMB = (CMagBoard*)thisPtr;

	// ループフラグONの間ループ
	while (pMB->m_OnLoop) {

		// クリティカルセクションEnter
		EnterCriticalSection(&cs);

		// データ更新
		pMB->Update();

		// クリティカルセクションLeave
		LeaveCriticalSection(&cs);

		Sleep(33);
		//		Sleep(100);

	}

	// スレッド終了
	_endthreadex(0);

	return 0;
}

//======================================================================================
//	FPSの計測
//======================================================================================
//float CMagBoard::GetFPS() const
//{
//	if (!m_PortEnable)	return 0.0f;
//
//	// FPS計測
//	static DWORD frames = 0;
//	static DWORD start = timeGetTime();
//	DWORD elapsed = timeGetTime() - start;
//	static float fps = 0.0f;
//
//	if (elapsed > 1000) {
//		fps = (float)frames / (elapsed / 1000.0f);
//		frames = 0;
//		start = timeGetTime();
//	}
//	frames++;
//
//	return fps;
//}
