//########################################################################################
//
//		2013年度プログラミングコンテスト(自由部門)出展作品	【Z!BA】
//
//			File: Common.h		Ziba共通ヘッダファイル
//
//			香川高等専門学校詫間キャンパス プロコンチーム
//
//########################################################################################

#pragma once

//======================================================================================
//	マクロ定義
//======================================================================================

#define COMNO_LEFT			7				// MagBoard通信ポート番号
#define	COMNO_RIGHT			5
#define	COMNO_PEN			9

#define	BAUDRATE			1250000

//---------------------------------------------------------
// ウィンドウサイズ(=バックバッファの解像度)
//---------------------------------------------------------
// フロント(※立体視のためには、1280x720@60Hz or 1920x1080@24Hz)
#define	PRI_WIDTH		1280
#define	PRI_HEIGHT		 720

// タッチディスプレイ解像度
#define	SND_WIDTH		1280
#define	SND_HEIGHT		 768

// メニューの解像度
#define MENU_RESOL_X	(SND_WIDTH)
#define MENU_RESOL_Y	(SND_HEIGHT)

#define TPWidth			522.0			// タッチパネルサイズ(投影サイズ)
#define	TPHeight		295.0			// タッチパネルサイズ(投影サイズ)

#define	MBWidth			408.0			// MagBoardサイズ
#define MBHeight		268.0			// MagBoardサイズ

#define BoardW			1034	//44
#define BoardH			728 //728

//---------------------------------------------------------
//	DirectX関連
//---------------------------------------------------------

// マルチサンプリング設定(アンチエイリアス用)
#define	MS_COUNT		4//4		// GTX580で使用可能な(Count, Quality)
#define	MS_QUALITY		16//16		//  = (1, 0)  (2, 2), (4, 16) (8, 32)

//---------------------------------------------------------
//	磁界デバイス関連
//---------------------------------------------------------

// センサー数
#define XSENSORS		(24)			// X方向
#define YSENSORS		(16)			// Y方向

#define DATASIZE	(XSENSORS*YSENSORS*3*12/8/2)

// グリッド数
#define XPINS			(XSENSORS*3)	// X方向
#define YPINS			(YSENSORS*3)	// Y方向


// 磁界検出フィールドの大きさ(ワールド座標 Y=0 平面)
#define MF_WIDTH		(2.4f)
#define MF_HEIGHT		(1.6f)

// 磁界マップテクスチャ解像度
#define	MFMAPX			240
#define MFMAPY			160

// 高さマップテクスチャ解像度
#define	HMAPX			(XSENSORS*20)		//480
#define HMAPY			(YSENSORS*20)		//320


//---------------------------------------------------------
//	PhysX関連
//---------------------------------------------------------
#define MAXPARTICLES	20000



// ジョイスティックボタンの判定用マクロ
#define BUTTON(name, key) (name.rgbButtons[key] & 0x80)

#define BPRESS(cjs, pjs, key) (BUTTON(cjs, key) && !BUTTON(pjs, key))
#define BRELEASE(cjs, pjs, key) (!BUTTON(cjs, key) && BUTTON(pjs, key))

//BUTTON MASK
#define	BN_MARU		1
#define	BN_BATU		2	
#define	BN_SANKAKU	0
#define	BN_SIKAKU	3

#define	BN_R1		7
#define	BN_R2		5
#define	BN_L1		6
#define	BN_L2		4

#define	BN_L3		10
#define	BN_R3		11

#define BN_SELECT	9
#define BN_START	8


//---------------------------------------------------------
//	システムモード
//---------------------------------------------------------
typedef enum SYSMODE {
	SYS_OPENING,					// オープニング
	SYS_TOPMENU,					// トップメニュー
	SYS_VISUALIZE,					// 可視化モード
	SYS_ART_TOP,					// アートモード(TOP)
	SYS_ART_01,						// アートモード(WAVE)
	SYS_ART_02,						// アートモード(PAINT)
	SYS_AMUSE_TOP,					// アミューズメントモード(TOP)
	SYS_AMUSE_01,					// アミューズメントモード(GAME-01)
	SYS_AMUSE_02,					// アミューズメントモード(GAME-02)
} SYSMODE;

//---------------------------------------------------------
//	MFモード
//---------------------------------------------------------
typedef enum MFMODE {
	MF_POWER,
	MF_POLE,
	MF_VCOL,
	MF_HIGHT,
	MF_PICTURE,
	MF_WAVE,
} MFMODE;

//---------------------------------------------------------
//	MF_LINEモード
//---------------------------------------------------------
typedef enum MFLINE {
	ML_STRENGTH,
	ML_LOG,
	ML_FIXED,
} MFLINE;
