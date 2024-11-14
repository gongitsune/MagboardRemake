//######################################################################################
//
//		File: SerialInterface.h		シリアル通信クラス
//
//######################################################################################
#pragma once

#include <windows.h>
#include <stdio.h>
#include "Common.h"
/*
#define	XSENSORS	24
#define YSENSORS	16
#define	DATASIZE	(XSENSORS*YSENSORS/2*3*12/8)
*/
#define DEFAULT_BUFFER_SIZE		65536

enum RECOMB_DATA_TYPE {
	RDT_LEFT,
	RDT_RIGHT
};



// 受信データ型 (3次元ベクトル)	0-4095
struct SENSOR_3D {
	unsigned int x, y, z;
};

//=================================================================================
//	シリアル通信クラス	CSerialInterface
//=================================================================================
class CSerialInterface
{

private:
	HANDLE		hPort;					// COMポートハンドル

	//	CString		ErrorMsg;
	ULONG		PortNumber;				// ポート番号
	ULONG		SendBufSize;			// 送信バッファサイズ
	ULONG		RecvBufSize;			// 受信バッファサイズ

public:
	int TakishitaCount;

public:
	//-----------------------------------------------------------
	//	コンストラクタ
	//-----------------------------------------------------------
	CSerialInterface()
	{
		ResetPort();
		TakishitaCount = 0;
	};

	//-----------------------------------------------------------
	//	コンストラクタ
	//-----------------------------------------------------------
	CSerialInterface(ULONG port_number, ULONG send_buffer_size = DEFAULT_BUFFER_SIZE, ULONG recv_buffer_size = DEFAULT_BUFFER_SIZE)
	{
		ResetPort();
		OpenPort(port_number, send_buffer_size, recv_buffer_size);
	}

	//-----------------------------------------------------------
	//	デストラクタ
	//-----------------------------------------------------------
	virtual ~CSerialInterface()
	{
		ClosePort();
	};

	//CString GetError()
	//{
	//	return(ErrorMsg);
	//};

private:

	//-----------------------------------------------------------
	//	ポート・リセット
	//-----------------------------------------------------------
	void ResetPort()
	{
		hPort = INVALID_HANDLE_VALUE;
		PortNumber = 0;
		SendBufSize = 0;
		RecvBufSize = 0;
	};

	inline bool SetError(const char* error) {
		//		ErrorMsg = error;
		//		return(ErrorMsg.IsEmpty());
		return true;
	};

public:

	//-----------------------------------------------------------
	//	ポート・オープン
	//-----------------------------------------------------------
	bool OpenPort(ULONG port_number, ULONG send_buffer_size = DEFAULT_BUFFER_SIZE, ULONG recv_buffer_size = DEFAULT_BUFFER_SIZE)
	{
		if (hPort != INVALID_HANDLE_VALUE)			ClosePort();

		BOOL Ret;
		char PortName[64];
		sprintf_s(PortName, 64, "\\\\.\\COM%d", port_number);

		hPort = CreateFileA(PortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hPort == INVALID_HANDLE_VALUE)			return false;

		Ret = SetupComm(hPort, send_buffer_size, recv_buffer_size);
		if (Ret == FALSE) {
			ClosePort();
			return false;
		}

		Ret = PurgeComm(hPort, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
		if (Ret == FALSE) {
			ClosePort();
			return false;
		}

		PortNumber = port_number;
		SendBufSize = send_buffer_size;
		RecvBufSize = recv_buffer_size;

		return true;
	};

	//-----------------------------------------------------------
	//	ポート・クローズ
	//-----------------------------------------------------------
	bool ClosePort()
	{
		if (hPort == INVALID_HANDLE_VALUE)			return false;

		BOOL Ret = CloseHandle(hPort);
		if (Ret) { ResetPort(); return true; }
		else			return false;
	};

	//-----------------------------------------------------------
	//	ポート設定
	//-----------------------------------------------------------
	bool PortSetting(ULONG baud_rate, ULONG send_time_out, ULONG recv_time_out)
	{
		if (hPort == INVALID_HANDLE_VALUE)			return false;

		DCB dcb;

		dcb.DCBlength = (sizeof(DCB));
		dcb.BaudRate = baud_rate;
		dcb.fBinary = TRUE;
		dcb.fParity = FALSE;

		dcb.fOutxCtsFlow = FALSE;
		dcb.fOutxDsrFlow = FALSE;
		dcb.fDtrControl = DTR_CONTROL_DISABLE;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;

		dcb.fDsrSensitivity = FALSE;
		dcb.fTXContinueOnXoff = TRUE;

		dcb.fOutX = FALSE;
		dcb.fInX = FALSE;
		dcb.ErrorChar = 0x00;
		dcb.fNull = FALSE;

		dcb.fAbortOnError = FALSE;
		dcb.XonLim = 512;
		dcb.XoffLim = 512;

		dcb.ByteSize = 8;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;

		dcb.XonChar = 0x11;
		dcb.XoffChar = FALSE;

		dcb.fAbortOnError = TRUE;
		dcb.fErrorChar = FALSE;
		dcb.ErrorChar = (char)0xFF;
		dcb.EofChar = 0x03;
		dcb.EvtChar = 0x02;

		BOOL Ret = SetCommState(hPort, &dcb);
		if (Ret == FALSE) {
			ClosePort();
			return false;
		}

		COMMTIMEOUTS	timeout;

		timeout.ReadIntervalTimeout = recv_time_out;
		timeout.ReadTotalTimeoutMultiplier = 0;
		timeout.ReadTotalTimeoutConstant = recv_time_out;

		timeout.WriteTotalTimeoutMultiplier = 0;
		timeout.WriteTotalTimeoutConstant = send_time_out;

		Ret = SetCommTimeouts(hPort, &timeout);
		if (Ret == FALSE) {
			ClosePort();
			return false;
		}

		return true;
	};


	//-----------------------------------------------------------
	//	メッセージの送信
	//-----------------------------------------------------------
	bool SendString(const char message[])
	{
		return SendData((BYTE*)message, (ULONG)strlen(message));
	};

	//-----------------------------------------------------------
	//	データ送信
	//-----------------------------------------------------------
	bool SendData(const BYTE message[], ULONG size)
	{
		// ポートハンドルチェック
		if (hPort == INVALID_HANDLE_VALUE) { return false; }

		DWORD	dwSentDataSize;
		COMSTAT	Comstat;
		DWORD	dwErrorMask;

		ClearCommError(hPort, &dwErrorMask, &Comstat);
		while (((1024 - Comstat.cbOutQue) <= size));// || (Comstat.fCtsHold == FALSE));

		// データ送信
		BOOL Ret = WriteFile(hPort, message, size, &dwSentDataSize, NULL);

		// 失敗でポートクローズ
		if (Ret == FALSE) {
			//			CloseHandle(hPort);
			return false;
		}

		// 送信データサイズが異なればクローズ
		if (size != dwSentDataSize) {
			//			CloseHandle(hPort);
			return false;
		}

		return true;
	};

	//-----------------------------------------------------------
	//	受信バッファのチェック
	//	ClearCommError関数を用いてポートの状態を取得し,受信バッファのバイト数を調べる
	//-----------------------------------------------------------
	int ChkRecvBuffer(void) {
		DWORD dwError;
		COMSTAT CommStat;

		if (hPort == INVALID_HANDLE_VALUE) {
			return(-1);
		}

		ClearCommError(hPort, &dwError, &CommStat);
		// ClearCommError(書き込みをするシリアルポートのハンドル, エラー情報を格納する変数へのポインタ, COMSTAT構造体へのポインタ)
		// 受信があった場合、この構造体のメンバのcbInQueに受信バッファのバイト数が格納。

		return(CommStat.cbInQue);
	};


	//-----------------------------------------------------------
	//	データ受信
	//		ポートオープン失敗		-1
	//		読み取りサイズ違い		-1
	//		成功					size
	//		受信バッファ小さい		 0
	//-----------------------------------------------------------
	int RecvData(BYTE* message, ULONG size)
	{
		// ポートオープンのチェック
		if (hPort == INVALID_HANDLE_VALUE) {
			SetError("Port is not opened.");
			return(-1);
		}

		// 受信バッファのサイズを確認
		int recv_size = ChkRecvBuffer();
		DWORD dwReadDataSize;

		// サイズOK
		if ((ULONG)recv_size >= size) {

			// ポートからデータを読み取り
			BOOL Ret = ReadFile(hPort, message, size, &dwReadDataSize, NULL);
			if (Ret == FALSE) { return(-1); }

			// 読み取りデータのサイズのチェック
			if (size != dwReadDataSize) {
				return(-1);
			}
			return(size);
		}

		return(0);
	};

	//-------------------------------------------------------------------
	//	受信データを並び替え
	//-------------------------------------------------------------------
	int RecvDataRecomb(SENSOR_3D data[YSENSORS][XSENSORS], RECOMB_DATA_TYPE type)
	{
		static BYTE recv_data[DATASIZE];

		int offset = (type == RDT_RIGHT) ? 12 : 0;

		// 受信
		if (ChkRecvBuffer() >= DATASIZE) {
			int n = RecvData(recv_data, DATASIZE);

			// データ変換
			unsigned int sensor_buf[8][3];

			for (int x = 0; x < XSENSORS / 2; x++) {			// sensor
				for (int dy = 0; dy < 2; dy++) {
					for (int d = 0; d < 3; d++) {		// dimension
						for (int y = 0; y < 8; y++) {
							sensor_buf[y][d] = 0x00;
						}
						for (int b = 0; b < 12; b++) {	// byte
							int pos = x * 72 + dy * 36 + d * 12 + b;

							for (int y = 0; y < 8; y++) {
								sensor_buf[y][d] <<= 1;
								sensor_buf[y][d] |= (recv_data[pos] & 0x01);
								recv_data[pos] >>= 1;
							}
						}
					}
					for (int y = 0; y < 8; y++) {
						int y2 = y * 2 + dy;
						data[y2][x + offset].x = sensor_buf[y][0];
						data[y2][x + offset].y = sensor_buf[y][1];
						data[y2][x + offset].z = sensor_buf[y][2];
					}
				}
			}

			return(DATASIZE);
		}

		return (0);
	}

	int RecvDataRecombWithoutTakishita(SENSOR_3D data[YSENSORS][XSENSORS], RECOMB_DATA_TYPE type)
	{
		static BYTE recv_data[DATASIZE];

		int offset = (type == RDT_RIGHT) ? 12 : 0;

		// 受信
		if (ChkRecvBuffer() >= DATASIZE) {
			int n = RecvData(recv_data, DATASIZE);

			const int Threshold = 8;

			int Takishita[144] = { 0,1,0,1,0,1,0,0, /*T*/	0,1,1,0,0,0,0,1, /*a*/	0,1,1,0,1,0,1,1, /*k*/
									0,1,1,0,1,0,0,1, /*i*/	0,1,1,1,0,0,1,1, /*s*/	0,1,1,0,1,0,0,0, /*h*/
									0,1,1,0,1,0,0,1, /*i*/	0,1,1,1,0,1,0,0, /*t*/	0,1,1,0,0,0,0,1, /*a*/
									0,0,1,0,0,0,0,0, /* */	0,1,0,1,0,0,1,1, /*S*/	0,1,1,0,1,0,0,0, /*h*/
									0,1,1,0,1,1,1,1, /*o*/	0,0,1,0,0,0,0,0, /* */	0,0,1,1,0,0,1,1, /*3*/
									0,0,1,0,1,1,0,1, /*-*/	0,1,0,0,1,0,0,1, /*I*/	0,1,0,1,0,1,0,0	 /*T*/ };

			for (int b = 0; b < 8; b++) {
				int mask = 1 << b;

				int x, i, j;

				for (x = 0; x < 6; x++) {
					for (i = 0; i < 144 - Threshold; i++) {
						for (j = 0; j + i < 144; j++) {
							if ((recv_data[x * 144 + i + j] & mask) != Takishita[j])	break;

							if (j >= 20)
								j = j;
						}
						if (i + j == 144)											break;
					}

					if (i + j == 144) {
						TakishitaCount++;
						for (i = 0; i < 144; i++) {
							recv_data[x * 144 + i] &= ~mask;
						}
					}
				}

				for (int i = 0; i < 144; i++)	Takishita[i] <<= 1;
			}

			// データ変換
			unsigned int sensor_buf[8][3];

			for (int x = 0; x < XSENSORS / 2; x++) {			// sensor
				for (int dy = 0; dy < 2; dy++) {
					for (int d = 0; d < 3; d++) {		// dimension
						for (int y = 0; y < 8; y++) {
							sensor_buf[y][d] = 0x00;
						}
						for (int b = 0; b < 12; b++) {	// byte
							int pos = x * 72 + dy * 36 + d * 12 + b;

							for (int y = 0; y < 8; y++) {
								sensor_buf[y][d] <<= 1;
								sensor_buf[y][d] |= (recv_data[pos] & 0x01);
								recv_data[pos] >>= 1;
							}
						}
					}
					for (int y = 0; y < 8; y++) {
						int y2 = y * 2 + dy;
						data[y2][x + offset].x = sensor_buf[y][0];
						data[y2][x + offset].y = sensor_buf[y][1];
						data[y2][x + offset].z = sensor_buf[y][2];
					}
				}
			}

			return(DATASIZE);
		}

		return (0);
	}
};
