//######################################################################################
//
//		File: CMagBoard.h		BagBoard/MagPencilクラス
//
//######################################################################################
#pragma once

//-----------------------------------------------------------------------------------
//	インクルード・ヘッダ
//-----------------------------------------------------------------------------------
#include "Common.h"
#include "SerialInterface.h"				// シリアル通信による動作テスト用

typedef struct D3DXVECTOR3 {
	FLOAT x;
	FLOAT y;
	FLOAT z;
} D3DXVECTOR3;

//======================================================================================
//	CMagBoard
//======================================================================================
class CMagBoard
{
private:
	/**/ FILE* log_fp;

	HANDLE				m_hThread;					// スレッドハンドル

	CSerialInterface	m_SerialLeft;				// 
	CSerialInterface	m_SerialRight;
	CSerialInterface	m_SerialPen;

	int					m_PortNo_Left;
	int					m_PortNo_Right;
	int					m_PortNo_Pen;

	BOOL				m_SerialLeft_Enable;		// ポート使用可能フラグ	(LEFT)
	BOOL				m_SerialRight_Enable;		// ポート使用可能フラグ	(RIGHT)
	BOOL				m_SerialPen_Enable;			// ポート使用可能フラグ	(PEN)

	LONG				m_MFDatBytes;				// 磁界計測データバイトサイズ
	D3DXVECTOR3* m_pMFDat;					// 磁界計測データバッファへのポインタ

	LONG				m_BufferBytes;						// 
	SENSOR_3D			m_pOrig[YSENSORS][XSENSORS];		// 磁界データ・オリジナル
	SENSOR_3D			m_pOrigPrev[YSENSORS][XSENSORS];	// 磁界データ・オリジナル
	SENSOR_3D			m_pOffset[YSENSORS][XSENSORS];		// 磁界データ・オフセット

	DWORD				m_Count;					// カウント

public:
	bool				m_PortEnable;
	bool				m_OnLoop;
	float				m_FPS;

public:

	CMagBoard();
	~CMagBoard();

	HRESULT Init();									// 初期化
	void	Cleanup();								// 解放

	HRESULT OpenAllPort();
	void	CloseAllPort();

	bool	GetData(D3DXVECTOR3* buf, SENSOR_3D org[YSENSORS][XSENSORS]) const;	// データ受信
	bool	GetOrgData(SENSOR_3D buf[YSENSORS][XSENSORS]) const;

	bool	SetOffset(bool flgZero);				// オフセット更新

	void	Update();

	void	ThreadStart();								// 計測ループ開始
	void	ThreadStop();									// 計測ループ停止
	bool	Reset();								// 計測ループリセット

	float	GetFPS() const;
	int		GetTakishitaCount() const;
};