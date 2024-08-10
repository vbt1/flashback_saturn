/*                                       */
/*      SEGA Library defines Ver3.00     */
/*                                       */

#ifndef SEGA_XPT_H
#define SEGA_XPT_H

#if 0
#  define ST_V				/* or ST_VE (for Enhanced ST-V) */
#  define TITAN
#endif

#define		shc		1
#define		sh_gcc		2
#define		Compiler	sh_gcc		/* fot Hitachi then "shc" */

#if Compiler == shc
#  define  STCNS  static const
#else
#  define  STCNS  static
#endif

/*****************************************************************************/
/****************************** ��{�f�[�^�^�錾 *****************************/
/*****************************************************************************/

typedef unsigned char	Uint8 ;		/* �����Ȃ��P�o�C�g���� */
typedef signed   char	Sint8 ;		/* �������P�o�C�g���� */
typedef unsigned short	Uint16 ;	/* �����Ȃ��Q�o�C�g���� */
typedef signed   short	Sint16 ;	/* �������Q�o�C�g���� */
typedef unsigned long	Uint32 ;	/* �����Ȃ��S�o�C�g���� */
typedef signed   long	Sint32 ;	/* �������S�o�C�g���� */
typedef float		Float32 ;	/* �S�o�C�g���� */
typedef double		Float64 ;	/* �W�o�C�g���� */

typedef int		Int ;		/* INT�^�i�c�[���p�j		*/

typedef int		Bool ;		/* �_���^�i�_���萔��l�ɂƂ�j	*/

/*****************************************************************************/
/********************************* �萔�}�N�� ********************************/
/*****************************************************************************/

#ifndef NULL
#define NULL		((void *)0)	/* NULL */
#endif  /* NULL */

/*****************************************************************************/
/********************************** �񋓒萔 *********************************/
/*****************************************************************************/

enum BooleanLogic {			/* �_���萔�P�i�U�A�^�j */
    FALSE = 0 ,
    TRUE  = 1
};

enum BooleanSwitch {			/* �_���萔�Q�i�X�C�b�`�j */
    OFF = 0 ,
    ON  = 1
};

enum Judgement {			/* ���ʔ���̒萔�i�����A���s�j */
    OK = 0 ,				/* ���� */
    NG = -1				/* ���s */
};

/*****************************************************************************/
/********************************* �����}�N�� ********************************/
/*****************************************************************************/

#define MAX(x, y)	((x) > (y) ? (x) : (y))	/* �ő�l */
#define MIN(x, y)	((x) < (y) ? (x) : (y))	/* �ŏ��l */
#define ABS(x)		((x) < 0 ? -(x) : (x))	/* ��Βl */

/*------------------------------------------------------------------------------------*/

#define	    DI		15		/* ���荞�݂̋֎~ */
#define	    EI		 0		/* ���荞�݂̋��� */

#if 0	/* Ver2.1 ���폜 */
#define	    MAX_EVENTS	64		/* �g�p�ł���C�x���g�̐� */
#define	    MAX_WORKS	256		/* �g�p�ł��郏�[�N�̐� */
#endif

#define	    MAX_NEST	20		/* �}�g���N�X�̃l�X�g�ł��鐔 */

#define	    WORK_SIZE	0x40		/* ���[�N�̑傫�� */
#define	    EVENT_SIZE	0x80		/* �C�x���g�̑傫�� */

typedef	    Sint16	ANGLE ;		/* ����R�U�O�x�� �P�U�r�b�g�ŕ\��  */
typedef	    Sint32	FIXED ;		/* ��ʂP�U�r�b�g�Ő������A���ʂP�U�r�b�g�ŏ�������\���Œ菬���_�^ */

enum ps {X , Y , Z , XYZ , XYZS , XYZSS , XY = Z , S = XYZ , Sh = S , Sv = XYZS} ;

typedef struct work {
    struct work *next ;
    Uint8	user[WORK_SIZE - sizeof(struct work *)] ;
} WORK ;

typedef struct evnt {
    WORK		*work ;
    struct evnt		*next ;
    struct evnt		*before ;
    void		(*exad)() ;
    Uint8		 user[EVENT_SIZE - (sizeof(WORK *) + sizeof(struct evnt *) * 2 + sizeof(void (*)()))] ;
} EVENT ;

typedef     FIXED	MATRIX[4][3] ;		/* �}�g���N�X�f�[�^ */
typedef     FIXED	VECTOR[XYZ] ;		/* �x�N�g���f�[�^ */
typedef     FIXED	POINT[XYZ] ;		/* ���_�̈ʒu�f�[�^ */

#define	    CURRENT		0		/* Current Matrix */
#define	    ORIGINAL		(0.99999)	/* Original Scale */

enum mtrx {M00 , M01 , M02 ,		/* �}�g���N�X�̗v�f�ԍ� */
	   M10 , M11 , M12 ,
	   M20 , M21 , M22 ,
	   M30 , M31 , M32 ,
	   MTRX
	   } ;

enum tvsz {
	TV_320x224 , TV_320x240 , TV_320x256 , TV_dummy1 ,
	TV_352x224 , TV_352x240 , TV_352x256 , TV_dummy2 ,
	TV_640x224 , TV_640x240 , TV_640x256 , TV_dummy3 ,
	TV_704x224 , TV_704x240 , TV_704x256 , TV_dummy4 ,

	TV_320x448 , TV_320x480 , TV_320x512 , TV_dummy5 ,
	TV_352x448 , TV_352x480 , TV_352x512 , TV_dummy6 ,
	TV_640x448 , TV_640x480 , TV_640x512 , TV_dummy7 ,
	TV_704x448 , TV_704x480 , TV_704x512 , TV_dummy8
    } ;

#define		_SysPause		64
#define		_SprPause		 2
#define		_ScrPause		 1

/*----------------------------------------------------------------------------------*/

//#define     M_PI                3.1415926535897932
//#define	    toFIXED(a)		((FIXED)(65536.0 * (a)))
#define	    toFIXED(a)		((FIXED)(a<<16))
#define	    POStoFIXED(x,y,z)	{toFIXED(x),toFIXED(y),toFIXED(z)}
#define	    ATTRIBUTE(f,s,t,c,g,a,d,o)	{f,(s)|(((d)>>16)&0x1c)|(o),t,(a)|(((d)>>24)&0xc0),c,g,(d)&0x3f}
#define	    SPR_ATTRIBUTE(t,c,g,a,d)	{t,(a)|(((d)>>24)&0xc0),c,g,(d)&0x0f3f}
#define	    DEGtoANG(d)		((ANGLE)((65536.0 * (d)) / 360.0))
#define     RADtoANG(d)         ((ANGLE)((65536.0 * (d)) / (2*M_PI)))
#define	    RGB(r,g,b)		(0x8000|((b)<<10)|((g)<<5)|(r))
#define	    DGTtoRGB(c)		(0x8000|(((c)&0x0f00)<<3)|(((c)&0x4000)>>4)|(((c)&0x00f0)<<2)|(((c)&0x2000)>>8)|(((c)&0x000f)<<1)|(((c)&0x1000)>>12))
#define	    SLocate(x,y,p)	((p) * Page + (y) * Line + (x))

/*  Sprite control function */

#define	    FUNC_Sprite		1
#define	    FUNC_Texture	2
#define	    FUNC_Polygon	4
#define	    FUNC_PolyLine	5
#define	    FUNC_Line		6
#define	    FUNC_SystemClip	9
#define	    FUNC_UserClip	8
#define	    FUNC_BasePosition	10
#define	    FUNC_End		-1

#define	    _ZmLT		(0x05 << 8)	/* Zoom base Left Top */
#define	    _ZmLC		(0x09 << 8)	/* Zoom base Left Center */
#define	    _ZmLB		(0x0d << 8)	/* Zoom base Left Bottom */
#define	    _ZmCT		(0x06 << 8)	/* Zoom base Center Top */
#define	    _ZmCC		(0x0a << 8)	/* Zoom base Center Center */
#define	    _ZmCB		(0x0e << 8)	/* Zoom base Center Bottom */
#define	    _ZmRT		(0x07 << 8)	/* Zoom base Right Top */
#define	    _ZmRC		(0x0b << 8)	/* Zoom base Right Center */
#define	    _ZmRB		(0x0f << 8)	/* Zoom base Right Bottom */

#define	    MSBon		(1 << 15)	/* �t���[���o�b�t�@�ɏ������ނl�r�a���P�ɂ��� */
#define	    HSSon		(1 << 12)	/* �n�C�X�s�[�h�V�������N�L�� */
#define	    HSSoff		(0 << 12)	/* �n�C�X�s�[�h�V�������N����(default) */
#define	    No_Window		(0 << 9)	/* �E�B���h�E�̐������󂯂Ȃ�(default)*/
#define	    Window_In		(2 << 9)	/* �E�B���h�E�̓����ɕ\�� */
#define	    Window_Out		(3 << 9)	/* �E�B���h�E�̊O���ɕ\�� */
#define	    MESHoff		(0 << 8)	/* �ʏ�\��(default) */
#define	    MESHon		(1 << 8)	/* ���b�V���ŕ\�� */
#define	    ECdis		(1 << 7)	/* �G���h�R�[�h���p���b�g�̂ЂƂƂ��Ďg�p */
#define	    ECenb		(0 << 7)	/* �G���h�R�[�h�L�� */
#define	    SPdis		(1 << 6)	/* �X�y�[�X�R�[�h���p���b�g�̂ЂƂƂ��Ďg�p */
#define	    SPenb		(0 << 6)	/* �X�y�[�X�͕\�����Ȃ�(default) */
#define	    CL16Bnk		(0 << 3)	/* �J���[�o���N�P�U�F���[�h (default) */
#define	    CL16Look		(1 << 3)	/* �J���[���b�N�A�b�v�P�U�F���[�h */
#define	    CL64Bnk		(2 << 3)	/* �J���[�o���N�U�S�F���[�h */
#define	    CL128Bnk		(3 << 3)	/* �J���[�o���N�P�Q�W�F���[�h */
#define	    CL256Bnk		(4 << 3)	/* �J���[�o���N�Q�T�U�F���[�h */
#define	    CL32KRGB		(5 << 3)	/* �q�f�a�R�Q�j�F���[�h */
#define	    CL_Replace		0		/* �d�ˏ���(�㏑��)���[�h */
#define	    CL_Shadow		1		/* �e���[�h */
#define	    CL_Half		2		/* ���P�x���[�h */
#define	    CL_Trans		3		/* ���������[�h */
#define	    CL_Gouraud		4		/* �O�[���[�V�F�[�f�B���O���[�h */

#define	    UseTexture		(1 << 2)	/* �e�N�X�`����\��|���S�� */
#define	    UseLight		(1 << 3)	/* �����̉e�����󂯂�|���S�� */
#define	    UsePalette		(1 << 5)	/* �|���S���̃J���[ */

/* SGL Ver3.0 Add */
#define	    UseNearClip		(1 << 6)	/* �j�A�E�N���b�s���O������ */
#define	    UseGouraud		(1 << 7)	/* ���A���O�[���[ */
#define	    UseDepth		(1 << 4)	/* �f�v�X�L���[ */
/* */

#if 0 /* SGL Ver3.0 Delete */
#define	    UseClip		(1 << 4)		/* ��ʊO�ɒ��_���o����\�����Ȃ� */
#else
#define	    UseClip		UseNearClip		/* �j�A�E�N���b�s���O������ */
#endif

#define	    sprHflip		((1 << 4) | FUNC_Texture | (UseTexture << 16))
#define	    sprVflip		((1 << 5) | FUNC_Texture | (UseTexture << 16))
#define	    sprHVflip		((3 << 4) | FUNC_Texture | (UseTexture << 16))
#define	    sprNoflip		((0) | FUNC_Texture | (UseTexture << 16))
#define	    sprPolygon		(FUNC_Polygon | ((ECdis | SPdis) << 24))
#define	    sprPolyLine		(FUNC_PolyLine | ((ECdis | SPdis) << 24))
#define	    sprLine		(FUNC_Line | ((ECdis | SPdis) << 24))

#define	    No_Texture		0		/* �e�N�X�`�����g�p���Ȃ��� */
#define	    No_Option		0		/* �I�v�V�������g�p���Ȃ��� */
#define	    No_Gouraud		0		/* �O�[���[�V�F�[�f�B���O���g�p���Ȃ��� */
#define	    No_Palet	        0		/* �J���[�p���b�g�̎w�肪����Ȃ��� */
#define	    GouraudRAM		(0x00080000 - (32*8))	/* �����̉e���p�̃O�[���[�o�b�t�@ */

typedef struct {
    VECTOR	 norm ;			/* �@���x�N�g�� */
    Uint16	 Vertices[4] ;		/* �|���S�����\�����钸�_�ԍ� */
} POLYGON ;

typedef struct {
    Uint8	 flag ;			/* �Жʂ����ʂ��̃t���O */
    Uint8	 sort ;			/* �\�[�g�̊�ʒu�ƃI�v�V�����ݒ� */
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 atrb ;			/* �A�g���r���[�g�f�[�^(�\�����[�h) */
    Uint16	 colno ;		/* �J���[�i���o�[ */
    Uint16	 gstb ;			/* �O�[���[�V�F�[�f�B���O�e�[�u�� */
    Uint16	 dir ;			/* �e�N�X�`���̔��]�ƃt�@���N�V�����i���o�[ */
} ATTR ;

typedef struct {
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 atrb ;			/* �A�g���r���[�g�f�[�^(�\�����[�h) */
    Uint16	 colno ;		/* �J���[�i���o�[ */
    Uint16	 gstb ;			/* �O�[���[�V�F�[�f�B���O�e�[�u�� */
    Uint16	 dir ;			/* �e�N�X�`���̔��]�ƃt�@���N�V�����i���o�[ */
} SPR_ATTR ;

typedef struct {
    POINT	*pntbl ;		/* ���_�̈ʒu�f�[�^�e�[�u�� */
    Uint32	 nbPoint ;		/* ���_�̐� */
    POLYGON	*pltbl ;		/* �|���S����`�e�[�u�� */
    Uint32	 nbPolygon ;		/* �|���S���̐� */
    ATTR	*attbl ;		/* �|���S���̃A�g���r���[�g�e�[�u�� */
} PDATA ;

/* SGL Ver3.0 Add */
typedef struct {
    POINT	*pntbl ;		/* ���_�̈ʒu�f�[�^�e�[�u�� */
    Uint32	 nbPoint ;		/* ���_�̐� */
    POLYGON	*pltbl ;		/* �|���S����`�e�[�u�� */
    Uint32	 nbPolygon ;		/* �|���S���̐� */
    ATTR	*attbl ;		/* �|���S���̃A�g���r���[�g�e�[�u�� */
    VECTOR	*vntbl;
} XPDATA ;
/* */

#ifdef ST_VE

typedef struct {
    Uint16	 pmod ;			/* �X�v���C�g�\�����[�h */
    Uint16	 colno ;		/* �J���[�i���o�[ */
    Uint16	 CGadr ;		/* �X�v���C�g�b�f�A�h���X / �W */
    Uint16	 HVsize ;		/* �g�T�C�Y/�W�A�u�T�C�Y(�n�[�h�p) */
} gxATTR_VDP1 ;

typedef struct {
    Uint32	 texcel ;		/* �e�N�X�`���Z���A�h���X */
    Uint16	 dspmode ;		/* �f�B�X�v���C���[�h�t���O */
    Uint16	 colno ;		/* �J���[�f�[�^ */
    Uint8	 TXA , TYA ;		/* �e�N�X�`���|�C���g */
    Uint8	 TXB , TYB ;
    Uint8	 TXC , TYC ;
    Uint8	 TXD , TYD ;
} gxATTR_ENHC ;

typedef struct{
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 atrb ;			/* �A�g���r���[�g�f�[�^(�\�����[�h) */
    Uint16	 colno ;		/* �J���[�i���o�[ */
    Uint16	 gstb ;			/* �O�[���[�V�F�[�f�B���O�e�[�u�� */
    Uint16	 dir ;			/* �e�N�X�`���̔��]�ƃt�@���N�V�����i���o�[ */
} gxSPR_ATTR ;

typedef struct {
    Uint16	texno ;			/* �e�N�X�`���I�t�Z�b�g�A�h���X */
    Uint8	attr ;			/* �����R�[�h�A�\�[�e�B���O���[�h�� */
    Uint8	nvface ;		/* �ʖ@���x�N�g���i���o�[ */
    Uint8	nvnum[4] ;		/* ���_�@���x�N�g���i���o�[ */
    Uint8	Vertices[4] ;		/* �|���S�����\�����钸�_�ԍ� */
} gxPOLYGON ;

typedef struct {
    POINT	*pntbl ;		/* ���_�̈ʒu�f�[�^�e�[�u�� */
    Uint16	 nbPoint ;		/* ���_�̐� */
    Uint16	 nbNVector ;		/* �@���x�N�g���̐� */
    VECTOR	*nvtbl ;		/* �@���x�N�g���f�[�^�e�[�u�� */
    gxPOLYGON	*pltbl ;		/* �|���S����`�e�[�u�� */
    Uint32	 nbPolygon ;		/* �|���S���̐� */
    gxATTR_VDP1	*attbl ;		/* �|���S���̃A�g���r���[�g�e�[�u�� */
} gxPDATA ;

typedef struct {
    Uint32	 texadr ;		/* �e�N�X�`���f�[�^�e�[�u�� */
    Uint8	 BD ;			/* �e���_�̋P�x */
    Uint8	 BC ;
    Uint8	 BB ;
    Uint8	 BA ;
    Sint16	 XA ;			/* �e���_�̈ʒu */
    Sint16	 YA ;
    Sint16	 XB ;
    Sint16	 YB ;
    Sint16	 XC ;
    Sint16	 YC ;
    Sint16	 XD ;
    Sint16	 YD ;
    Uint16	 ZA ;			/* �e���_�̂y�l */
    Uint16	 ZB ;
    Uint16	 ZC ;
    Uint16	 ZD ;
    Uint16	 winno ;		/* �E�B���h�E�i���o�[ */
    Uint16	 ZF ;			/* ��\�y�l */
} gxDIRECT ;

#define		winEXn		0	/* �G���n���T�� ��O(near) */
#define		winEXf		1	/* �G���n���T�� ��(far) */
#define		winSTn		2	/* �r�s-�u�� ��O(near) */
#define		winSTf		3	/* �r�s-�u�� ��(far) */

#define	    gxPOLYTBL(f,s,t,m,n)	{((f)<<16)|((t)<<20)|((s)<<12)|((m)<<8)|(n)
#define	    gxNormal(a,b,c,d)		(a,b,c,d)
#define	    gxVertices(a,b,c,d)		(a,b,c,d)}
#define	    gxATTRVDP1(p,c,a,s)		{p,c,a,s}
#define	    gxATTRENHC(cx,cy,sx,sy,cm,cl,an,hf,ms,sp,te,fm)	{\
					((cx)<<4)|((cy)<<14)|(sx)|((sy)<<10)|(cm),\
					((ms)>>5)|(((sp)^SPdis)>>4)|((te)<<1)|\
					((an)<<8)|((hf)<<6)|(fm),(cl)
#endif

typedef struct obj{
    PDATA	*pat ;			/* �|���S�����f���f�[�^�e�[�u�� */
    FIXED	 pos[XYZ] ;		/* ���Έʒu�f�[�^ */
    ANGLE	 ang[XYZ] ;		/* ��]�p�f�[�^ */
    FIXED	 scl[XYZ] ;		/* �X�P�[�����O�f�[�^ */
    struct obj	*child ;		/* �q�I�u�W�F�N�g�f�[�^ */
    struct obj	*sibling ;		/* �Z��I�u�W�F�N�g�f�[�^ */
} OBJECT ;

typedef struct {
    Uint16	 Hsize ;		/* �X�v���C�g�̂g�T�C�Y */
    Uint16	 Vsize ;		/* �X�v���C�g�̂u�T�C�Y */
    Uint16	 CGadr ;		/* �X�v���C�g�b�f�A�h���X / �W */
    Uint16	 HVsize ;		/* �g�T�C�Y/�W�A�u�T�C�Y(�n�[�h�p) */
} TEXTURE ;

typedef struct {
    Uint16	 texno ;		/* �e�N�X�`���ԍ� */
    Uint16	 cmode ;		/* �J���[���[�h */
    void	*pcsrc ;		/* ���f�[�^�̏��� */
} PICTURE ;

#define	    COL_16	(2+1)		/* �J���[�o���N�P�U�F���[�h */
#define	    COL_64	(2+0)		/* �J���[�o���N�U�S�F���[�h */
#define	    COL_128	(2+0)		/* �J���[�o���N�P�Q�W�F���[�h */
#define	    COL_256	(2+0)		/* �J���[�o���N�Q�T�U�F���[�h */
#define	    COL_32K	(2-1)		/* �q�f�a�R�Q�j�F���[�h */

    enum base {
	SORT_BFR ,			/* ���O�ɕ\�������|���S���̈ʒu���g�� */
	SORT_MIN ,			/* �S�_�̓��A��Ԏ�O�̓_���g�� */
	SORT_MAX ,			/* �S�_�̓���ԉ����_���g�� */
	SORT_CEN			/* �S�_�̕��ψʒu���g�� */
    } ;

    enum pln {
	Single_Plane ,			/* �Жʃ|���S�� */
	Dual_Plane			/* ���ʃ|���S��(�\������̌��ʂ����Ȃ�) */
    } ;

typedef struct {
    Uint16	CTRL ;			/* �R���g���[���t�@���N�V���� */
    Uint16	LINK ;			/* �����N�A�h���X */
    Uint16	PMOD ;			/* �v�b�g���[�h */
    Uint16	COLR ;			/* �J���[�f�[�^ */
    Uint16	SRCA ;			/* �b�f�A�h���X */
    Uint16	SIZE ;			/* �L�����N�^�T�C�Y */
    Sint16	  XA ;			/* �\���ʒu �` */
    Sint16	  YA ;
    Sint16	  XB ;			/* �\���ʒu �a */
    Sint16	  YB ;
    Sint16	  XC ;			/* �\���ʒu �b */
    Sint16	  YC ;
    Sint16	  XD ;			/* �\���ʒu �c */
    Sint16	  YD ;
    Uint16	GRDA ;			/* �O�[���[�V�F�[�f�B���O�e�[�u�� */
    Uint16	DMMY ;			/* �T�C�Y���킹�p �_�~�[�f�[�^ */
} SPRITE ;

#define	    SpriteVRAM		0x25c00000

/*-----------------------------------------------------------------------*/

/* �q�f�a���[�h�J���[�T���v���f�[�^ */

#define	    RGB_Flag		0x8000

#define	    CD_Black		( 0 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_DarkRed		( 0 << 10) | ( 0 << 5) | (8)  | RGB_Flag
#define	    CD_DarkGreen	( 0 << 10) | ( 8 << 5) | (0)  | RGB_Flag
#define	    CD_DarkYellow	( 0 << 10) | ( 8 << 5) | (8)  | RGB_Flag
#define	    CD_DarkBlue		( 8 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_DarkPurple	( 8 << 10) | ( 0 << 5) | (8)  | RGB_Flag
#define	    CD_DarkMagenta	( 8 << 10) | ( 8 << 5) | (0)  | RGB_Flag
#define	    CD_DarkWhite	( 8 << 10) | ( 8 << 5) | (8)  | RGB_Flag
#define	    CD_MediumRed	( 0 << 10) | ( 0 << 5) | (16) | RGB_Flag
#define	    CD_MediumGreen	( 0 << 10) | (16 << 5) | (0)  | RGB_Flag
#define	    CD_MediumYellow	( 0 << 10) | (16 << 5) | (16) | RGB_Flag
#define	    CD_MediumBlue	(16 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_MediumPurple	(16 << 10) | ( 0 << 5) | (16) | RGB_Flag
#define	    CD_MediumMagenta	(16 << 10) | (16 << 5) | (0)  | RGB_Flag
#define	    CD_MediumWhite	(16 << 10) | (16 << 5) | (16) | RGB_Flag
#define	    CD_SemiRed		( 0 << 10) | ( 0 << 5) | (24) | RGB_Flag
#define	    CD_SemiGreen	( 0 << 10) | (24 << 5) | (0)  | RGB_Flag
#define	    CD_SemiYellow	( 0 << 10) | (24 << 5) | (24) | RGB_Flag
#define	    CD_SemiBlue		(24 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_SemiPurple	(24 << 10) | ( 0 << 5) | (24) | RGB_Flag
#define	    CD_SemiMagenta	(24 << 10) | (24 << 5) | (0)  | RGB_Flag
#define	    CD_SemiWhite	(24 << 10) | (24 << 5) | (24) | RGB_Flag
#define	    CD_Red		( 0 << 10) | ( 0 << 5) | (31) | RGB_Flag
#define	    CD_Green		( 0 << 10) | (31 << 5) | (0)  | RGB_Flag
#define	    CD_Yellow		( 0 << 10) | (31 << 5) | (31) | RGB_Flag
#define	    CD_Blue		(31 << 10) | ( 0 << 5) | (0)  | RGB_Flag
#define	    CD_Purple		(31 << 10) | ( 0 << 5) | (31) | RGB_Flag
#define	    CD_Magenta		(31 << 10) | (31 << 5) | (0)  | RGB_Flag
#define	    CD_White		(31 << 10) | (31 << 5) | (31) | RGB_Flag

/*------[ Scroll control ]------------------------------------------------*/

/*	Scroll V-RAM map address	*/

#define	    VDP2_VRAM_A0	0x25e00000
#define	    VDP2_VRAM_A1	0x25e20000
#define	    VDP2_VRAM_B0	0x25e40000
#define	    VDP2_VRAM_B1	0x25e60000

#define	    CGN01_RAM		VDP2_VRAM_B1		/* �X�N���[���O�A�P�p�b�f�f�[�^ */
#define	    NBG0_MAP		(CGN01_RAM+0x16000)	/* �X�N���[���O�̂u�q�`�l�A�h���X */
#define	    NBG1_MAP		(CGN01_RAM+0x18000)	/* �X�N���[���P�̂u�q�`�l�A�h���X */

#define	    KTBL0_RAM		VDP2_VRAM_A1		/* ��]�X�N���[���p�W���e�[�u�� */
#define	    BACK_CRAM		(KTBL0_RAM + 0x1fffe)	/* Back color */
#define     RBG_PARA_ADR	(KTBL0_RAM + 0x1ff00)
#define     RBG_PARB_ADR	(RBG_PARA_ADR+ 0x80)

#define	    CGR0_RAM		VDP2_VRAM_A0		/* ��]�X�N���[���p�b�f�f�[�^*/
#define	    RBG0_MAP		VDP2_VRAM_B0		/* ��]�X�N���[���O�̂u�q�`�l�A�h���X */
#define	    VDP2_COLRAM    		0x25f00000

#define	    CG_Size		(8*8)		/* �P�Z���̑傫��(CG) */
#define     CG16Size            (8*8/2)         /* �P�Z���̑傫��(CG 16Colors) */
#define     CG256Size           (8*8)           /* �P�Z���̑傫��(CG 256Colors) */
#define     CG2048Size          (8*8*2)         /* �P�Z���̑傫��(CG 2048Colors) */
#define     CG32KSize           (8*8*2)         /* �P�Z���̑傫��(CG 32KColors) */

typedef     Uint16		CELL ;		/* �P�Z���̑傫��(Pattern name) */
#define	    Line		64		/* �P���C���̑傫�� */
#define	    Page		(64*Line)	/* �P�y�[�W�̑傫�� */

typedef struct rdat{
    FIXED	XST ;		/* �X�N���[����ʃX�^�[�g���W �wst */
    FIXED	YST ;		/* �X�N���[����ʃX�^�[�g���W �xst */
    FIXED	ZST ;		/* �X�N���[����ʃX�^�[�g���W �yst */
    FIXED	DXST ;		/* �X�N���[����ʐ����������W���� ���wst */
    FIXED	DYST ;		/* �X�N���[����ʐ����������W���� ���xst */
    FIXED	DX ;		/* �X�N���[����ʐ����������W���� ���w */
    FIXED	DY ;		/* �X�N���[����ʐ����������W���� ���x */
    FIXED	MATA ;		/* ��]�}�g���N�X�p�����[�^�` */
    FIXED	MATB ;		/* ��]�}�g���N�X�p�����[�^�a */
    FIXED	MATC ;		/* ��]�}�g���N�X�p�����[�^�b */
    FIXED	MATD ;		/* ��]�}�g���N�X�p�����[�^�c */
    FIXED	MATE ;		/* ��]�}�g���N�X�p�����[�^�d */
    FIXED	MATF ;		/* ��]�}�g���N�X�p�����[�^�e */

    Sint16	PX ;		/* ���_���W �ox */
    Sint16	PY ;		/* ���_���W �oy */
    Sint16	PZ ;		/* ���_���W �oz */
    Sint16	dummy0 ;
    Sint16	CX ;		/* ���S���W �bx */
    Sint16	CY ;		/* ���S���W �by */
    Sint16	CZ ;		/* ���S���W �bz */
    Sint16	dummy1 ;

    FIXED	MX ;		/* ���s�ړ��� �lx */
    FIXED	MY ;		/* ���s�ړ��� �ly */
    FIXED	KX ;		/* �g��k���W�� kx */
    FIXED	KY ;		/* �g��k���W�� ky */

    Uint32	KAST ;		/* �W���e�[�u���X�^�[�g�A�h���X �j�`st */
    Sint32	DKAST ;		/* �W���e�[�u�����������A�h���X���� �j�`st */
    Sint32	DKA ;		/* �W���e�[�u�����������A�h���X���� �j�` */
} ROTSCROLL ;

/* slExtSignal */
#define		EXLTEN			0x0200		/* External latch enable */
#define		EXSYEN			0x0100		/* External synch enable */
#define		DASEL			0x0002		/* Display area select */
#define		EXBGEN			0x0001		/* External background screen enable */

/* slColRAMMode */
#define		CRM16_1024		0
#define		CRM16_2048		1
#define		CRM32_1024		2
/* slChar */
#define		COL_TYPE_16		0x00
#define		COL_TYPE_256		0x10
#define		COL_TYPE_2048		0x20
#define		COL_TYPE_32768		0x30
#define		COL_TYPE_1M		0x40
#define		CHAR_SIZE_1x1		0
#define		CHAR_SIZE_2x2		1
/* slPage */
#define		PNB_2WORD		0
#define		PNB_1WORD		0x8000
#define		CN_10BIT       		0
#define		CN_12BIT		0x4000
/* slPlane */
#define		PL_SIZE_1x1		0
#define		PL_SIZE_2x1		1
#define		PL_SIZE_2x2		3
/* slScrDisp */
#define		NBG0ON			(1<<0)
#define		NBG1ON			(1<<1)
#define		NBG2ON			(1<<2)
#define		NBG3ON			(1<<3)
#define		RBG0ON			(1<<4)
#define		LNCLON			(1<<5)
#define		SPRON			(1<<6)
#define		BACKON			(1<<5)
#define		NBG0OFF			(1<<16)
#define		NBG1OFF			(1<<17)
#define		NBG2OFF			(1<<18)
#define		NBG3OFF			(1<<19)
#define		RBG0OFF			(1<<20)
/* slZoomMode */
#define		ZOOM_1			0
#define		ZOOM_HALF	       	1
#define		ZOOM_QUARTER	       	2
/* slKtable */
#define		K_OFF		       	0
#define		K_ON		       	1
#define		K_2WORD		       	0
#define		K_1WORD		       	2
#define		K_MODE0		       	0
#define		K_MODE1			4
#define		K_MODE2			8
#define		K_MODE3			12
#define		K_LINECOL		0x10
#define		K_DOT			0x20
#define		K_LINE			0
#define		K_FIX			0x40
/* slCurRpara */
/* slRparaMode */
#define		RA			0
#define		RB			1
#define		K_CHANGE       		2
#define		W_CHANGE       		3

#define		LnRd_Xst		0x01
#define		LnRd_Yst		0x02
#define		LnRd_KAst		0x04

/* slColCalc */
#define		CC_RATE		       	0
#define		CC_ADD		       	0x100
#define		CC_TOP		       	0
#define		CC_2ND		       	0x200
#define		CC_EXT		       	0x400

/* Gradation control */
#define		grdNBG0			0xa000
#define		grdNBG1			0xc000
#define		grdNBG2			0xd000
#define		grdNBG3			0xe000
#define		grdRBG0			0x9000
#define		grdSPR			0x8000

/* color */
#define		COL_R			(1<<0)
#define		COL_G			(1<<5)
#define		COL_B			(1<<10)
/* SpriteColorCalc */
#define		CC_pr_CN		0	/* Priority <= CCalcNum */
#define		CC_PR_CN		1	/* Priority == CCalcNum */
#define		CC_PR_cn		2	/* Priority >= CCalcNum */
#define		CC_MSB			3	/* Use MSB */

#define		SPR_PAL			0	/* Palette code only */
#define		SPR_PAL_RGB		1	/* Use Palette and RGB */

    enum color_rate{
	CLRate31_1 ,		/* Top 31 : Second 1 */
	CLRate30_2 ,
	CLRate29_3 ,
	CLRate28_4 ,
	CLRate27_5 ,
	CLRate26_6 ,
	CLRate25_7 ,
	CLRate24_8 ,
	CLRate23_9 ,
	CLRate22_10 ,
	CLRate21_11 ,
	CLRate20_12 ,
	CLRate19_13 ,
	CLRate18_14 ,
	CLRate17_15 ,
	CLRate16_16 ,
	CLRate15_17 ,
	CLRate14_18 ,
	CLRate13_19 ,
	CLRate12_20 ,
	CLRate11_21 ,
	CLRate10_22 ,
	CLRate9_23 ,
	CLRate8_24 ,
	CLRate7_25 ,
	CLRate6_26 ,
	CLRate5_27 ,
	CLRate4_28 ,
	CLRate3_29 ,
	CLRate2_30 ,
	CLRate1_31 ,
	CLRate0_32		/* Top 0 : Second 32 */
    } ;

/* Screen select */
#define		scnNBG0			1
#define		scnNBG1			0
#define		scnNBG2			3
#define		scnNBG3			2
#define		scnRBG0			5
#define		scnRBGA			5
#define		scnRBGB			4
#define		scnSPR			4
#define		scnROT			7
#define		scnCCAL			6

#define		scnLNCL			7
#define		scnBACK			6

#define		scnSPR0			-7
#define		scnSPR1			-8
#define		scnSPR2			-5
#define		scnSPR3			-6
#define		scnSPR4			-3
#define		scnSPR5			-4
#define		scnSPR6			-1
#define		scnSPR7			-2

#define		bmNBG0			1
#define		bmNBG1			0
#define		bmRBG0			3

/* Window control */
#define		win_OR			0x80
#define		win_AND			0x00

#define		win0_IN			0x03
#define		win0_OUT		0x02

#define		win1_IN			0x0c
#define		win1_OUT		0x08

#define		spw_IN			0x30
#define		spw_OUT			0x20

/* Line Scroll control */
#define		lineSZ1			0x00
#define		lineSZ2	 		0x10
#define		lineSZ4			0x20
#define		lineSZ8			0x30

#define		lineZoom		0x08
#define		lineVScroll		0x04
#define		lineHScroll		0x02
#define		VCellScroll		0x01

/* BitMap control */
#define		BM_512x256		0x02
#define		BM_512x512		0x06
#define		BM_1024x256		0x0a
#define		BM_1024x512		0x0e

/* Special Function Control */
#define		spPRI_Scrn		0
#define		spPRI_Char		1
#define		spPRI_Dot		2

#define		spCCAL_Scrn		0
#define		spCCAL_Char		1
#define		spCCAL_Dot		2
#define		spCCAL_MSB		3

#define		sfCOL_01		0x01
#define		sfCOL_23		0x02
#define		sfCOL_45		0x04
#define		sfCOL_67		0x08
#define		sfCOL_89		0x10
#define		sfCOL_ab		0x20
#define		sfCOL_cd		0x40
#define		sfCOL_ef		0x80

/* V-RAM Division */
#define		Div_A			1
#define		Div_B			2

/*------[ D.M.A. control ]------------------------------------------------*/
#define		Sinc_Dinc_Byte		0
#define		Sdec_Dinc_Byte		1
#define		Sinc_Ddec_Byte		2
#define		Sfix_Dinc_Byte		3
#define		Sfix_Ddec_Byte		4
#define		Sinc_Dfix_Byte		5
#define		Sdec_Dfix_Byte		6

#define		Sinc_Dinc_Word		7
#define		Sdec_Dinc_Word		8
#define		Sinc_Ddec_Word		9
#define		Sfix_Dinc_Word		10
#define		Sfix_Ddec_Word		11
#define		Sinc_Dfix_Word		12
#define		Sdec_Dfix_Word		13

#define		Sinc_Dinc_Long		14
#define		Sdec_Dinc_Long		15
#define		Sinc_Ddec_Long		16
#define		Sfix_Dinc_Long		17
#define		Sfix_Ddec_Long		18
#define		Sinc_Dfix_Long		19
#define		Sdec_Dfix_Long		20

/*------[ Sound control ]-------------------------------------------------*/

typedef struct{
    Uint8	mode ;		/* Mode */
    Uint8	channel ;	/* PCM Channel Number */
    Uint8	level ;		/* 0 ~ 127 */
    Sint8	pan ;		/* -128 ~ +127 */
    Uint16	pitch ;
    Uint8	eflevelR ;	/* Effect level for Right(mono) 0 ~ 7 */
    Uint8	efselectR ;	/* Effect select for Right(mono) 0 ~ 15 */
    Uint8	eflevelL ;	/* Effect level for Left 0 ~ 7 */
    Uint8	efselectL ;	/* Effect select for Left 0 ~ 15 */
} PCM ;

#define		_Stereo		0x80
#define		_Mono		0x00

#define		_PCM16Bit	0x00
#define		_PCM8Bit	0x10

#define		SND_SEQ_START		0x01	/* Sequence Start */
#define		SND_SEQ_STOP		0x02	/* Sequence Stop */
#define		SND_SEQ_PAUSE		0x03	/* Sequence Pause */
#define		SND_SEQ_CONT		0x04	/* Sequence Continue */
#define		SND_SEQ_VOL		0x05	/* Sequence Volume */
#define		SND_TEMPO_CHG		0x07	/* Tempo Change */
#define		SND_MAP_CHG		0x08	/* map Change */
#define		SND_MIDI_CTRL_DIR	0x09	/* MIDI direct control */
#define		SND_VOL_ANL_START	0x0a	/* Volume analize start */
#define		SND_VOL_ANL_STOP	0x0b	/* Volume analize stop */
#define		SND_DSP_STOP		0x0c	/* DSP stop */
#define		SND_ALL_OFF		0x0d	/* Sound all OFF */
#define		SND_SEQ_PAN		0x0e	/* Sequence PAN */
#define		SND_CDDA_LEV		0x80	/* CD-DA Level */
#define		SND_CDDA_PAN		0x81	/* CD-DA pan */
#define		SND_TTL_VOL		0x82	/* Total Volume */
#define		SND_EFCT_CHG		0x83	/* Effect Change */
#define		SND_PCM_START		0x85	/* PCM start */
#define		SND_PCM_STOP		0x86	/* PCM stop */
#define		SND_MIX_CHG		0x87	/* Mixer change */
#define		SND_MIX_PRM_CHG		0x88	/* Mixer parameter change */
#define		SND_HARD_CHK		0x89	/* Hard check */
#define		SND_PCM_PRM_CHG		0x8a	/* PCM parameter change */

#define		SoundRAM		0x25a00000

/*------[ System Work Assignments ]---------------------------------------*/

    extern const void*   MasterStack ;	/* Stack area End */
    extern const Uint16  MaxVertices ;	/* Calculatable vertice numbers */
    extern const Uint16  MaxPolygons ;	/* Calculatable polygon numbers */
    extern const Uint16  EventSize ;	/* sizeof(EVENT) */
    extern const Uint16  WorkSize ;	/* sizeof(WORK) */
    extern const Uint16  MaxEvents ;	/* Useful Event count */
    extern const Uint16  MaxWorks ;	/* Useful Work count */
    extern const void*   SortList ;	/* SCU D.M.A. Table */
    extern const Uint32  SortListSize ;	/* Sprite transfer request table size */
    extern const void*   TransList ;	/* SCU D.M.A. Table */
    extern const void*   Zbuffer ;	/* (128 + 128 + 256) * 4 Bytes fix */
    extern const void*   SpriteBuf ;	/* Sprite control data buffer */
    extern const Uint32  SpriteBufSize ; /* Sprite control data buffer size */
    extern const void*   Pbuffer ;	/* Point calculate buffer */
    extern const void*   CLOfstBuf ;	/* 32 * 32 Bytes fix */
    extern const void*   CommandBuf ;	/* Command for slave CPU buffer */
    extern const void*   PCM_Work ;	/* PCM data buffer */
    extern const Uint32  PCM_WkSize ;	/* PCM data buffer size */

    extern EVENT  EventBuf[] ;		/* buffer for Event use */
    extern WORK   WorkBuf[] ;		/* buffer for Work use */
    extern EVENT* RemainEvent[] ;	/* Remain Event address buffer */
    extern WORK*  RemainWork[] ;	/* Remain Work address buffer */

/*------[ Prottype ]------------------------------------------------------*/

/*  Math library	*/

    extern  FIXED  slSin(ANGLE) ;
    extern  FIXED  slCos(ANGLE) ;
    extern  FIXED  slTan(ANGLE) ;
    extern  ANGLE  slAtan(FIXED , FIXED) ;
    extern  Uint32 slSquart(Uint32) ;
    extern  FIXED  slSquartFX(FIXED) ;
    extern  FIXED  slMulFX(FIXED , FIXED) ;
    extern  FIXED  slDivFX(FIXED , FIXED) ;
    extern  FIXED  slInnerProduct(VECTOR vct1 , VECTOR vct2) ;
    extern  FIXED  slCalcPoint(FIXED , FIXED , FIXED , FIXED ans[XYZ]) ;
    extern  FIXED  slCalcVector(VECTOR vct , VECTOR ans) ;
    extern  FIXED  slRandom(void) ;
    extern  void   slNormalVector(VECTOR , VECTOR , VECTOR , VECTOR) ;
    extern  void   slRevNormalVector(VECTOR , VECTOR , VECTOR , VECTOR) ;

    extern  void   slInitMatrix(void) ;
    extern  void   slUnitMatrix(MATRIX) ;
    extern  Bool   slPushMatrix(void) ;
    extern  Bool   slPopMatrix(void) ;
    extern  Bool   slPushUnitMatrix(void) ;
    extern  Bool   slIncMatrixPtr(void) ;
    extern  Bool   slDecMatrixPtr(void) ;
    extern  void   slLoadMatrix(MATRIX) ;
    extern  void   slGetMatrix(MATRIX) ;
    extern  Bool   slCopyMatrix() ;
    extern  void   slGetTranslate(FIXED*) ;
    extern  void   slLoadTranslate(FIXED , FIXED , FIXED) ;
    extern  void   slRegistMatrix(MATRIX) ;
    extern  void   slInversMatrix(void) ;
    extern  void   slTransposeMatrix(void) ;
    extern  void   slUnitAngle(MATRIX) ;
    extern  void   slUnitTranslate(MATRIX) ;

    extern  void   slTranslate(FIXED , FIXED , FIXED) ;
    extern  void   slRegistTranslate(FIXED , FIXED , FIXED) ;
    extern  void   slMultiMatrix(MATRIX) ;
    extern  void   slScale(FIXED , FIXED , FIXED) ;
    extern  void   slRotX(ANGLE) ;
    extern  void   slRotY(ANGLE) ;
    extern  void   slRotZ(ANGLE) ;
    extern  void   slRotXSC(FIXED , FIXED) ;
    extern  void   slRotYSC(FIXED , FIXED) ;
    extern  void   slRotZSC(FIXED , FIXED) ;
    extern  void   slRotAX(FIXED , FIXED , FIXED , ANGLE) ;
    extern  void   slLookAt(FIXED *camera , FIXED *target , ANGLE) ;
    extern  FIXED  slBallCollision(FIXED *pos1 , FIXED size1 , FIXED *pos2 , FIXED size2) ;
    extern  FIXED  slCheckOnScreen(FIXED *pos , FIXED size) ;
    extern  FIXED  slCheckOnScreen0(FIXED size) ;
    extern  void   slBezier(VECTOR p1,VECTOR p2,VECTOR p3,VECTOR p4,FIXED time,VECTOR ans) ;
    extern  FIXED  slConvert3Dto2D( FIXED pos[XYZ], Sint32 ans[XY]) ;
    extern  FIXED  slConvert3Dto2DFX( FIXED pos[XYZ], FIXED ans[XY]) ;

/*  Text display library	*/

    extern void   *slLocate(Uint16 , Uint16) ;
    extern void    slCurScrn(Uint16) ;
    extern void    slCurColor(Uint16) ;
    extern void    slDispHex(Uint32 , void *) ;
    extern void    slPrintHex(Uint32 , void *) ;
    extern void    slPrintMatrix(MATRIX , void *) ;
    extern void    slPrintFX(FIXED , void *) ;
    extern Uint32  slHex2Dec(Uint32) ;
    extern Uint32  slDec2Hex(Uint32) ;
    extern Uint16  slAng2Hex(ANGLE) ;
    extern Uint16  slAng2Dec(ANGLE) ;
    extern FIXED   slAng2FX(ANGLE) ;
    extern void    slPrint(char * , void *) ;
    extern void    set_vbar(Uint16) ;
    extern void    reset_vbar(Uint16) ;
    extern void    slExtendScroll(Uint8 * , void *) ;
    extern void   *slLocateNbg(Uint16 , Uint16 , Uint16) ;
#define		slLocateNbg0(x,y)	slLocateNbg(x,y,scnNBG0)
#define		slLocateNbg1(x,y)	slLocateNbg(x,y,scnNBG1)
#define		slLocateNbg2(x,y)	slLocateNbg(x,y,scnNBG2)
#define		slLocateNbg3(x,y)	slLocateNbg(x,y,scnNBG3)
    extern void   *slLocatePage(Uint16 , Uint16 , Uint16) ;

/*  Sprite library	*/

    extern  Bool   slPutPolygon(PDATA *) ;
    extern  Bool   slPutPolygonS(PDATA *) ;
    extern  Bool   slDispPolygon(PDATA * , Uint16) ;
    extern  Bool   slPutSprite(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSprite(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSpriteHV(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSpriteSZ(FIXED * , SPR_ATTR *attr , ANGLE) ;
    extern  Bool   slDispSprite4P(FIXED * , FIXED , SPR_ATTR *attr) ;
    extern  void   slPerspective(ANGLE) ;
    extern  void   slSetScreenDist(FIXED) ;
    extern  Bool   slWindow(Sint16 , Sint16 , Sint16 , Sint16 , Sint16 , Sint16 , Sint16) ;
    extern  void   slLight(VECTOR) ;
    extern  Bool   slSetSprite(SPRITE * , FIXED) ;
    extern  void   slZdspLevel(Uint16) ;
    extern  Bool   slPutObject(OBJECT *) ;
    extern  void   slCurWindow(Uint8) ;
    enum {
	winFar ,		/* Far side Window */
	winNear			/* Near side Window */
    } ;
    extern  Uint8  slSprWinNum() ;
    extern  void   slFrameClipSize(Sint16 , Sint16 );
    extern  void   slSetScreenDist(FIXED);
    extern  void   slGetFrameData(void * , Uint16 , Uint16 );

/*  Scroll library	*/

    extern  void    slTVOn(void) ;
    extern  void    slTVOff(void) ;
    extern  Uint16  slGetHCount(void) ;
    extern  Uint16  slGetVCount(void) ;
    extern  void    slExtSignal(Uint16 , Uint16) ;
    extern  void    slColRAMMode(Uint16) ;
    extern  void    slVRAMMode(Uint16) ;
    extern  void    slScrCycleSet(Uint32 , Uint32 , Uint32 , Uint32) ;
    extern  void    slScrDisp(Uint32) ;
    extern  Bool    slScrAutoDisp(Uint32) ;
    extern  void    slScrTransparent(Uint16) ;
    extern  void    slZrotR(ANGLE) ;
    extern  void    slScrMatSet(void) ;
    extern  void    slBackColTable(void *) ;
    extern  void    slBack1ColSet(void * , Uint16) ;
    extern  void    slLineColTable(void *) ;
    extern  void    slLine1ColSet(void * , Uint16) ;
    extern  void    slLineColDisp(Uint16) ;
    extern  void    slColorCalc(Uint16) ;
    extern  void    slColorCalcMode(Uint16) ;
    extern  void    slColorCalcOn(Uint16) ;
    extern  void    slColRate(Sint16 , Uint16) ;
#define		slColRateNbg0(rate)	slColRate(scnNBG0 , rate)
#define		slColRateNbg1(rate)	slColRate(scnNBG1 , rate)
#define		slColRateNbg2(rate)	slColRate(scnNBG2 , rate)
#define		slColRateNbg3(rate)	slColRate(scnNBG3 , rate)
#define		slColRateRbg0(rate)	slColRate(scnRBG0 , rate)
#define		slColRateLNCL(rate)	slColRate(scnLNCL , rate)
#define		slColRateBACK(rate)	slColRate(scnBACK , rate)
#define		slColRateSpr0(rate)	slColRate(scnSPR0 , rate)
#define		slColRateSpr1(rate)	slColRate(scnSPR1 , rate)
#define		slColRateSpr2(rate)	slColRate(scnSPR2 , rate)
#define		slColRateSpr3(rate)	slColRate(scnSPR3 , rate)
#define		slColRateSpr4(rate)	slColRate(scnSPR4 , rate)
#define		slColRateSpr5(rate)	slColRate(scnSPR5 , rate)
#define		slColRateSpr6(rate)	slColRate(scnSPR6 , rate)
#define		slColRateSpr7(rate)	slColRate(scnSPR7 , rate)
    extern  void    slColOffsetOn(Uint16) ;
    extern  void    slColOffsetOff(Uint16) ;
    extern  void    slColOffsetAUse(Uint16) ;
    extern  void    slColOffsetBUse(Uint16) ;
    extern  void    slColOffsetScrn(Uint16 , Uint16) ;
    extern  void    slColOffsetA(Sint16 r,Sint16 g,Sint16 b) ;
    extern  void    slColOffsetB(Sint16 r,Sint16 g,Sint16 b) ;
    extern  void    slCharNbg0(Uint16 type,Uint16 size) ;
    extern  void    slCharNbg1(Uint16 type,Uint16 size) ;
    extern  void    slCharNbg2(Uint16 type,Uint16 size) ;
    extern  void    slCharNbg3(Uint16 type,Uint16 size) ;
    extern  void    slCharRbg0(Uint16 type,Uint16 size) ;
    extern  void    slPageNbg0(void *,void *,Uint16) ;
    extern  void    slPageNbg1(void *,void *,Uint16) ;
    extern  void    slPageNbg2(void *,void *,Uint16) ;
    extern  void    slPageNbg3(void *,void *,Uint16) ;
    extern  void    slPageRbg0(void *,void *,Uint16) ;
    extern  void    slPlaneNbg0(Uint16) ;
    extern  void    slPlaneNbg1(Uint16) ;
    extern  void    slPlaneNbg2(Uint16) ;
    extern  void    slPlaneNbg3(Uint16) ;
    extern  void    slPlaneRA(Uint16) ;
    extern  void    slPlaneRB(Uint16) ;
    extern  void    slOverRA(Uint16) ;
    extern  void    slOverRB(Uint16) ;
    extern  void    slMapNbg0(void *,void *,void *,void *) ;
    extern  void    slMapNbg1(void *,void *,void *,void *) ;
    extern  void    slMapNbg2(void *,void *,void *,void *) ;
    extern  void    slMapNbg3(void *,void *,void *,void *) ;
    extern  void    sl1MapRA(void *) ;
    extern  void    sl1MapRB(void *) ;
    extern  void    sl16MapRA(Uint8 *) ;
    extern  void    sl16MapRB(Uint8 *) ;
    extern  void    slRparaInitSet(ROTSCROLL *) ;
    extern  void    slCurRpara(Uint16) ;
    extern  void    slRparaMode(Uint16) ;
    extern  void    slMakeKtable(void *) ;
    extern  void    slKtableRA(void *,Uint16) ;
    extern  void    slKtableRB(void *,Uint16) ;
    extern  void    slPriority(Sint16 , Uint16) ;
#define		slPriorityNbg0(num)	slPriority(scnNBG0 , num)
#define		slPriorityNbg1(num)	slPriority(scnNBG1 , num)
#define		slPriorityNbg2(num)	slPriority(scnNBG2 , num)
#define		slPriorityNbg3(num)	slPriority(scnNBG3 , num)
#define		slPriorityRbg0(num)	slPriority(scnRBG0 , num)
#define		slPrioritySpr0(num)	slPriority(scnSPR0 , num)
#define		slPrioritySpr1(num)	slPriority(scnSPR1 , num)
#define		slPrioritySpr2(num)	slPriority(scnSPR2 , num)
#define		slPrioritySpr3(num)	slPriority(scnSPR3 , num)
#define		slPrioritySpr4(num)	slPriority(scnSPR4 , num)
#define		slPrioritySpr5(num)	slPriority(scnSPR5 , num)
#define		slPrioritySpr6(num)	slPriority(scnSPR6 , num)
#define		slPrioritySpr7(num)	slPriority(scnSPR7 , num)
    extern  void    slScrPosNbg0(FIXED x,FIXED y) ;
    extern  void    slScrPosNbg1(FIXED x,FIXED y) ;
    extern  void    slScrPosNbg2(FIXED x,FIXED y) ;
    extern  void    slScrPosNbg3(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg0(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg1(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg2(FIXED x,FIXED y) ;
    extern  void    slScrMoveNbg3(FIXED x,FIXED y) ;
    extern  void    slLookR(FIXED x,FIXED y) ;
    extern  void    slDispCenterR(FIXED x,FIXED y) ;
    extern  void    slZoomNbg0(FIXED x,FIXED y) ;
    extern  void    slZoomNbg1(FIXED x,FIXED y) ;
    extern  void    slZoomR(FIXED x,FIXED y) ;
    extern  void    slScrScaleNbg0(FIXED x,FIXED y) ;
    extern  void    slScrScaleNbg1(FIXED x,FIXED y) ;
    extern  void    slScrScaleR(FIXED x,FIXED y) ;
    extern  void    slZoomMode(Uint16 , Uint16) ;
#define		slZoomModeNbg0(mode)	slZoomMode(scnNBG0 , mode)
#define		slZoomModeNbg1(mode)	slZoomMode(scnNBG1 , mode)
    extern  void    slScrAscSet(void *) ;
    extern  void    slScrMatConv(void) ;
    extern  void    slSpriteCCalcCond(Uint16) ;
    extern  void    slSpriteCCalcNum(Uint16) ;
    extern  void    slSpriteColMode(Uint16) ;
    extern  void    slSpriteWinMode(Uint16) ;
    extern  void    slSpriteType(Uint16) ;
    extern  void    slScrMosaicOn(Uint16) ;
    extern  void    slScrMosSize(Uint16 , Uint16) ;
    extern  void    slScrWindow0(Uint16 , Uint16 , Uint16 , Uint16) ;
    extern  void    slScrWindow1(Uint16 , Uint16 , Uint16 , Uint16) ;
    extern  void    slScrLineWindow0(void *) ;
    extern  void    slScrLineWindow1(void *) ;
    extern  void    slScrWindowMode(Uint16 , Uint16) ;
#define		slScrWindowModeNbg0(mode)	slScrWindowMode(scnNBG0 , mode)
#define		slScrWindowModeNbg1(mode)	slScrWindowMode(scnNBG1 , mode)
#define		slScrWindowModeNbg2(mode)	slScrWindowMode(scnNBG2 , mode)
#define		slScrWindowModeNbg3(mode)	slScrWindowMode(scnNBG3 , mode)
#define		slScrWindowModeRbg0(mode)	slScrWindowMode(scnRBG0 , mode)
#define		slScrWindowModeSPR(mode)	slScrWindowMode(scnSPR  , mode)
#define		slScrWindowModeROT(mode)	slScrWindowMode(scnROT  , mode)
#define		slScrWindowModeCCAL(mode)	slScrWindowMode(scnCCAL , mode)
    extern  void    slLineScrollMode(Uint16 , Uint16) ;
#define		slLineScrollModeNbg0(mode)	slLineScrollMode(scnNBG0 , mode)
#define		slLineScrollModeNbg1(mode)	slLineScrollMode(scnNBG1 , mode)
    extern  void    slLineScrollTable0(void *) ;
    extern  void    slLineScrollTable1(void *) ;
    extern  void    slVCellTable(void *) ;
    extern  void    slGradationOn(Uint16) ;
#define		slGradationOff()	slGradationOn(OFF)
    extern  void    slShadowOn(Uint16) ;
    extern  void    slTpShadowMode(Uint16) ;
    extern  void    slOverPatRA(Uint16 pat) ;
    extern  void    slOverPatRB(Uint16 pat) ;
    extern  void    slColRAMOffsetNbg0(Uint16 offset) ;
    extern  void    slColRAMOffsetNbg1(Uint16 offset) ;
    extern  void    slColRAMOffsetNbg2(Uint16 offset) ;
    extern  void    slColRAMOffsetNbg3(Uint16 offset) ;
    extern  void    slColRAMOffsetRbg0(Uint16 offset) ;
    extern  void    slColRAMOffsetSpr(Uint16 offset) ;
    extern  void    slBitMapNbg0(Uint16 , Uint16 , void *) ;
    extern  void    slBitMapNbg1(Uint16 , Uint16 , void *) ;
    extern  void    slBitMapRbg0(Uint16 , Uint16 , void *) ;
    extern  void    slBMPalette(Uint16 , Uint16) ;
#define		slBMPaletteNbg0(pal)	slBMPalette(bmNBG0 , pal)
#define		slBMPaletteNbg1(pal)	slBMPalette(bmNBG1 , pal)
#define		slBMPaletteRbg0(pal)	slBMPalette(bmRBG0 , pal)
    extern  void    slSpecialPrioModeNbg0(Uint16) ;
    extern  void    slSpecialPrioModeNbg1(Uint16) ;
    extern  void    slSpecialPrioModeNbg2(Uint16) ;
    extern  void    slSpecialPrioModeNbg3(Uint16) ;
    extern  void    slSpecialPrioModeRbg0(Uint16) ;
    extern  void    slSpecialCCalcModeNbg0(Uint16) ;
    extern  void    slSpecialCCalcModeNbg1(Uint16) ;
    extern  void    slSpecialCCalcModeNbg2(Uint16) ;
    extern  void    slSpecialCCalcModeNbg3(Uint16) ;
    extern  void    slSpecialCCalcModeRbg0(Uint16) ;
    extern  void    slSpecialFuncCodeA(Uint16) ;
    extern  void    slSpecialFuncCodeB(Uint16) ;
    extern  void    slSpecialFuncSelectB(Uint16) ;
    extern  void    slRparaReadCtrlRA(Uint16) ;
    extern  void    slRparaReadCtrlRB(Uint16) ;
    extern  void    slSpecialPrioBitN01(Uint16 , Uint16) ;
    extern  void    slSpecialPrioBitScr(Uint16 , Uint16) ;
    extern  void    slSpecialPrioBitRbg0(Uint16) ;
#define		slSpecialPrioBitNbg0(flag)	slSpecialPrioBitN01(scnNBG0 , flag)
#define		slSpecialPrioBitNbg1(flag)	slSpecialPrioBitN01(scnNBG1 , flag)
#define		slSpecialPrioBitNbg2(flag)	slSpecialPrioBitScr(scnNBG2 , flag)
#define		slSpecialPrioBitNbg3(flag)	slSpecialPrioBitScr(scnNBG3 , flag)
#define		slSpecialPrioBitBMN0(flag)	slSpecialPrioBitN01(scnNBG0 , flag)
#define		slSpecialPrioBitBMN1(flag)	slSpecialPrioBitN01(scnNBG1 , flag)
#define		slSpecialPrioBitBMR0(flag)	slSpecialPrioBitRbg0(flag)
    extern  void    slSpecialCCalcBitN01(Uint16 , Uint16) ;
    extern  void    slSpecialCCalcBitScr(Uint16 , Uint16) ;
    extern  void    slSpecialCCalcBitRbg0(Uint16) ;
#define		slSpecialCCalcBitNbg0(flag)	slSpecialCCalcBitN01(scnNBG0 , flag)
#define		slSpecialCCalcBitNbg1(flag)	slSpecialCCalcBitN01(scnNBG1 , flag)
#define		slSpecialCCalcBitNbg2(flag)	slSpecialCCalcBitScr(scnNBG2 , flag)
#define		slSpecialCCalcBitNbg3(flag)	slSpecialCCalcBitScr(scnNBG3 , flag)
#define		slSpecialCCalcBitBMN0(flag)	slSpecialCCalcBitN01(scnNBG0 , flag)
#define		slSpecialCCalcBitBMN1(flag)	slSpecialCCalcBitN01(scnNBG1 , flag)
#define		slSpecialCCalcBitBMR0(flag)	slSpecialCCalcBitRbg0(flag)
    extern  void    setASC_1to8(Uint8 * , void * , Uint32 , Uint8) ;
    extern  void    setASC_4to8(Uint8 * , void * , Uint32 , Uint8) ;
    extern  void    setASC_1to4(Uint8 * , void * , Uint32 , Uint8 , Sint32) ;
    extern  Uint8   SGL_ASCII_CG[] ;

/*  BitMap graphics library */

    extern  Bool    slInitBitMap(Uint16 , Uint16 , void*) ;
#define         slInitBitMapNbg0(sz,adr)        slInitBitMap(bmNBG0,sz,adr)
#define         slInitBitMapNbg1(sz,adr)        slInitBitMap(bmNBG1,sz,adr)
#define         slInitBitMapRbg0(sz,adr)        slInitBitMap(bmRBG0,sz,adr)
    extern  void    slBitMapBase(Sint16 , Sint16) ;
    extern  Sint8  *slLocateBitMap(Sint16 , Sint16) ;
    extern  void    slClearBitMap(Sint8) ;
    extern  Bool    slBMPset(Sint16 , Sint16 , Sint8) ;
    extern  Sint8   slBMPoint(Sint16 , Sint16) ;
    extern  void    slBMCircle(Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMLine(Sint16 , Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMBox(Sint16 , Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMBoxFill(Sint16 , Sint16 , Sint16 , Sint16 , Sint8) ;
    extern  Bool    slBMGet(Sint16 , Sint16 , Sint16 , Sint16 , Sint8*) ;
    extern  Bool    slBMPut(Sint16 , Sint16 , Sint16 , Sint16 , Sint8*) ;
    extern  Bool    slBMSprPut(Sint16 , Sint16 , Sint16 , Sint16 , Sint8*) ;

#ifdef ST_VE
/*  Graphics enhancer control */

    extern  Bool gxInitEnhancer(Uint16 , Uint16 , Uint16) ;
#define	    EGX_IMMPAL		0	/* �p���b�g���� */
#define	    EGX_PAL15		1	/* �p���b�g�Ԑ� �P�T�r�b�g�J���[ */
#define	    EGX_PAL24		2	/* �p���b�g�Ԑ� �Q�S�r�b�g�J���[ */
#define	    EGX_PAL_RGB15	3	/* �p���b�g�A�q�f�a���� �P�T�r�b�g�J���[ */
#define	    EGX_PAL_RGB24	4	/* �p���b�g�A�q�f�a���� �Q�S�r�b�g�J���[ */

#define	    USE_EXVDP1		0
#define	    USE_DSPDMA		1

    extern  void gxFocus(FIXED , FIXED) ;
    extern  void gxCalcMode(Uint16) ;
#define	    CALCNVECTOR		1	/* �@���Čv�Z */
#define	    USENVECTOR		0	/* �@���g�p */

#define	    NOSHADE		2	/* �V�F�[�f�B���O�Ȃ� */
#define	    FLAT		4	/* �t���b�g�V�F�[�f�B���O */
#define	    GOURAUD		8	/* �O�[���[�V�F�[�f�B���O */

    extern  void gxSetMaterial(Uint16 , Uint16 , Uint16 , Uint16 , Uint16) ;

#define	    EFUNC_LoadTrans	0x0c0c0c0c
#define	    EFUNC_FlashMatrix	0x10101010
#define	    EFUNC_BaseMatrix	0x11111111
#define	    EFUNC_PushMatrix	0x12121212
#define	    EFUNC_PopMatrix	0x13131313
#define	    EFUNE_Translate	0x14141414
#define	    EFUNE_Scale		0x15151515
#define	    EFUNE_RotX		0x16161616
#define	    EFUNE_RotY		0x17171717
#define	    EFUNE_RotZ		0x18181818
#define	    EFUNE_MultiMatrix	0x19191919
#define	    EFUNC_RotLight	0x1a1a1a1a

    extern  void gxFuncOut(Uint32) ;
    extern  void gxFuncOut2(Uint32 , Uint32) ;
    extern  void gxFuncOut3(Uint32 , Uint32 , Uint32 , Uint32) ;
#define	    gxFlashMatrix()	gxFuncOut(EFUNC_FlashMatrix)
#define	    gxBaseMatrix()	gxFuncOut(EFUNC_BaseMatrix)
#define	    gxPushMatrix()	gxFuncOut(EFUNC_PushMatrix)
#define	    gxPopMatrix()	gxFuncOut(EFUNC_PopMatrix)
#define	    gxRotLight()	gxFuncOut(EFUNC_RotLight)

#define	    gxLoadTrans(x,y,z)	gxFuncOut3(EFUNC_LoadTrans , x , y , z)
#define	    gxTranslate(x,y,z)	gxFuncOut3(EFUNC_Translate , x , y , z)
#define	    gxScale(x,y,z)	gxFuncOut3(EFUNC_Scale , x , y , z)
#define	    gxRotX(ax)		gxFuncOut2(EFUNC_RotX , ax)
#define	    gxRotY(ay)		gxFuncOut2(EFUNC_RotY , ay)
#define	    gxRotZ(az)		gxFuncOut2(EFUNC_RotZ , az)

    extern  void gxMatrixOut(Uint32 , MATRIX) ;
#define	    gxLoadMatrix(mtrx)	gxMatrixOut(EFUNC_LoadMatrix , mtrx)
#define	    gxMultiMatrix(mtrx)	gxMatrixOut(EFUNC_MultiMatrix , mtrx)

    extern  void gxPutPolygon(gxPDATA*) ;
#endif

/*  Peripheral control */

#define	SMPC_MASK_STE		0x80	/* �r�d�s�s�h�l�d���s�σ}�X�N		*/
#define	SMPC_MASK_RESD		0x40	/* ���Z�b�g�C�l�[�u���}�X�N		*/
#define	SMPC_MASK_WEEK		0xf0	/* �q�s�b�j���}�X�N			*/
#define	SMPC_MASK_MONTH		0x0f	/* �q�s�b���}�X�N			*/
#define	SMPC_MASK_CTG1		0x02	/* �J�[�g���b�W�R�[�h�P			*/
#define	SMPC_MASK_CTG0		0x01	/* �J�[�g���b�W�R�[�h�O			*/
#define	SMPC_MASK_DOTSEL	0x4000	/* �c�n�s�r�d�k�M��			*/
#define	SMPC_MASK_MSHNMI	0x0800	/* �l�r�g�m�l�h�M��			*/
#define	SMPC_MASK_SYSRES	0x0200	/* �r�x�r�q�d�r�M��			*/
#define	SMPC_MASK_SNDRES	0x0100	/* �r�m�c�q�d�r�M��			*/
#define	SMPC_MASK_CDRES		0x0040	/* �b�c�q�d�r�M��			*/

#define	SMPC_SUN	(0 << 4)	/* �q�s�b�j���ԍ��i���j���j		*/
#define	SMPC_MON	(1 << 4)	/* �q�s�b�j���ԍ��i���j���j		*/
#define	SMPC_TUE	(2 << 4)	/* �q�s�b�j���ԍ��i�Ηj���j		*/
#define	SMPC_WED	(3 << 4)	/* �q�s�b�j���ԍ��i���j���j		*/
#define	SMPC_THU	(4 << 4)	/* �q�s�b�j���ԍ��i�ؗj���j		*/
#define	SMPC_FRI	(5 << 4)	/* �q�s�b�j���ԍ��i���j���j		*/
#define	SMPC_SAT	(6 << 4)	/* �q�s�b�j���ԍ��i�y�j���j		*/

#define	PER_RESULT_MAX		30	/* �ő�y���t�F������			*/

#define	PER_ID_StnPad		0x02	/* �T�^�[���W���p�b�h			*/
#define	PER_ID_StnLacing	0x13	/* �T�^�[�����[�V���O�R���g���[���[	*/
#define	PER_ID_StnAnalog	0x15	/* �T�^�[���A�i���O�W���C�X�e�B�b�N	*/
#define	PER_ID_StnMouse		0x23	/* �T�^�[���}�E�X			*/
#define	PER_ID_StnShooting	0x25	/* �T�^�[���V���[�e�B���O�R���g���[���[ */
#define	PER_ID_StnKeyBoard	0x34	/* �T�^�[���L�[�{�[�h			*/
#define	PER_ID_MD3BtnPad	0xe1	/* ���K�h���C�u�R�{�^���p�b�h		*/
#define	PER_ID_MD6BtnPad	0xe2	/* ���K�h���C�u�U�{�^���p�b�h		*/
#define	PER_ID_ShuttleMouse	0xe3	/* �V���g���}�E�X			*/
#define	PER_ID_ExtDigital	0x00	/* �g���T�C�Y�f�W�^���f�o�C�X		*/
#define	PER_ID_ExtAnalog	0x10	/* �g���T�C�Y�A�i���O�f�o�C�X		*/
#define	PER_ID_ExtPointing	0x20	/* �g���T�C�Y�|�C���e�B���O�f�o�C�X	*/
#define	PER_ID_ExtKeyBoard	0x30	/* �g���T�C�Y�L�[�{�[�h�f�o�C�X		*/
#define	PER_ID_ExtMegaDrive	0xe0	/* �g���T�C�Y���K�h���C�u�f�o�C�X	*/
#define	PER_ID_NotConnect	0xff	/* ���ڑ�				*/

					/* �f�W�^���f�o�C�X			*/
#define	PER_DGT_KR	(1 << 15)	/* Right Key				*/
#define	PER_DGT_KL	(1 << 14)	/*  Left Key				*/
#define	PER_DGT_KD	(1 << 13)	/*  Down Key				*/
#define	PER_DGT_KU	(1 << 12)	/*    Up Key				*/
#define	PER_DGT_ST	(1 << 11)	/* Start				*/
#define	PER_DGT_TA	(1 << 10)	/* A Trigger				*/
#define	PER_DGT_TC	(1 <<  9)	/* C Trigger				*/
#define	PER_DGT_TB	(1 <<  8)	/* B Trigger				*/
#define	PER_DGT_TR	(1 <<  7)	/* R Trigger				*/
#define	PER_DGT_TX	(1 <<  6)	/* X Trigger				*/
#define	PER_DGT_TY	(1 <<  5)	/* Y Trigger				*/
#define	PER_DGT_TZ	(1 <<  4)	/* Z Trigger				*/
#define	PER_DGT_TL	(1 <<  3)	/* L Trigger				*/

					/* �L�[�{�[�h�f�o�C�X			*/
#define	PER_KBD_CL	(1 << 6)	/* Caps Lock				*/
#define	PER_KBD_NL	(1 << 5)	/* Num Lock				*/
#define	PER_KBD_SL	(1 << 4)	/* Scroll Lock				*/
#define	PER_KBD_MK	(1 << 3)	/* Make					*/
#define	PER_KBD_BR	(1 << 0)	/* Break				*/

enum SmpcSelect {			/* �r�l�o�b���o�̓Z���N�g���[�h		*/
	SMPC_SH2_DIRECT,		/* (00) �r�g�Q�_�C���N�g		*/
	SMPC_CONTROL			/* (01) �r�l�o�b�R���g���[��		*/
};

enum SmpcExtLatch {			/* �r�l�o�b�O�����b�`���̓��[�h		*/
	SMPC_EXL_ENA,			/* (00) �O�����b�`���͋���		*/
	SMPC_EXL_DIS			/* (01) �O�����b�`���͋֎~		*/
};

enum SmpcResetButton {			/* �r�l�o�b���Z�b�g�{�^�����		*/
	SMPC_RES_ON,			/* (00) ���Z�b�g�{�^���n�m		*/
	SMPC_RES_OFF			/* (01) ���Z�b�g�{�^���n�e�e		*/
};

enum SmpcAreaCode {			/* �G���A�R�[�h				*/
	SMPC_AREA_RSV_0,		/* (00) �g�p�֎~			*/
	SMPC_AREA_JP,			/* (01) ���{�n��			*/
	SMPC_AREA_AJ_NTSC,		/* (02) �A�W�A�m�s�r�b�n��		*/
	SMPC_AREA_RSV_3,		/* (03) �g�p�֎~			*/
	SMPC_AREA_NA,			/* (04) �k�Ēn��			*/
	SMPC_AREA_SA_NTSC,		/* (05) ����Ăm�s�r�b�n��		*/
	SMPC_AREA_KO,			/* (06) �؍��n��			*/
	SMPC_AREA_RSV_7,		/* (07) �g�p�֎~			*/
	SMPC_AREA_RSV_8,		/* (08) �g�p�֎~			*/
	SMPC_AREA_RSV_9,		/* (09) �g�p�֎~			*/
	SMPC_AREA_AJ_PAL,		/* (0a) �A�W�A�o�`�k�n��		*/
	SMPC_AREA_RSV_B,		/* (0b) �g�p�֎~			*/
	SMPC_AREA_EU_PAL,		/* (0c) ���[���b�p�o�`�k�n��		*/
	SMPC_AREA_SA_PAL,		/* (0d) ����Ăo�`�k�n��		*/
	SMPC_AREA_RSV_E,		/* (0e) �g�p�֎~			*/
	SMPC_AREA_RSV_F			/* (0f) �g�p�֎~			*/
};

enum SmpcOptimize {			/* �擾���ԍœK�����[�h			*/
	SMPC_OPT_DIS,			/* (00) �擾���ԍœK���֎~		*/
	SMPC_OPT_ENA			/* (01) �擾���ԍœK������		*/
};

enum SmpcPortMode {			/* �r�l�o�b�|�[�g���[�h			*/
	SMPC_PORT_15,			/* (00) �P�T�o�C�g���[�h		*/
	SMPC_PORT_255,			/* (01) �Q�T�T�o�C�g���[�h		*/
	SMPC_PORT_RSV,			/* (02) �ݒ�֎~			*/
	SMPC_PORT_ZERO			/* (03) �O�o�C�g���[�h			*/
};

enum SmpcLanguage {			/* �r�l�o�b����������ԍ�		*/
	SMPC_ENGLISH,			/* (00) �p��				*/
	SMPC_DEUTSCH,			/* (01) �h�C�c��			*/
	SMPC_FRANCAIS,			/* (02) �t�����X��			*/
	SMPC_ESPANOL,			/* (03) �X�y�C����			*/
	SMPC_ITALIANO,			/* (04) �C�^���A��			*/
	SMPC_JAPAN			/* (05) ���{��				*/
};

enum SmpcSoundEffect {			/* �r�l�o�b���������ʉ����[�h		*/
	SMPC_EFFECT_OFF,		/* (00) ���ʉ��n�e�e			*/
	SMPC_EFFECT_ON			/* (01) ���ʉ��n�m			*/
};

enum SmpcSoundOutput {			/* �r�l�o�b�����������o�̓��[�h		*/
	SMPC_SOUND_MONO,		/* (00) ���m����			*/
	SMPC_SOUND_STEREO		/* (01) �X�e���I			*/
};

enum SmpcHelpWindow {			/* �r�l�o�b�������w���v�E�B���h�E���[�h	*/
	SMPC_HELP_DIS,			/* (00) �\�����Ȃ�			*/
	SMPC_HELP_ENA			/* (01) �\������			*/
};

enum SmpcMonth {			/* �q�s�b���ԍ�				*/
	SMPC_DMY,			/* (00) �_�~�[				*/
	SMPC_JAN,			/* (01) �@�P��				*/
	SMPC_FEB,			/* (02) �@�Q��				*/
	SMPC_MAR,			/* (03) �@�R��				*/
	SMPC_APR,			/* (04) �@�S��				*/
	SMPC_MAY,			/* (05) �@�T��				*/
	SMPC_JUN,			/* (06) �@�U��				*/
	SMPC_JUL,			/* (07) �@�V��				*/
	SMPC_AUG,			/* (08) �@�W��				*/
	SMPC_SEP,			/* (09) �@�X��				*/
	SMPC_OCT,			/* (0a) �P�O��				*/
	SMPC_NOV,			/* (0b) �P�P��				*/
	SMPC_DEC			/* (0c) �P�Q��				*/
};

enum SmpcCommand {			/* �r�l�o�b�R�}���h�ԍ�			*/
	SMPC_MSHON,			/* (00) �}�X�^�@�r�g�Q�@�n�m		*/
	SMPC_SSHON,			/* (01) �X���[�u�@�r�g�Q�@�n�m		*/
	SMPC_SSHOFF,			/* (02) �X���[�u�@�r�g�Q�@�n�e�e	*/
	SMPC_SNDON,			/* (03) �T�E���h�@�n�m			*/
	SMPC_SNDOFF,			/* (04) �T�E���h�@�n�e�e		*/
	SMPC_CDON,			/* (05) �b�c�@�n�m			*/
	SMPC_CDOFF,			/* (06) �b�c�@�n�e�e			*/
	SMPC_SYSRES,			/* (07) �V�X�e���S�̃��Z�b�g		*/
	SMPC_CKC352,			/* (08) �N���b�N�`�F���W�R�T�Q���[�h	*/
	SMPC_CKC320,			/* (09) �N���b�N�`�F���W�R�Q�O���[�h	*/
	SMPC_NMIREQ,			/* (0a) �m�l�h���N�G�X�g		*/
	SMPC_RESENA,			/* (0b) ���Z�b�g�C�l�[�u��		*/
	SMPC_RESDIS,			/* (0c) ���Z�b�g�f�B�X�G�[�u��		*/
	SMPC_GETSTS,			/* (0d) �X�e�[�^�X�擾			*/
	SMPC_GETPER,			/* (0e) �y���t�F�����擾		*/
	SMPC_SETMEM,			/* (0f) �r�l�o�b�������ݒ�		*/
	SMPC_SETTIM,			/* (10) �����ݒ�			*/
	SMPC_CMDMAX			/* (11) �R�}���h�ԍ��ő�l		*/
};

enum SmpcWaitMode {			/* �r�l�o�b�R�}���h���s���[�h		*/
	SMPC_NO_WAIT,			/* (00) �R�}���h���s�I���҂�����	*/
	SMPC_WAIT			/* (01) �R�}���h���s�I���҂��L��	*/
};

typedef struct {			/* �q�s�b����				*/
	Uint16	year;			/* ����N��				*/
	Uint8	month;			/* �j���E��				*/
	Uint8	date;			/* ��					*/
	Uint8	hour;			/* ��					*/
	Uint8	minute;			/* ��					*/
	Uint8	second;			/* �b					*/
	Uint8	dummy;			/* �_�~�[				*/
} SmpcDateTime;

typedef struct {			/* �r�l�o�b�X�e�[�^�X			*/
	Uint8	cond;			/* ��ԃX�e�[�^�X			*/
	Uint8	dummy1;			/* �_�~�[�P				*/
	Uint16	dummy2;			/* �_�~�[�Q				*/
	SmpcDateTime	rtc;		/* �q�s�b����				*/
	Uint8	ctg;			/* �J�[�g���b�W�R�[�h			*/
	Uint8	area;			/* �G���A�R�[�h				*/
	Uint16	system;			/* �V�X�e���X�e�[�^�X			*/
	Uint32	smem;			/* �r�l�o�b�������ێ��f�[�^		*/
} SmpcStatus;

typedef struct {			/* �f�W�^���f�o�C�X			*/
	Uint8	id;			/* �y���t�F�����h�c			*/
	Uint8	ext;			/* �g���f�[�^�T�C�Y			*/
	Uint16	data;			/* �{�^�����݃f�[�^			*/
	Uint16	push;			/* �{�^�������f�[�^			*/
	Uint16	pull;			/* �{�^������f�[�^			*/
	Uint32	dummy2[4];		/* �_�~�[�Q				*/
} PerDigital;

typedef struct {			/* �A�i���O�f�o�C�X			*/
	Uint8	id;			/* �y���t�F�����h�c			*/
	Uint8	ext;			/* �g���f�[�^�T�C�Y			*/
	Uint16	data;			/* �{�^�����݃f�[�^			*/
	Uint16	push;			/* �{�^�������f�[�^			*/
	Uint16	pull;			/* �{�^������f�[�^			*/
	Uint8	x;			/* �w���f�[�^��Βl			*/
	Uint8	y;			/* �x���f�[�^��Βl			*/
	Uint8	z;			/* �y���f�[�^��Βl			*/
	Uint8	dummy1;			/* �_�~�[�P				*/
	Uint32	dummy2[3];		/* �_�~�[�Q				*/
} PerAnalog;

typedef struct {			/* �|�C���e�B���O�f�o�C�X		*/
	Uint8	id;			/* �y���t�F�����h�c			*/
	Uint8	ext;			/* �g���f�[�^�T�C�Y			*/
	Uint16	data;			/* �{�^�����݃f�[�^			*/
	Uint16	push;			/* �{�^�������f�[�^			*/
	Uint16	pull;			/* �{�^������f�[�^			*/
	Uint16	x;			/* �w���W				*/
	Uint16	y;			/* �x���W				*/
	Uint32	dummy2[3];		/* �_�~�[�Q				*/
} PerPoint;

typedef struct {			/* �L�[�{�[�h�f�o�C�X			*/
	Uint8	id;			/* �y���t�F�����h�c			*/
	Uint8	ext;			/* �g���f�[�^�T�C�Y			*/
	Uint16	data;			/* �{�^�����݃f�[�^			*/
	Uint16	push;			/* �{�^�������f�[�^			*/
	Uint16	pull;			/* �{�^������f�[�^			*/
	Uint8	cond;			/* ��ԃf�[�^				*/
	Uint8	code;			/* �L�[�R�[�h				*/
	Uint16	dummy1;			/* �_�~�[�P				*/
	Uint32	dummy2[3];		/* �_�~�[�Q				*/
} PerKeyBoard;

extern	Uint8	slGetPortDir1(void);
extern	Uint8	slGetPortDir2(void);
extern	Bool	slSetPortDir1(Uint8);
extern	Bool	slSetPortDir2(Uint8);

extern	Uint8	slGetPortData1(void);
extern	Uint8	slGetPortData2(void);
extern	void	slSetPortData1(Uint8);
extern	void	slSetPortData2(Uint8);

extern	Uint8	slGetPortSelect1(void);
extern	Uint8	slGetPortSelect2(void);
extern	Bool	slSetPortSelect1(Uint8);
extern	Bool	slSetPortSelect2(Uint8);

extern	Uint8	slGetPortExt1(void);
extern	Uint8	slGetPortExt2(void);
extern	Bool	slSetPortExt1(Uint8);
extern	Bool	slSetPortExt2(Uint8);

extern	Uint8	slGetLanguage(void);
extern	void	slSetLanguage(Uint8);

extern	Uint8	slGetSoundEffect(void);
extern	void	slSetSoundEffect(Uint8);

extern	Uint8	slGetSoundOutput(void);
extern	void	slSetSoundOutput(Uint8);

extern	Uint8	slGetHelpWindow(void);
extern	void	slSetHelpWindow(Uint8);

extern	void	slInitPeripheral(void);
extern	Bool	slSetResultBuffer(Uint8*, PerDigital*, Uint16);

extern	Bool	slIntBackCancel(void);
extern	Uint8	slCheckReset(void);
extern	Bool	slClearReset(void);

extern	Uint8	slGetOptimize(void);
extern	Bool	slSetOptimize(Uint8);

extern	Uint8	slGetPortMode1(void);
extern	Uint8	slGetPortMode2(void);
extern	Bool	slSetPortMode1(Uint8);
extern	Bool	slSetPortMode2(Uint8);

extern	Bool	slRequestCommand(Uint8, Uint8);

extern	SmpcStatus*	Smpc_Status;
extern	PerDigital*	Smpc_Peripheral;
extern	Uint8		Per_Connect1, Per_Connect2;

#ifdef  TITAN
typedef struct {                        /* System Switch */
        Uint8   dummy ;
        Uint8   data ;
        Uint8   push ;
        Uint8   pull ;
} SysSwitch ;

    extern  PerDigital  PerDigitalBuf[4] ;
    extern  SysSwitch   SystemSwitchBuf ;

#define     _ServiceSw          0x80    /* ��Տ�̃X�C�b�`�Ƃi�`�l�l�`���͂� �n�q�������� */
#define     _TestSw             0x40
#define     _ServiceSwBD        0x20    /* ��Տ�̃^�N�g�X�C�b�` */
#define     _TestSwBD           0x10
#define     _ServiceSwJM        0x08    /* �i�`�l�l�`�R�l�N�^����̃X�C�b�`���� */
#define     _TestSwJM           0x04
#define     _Coin2              0x02
#define     _Coin1              0x01
#endif

#ifdef  ST_V
    extern  PerDigital	PerDigitalBuf[4] ;
#endif
#ifdef  ST_VE
    extern  PerDigital	PerDigitalBuf[4] ;
#endif

#define	slMasterOn()			slRequestCommand(SMPC_MSHON , SMPC_NO_WAIT)
#define	slSlaveOn()			slRequestCommand(SMPC_SSHON , SMPC_NO_WAIT)
#define	slSlaveOff()			slRequestCommand(SMPC_SSHOFF, SMPC_NO_WAIT)

#ifdef TITAN
    extern void slSoundOn() ;
    extern void slSoundOff() ;
#else
#define	slSoundOn()		slRequestCommand(SMPC_SNDON , SMPC_NO_WAIT)
#define	slSoundOff()		slRequestCommand(SMPC_SNDOFF, SMPC_NO_WAIT)
#endif

#define	slCDOn()			slRequestCommand(SMPC_CDON  , SMPC_NO_WAIT)
#define	slCDOff()			slRequestCommand(SMPC_CDOFF , SMPC_NO_WAIT)
#define	slSystemReset()			slRequestCommand(SMPC_SYSRES, SMPC_NO_WAIT)
#define	slClockChange352()		slRequestCommand(SMPC_CKC352, SMPC_NO_WAIT)
#define	slClockChange320()		slRequestCommand(SMPC_CKC320, SMPC_NO_WAIT)
#define	slNMIRequest()			slRequestCommand(SMPC_NMIREQ, SMPC_NO_WAIT)
#define	slResetEnable()			slRequestCommand(SMPC_RESENA, SMPC_NO_WAIT)
#define	slResetDisable()		slRequestCommand(SMPC_RESDIS, SMPC_NO_WAIT)
#define	slGetStatus()			slRequestCommand(SMPC_GETSTS, SMPC_NO_WAIT)
#define	slGetPeripheral()		slRequestCommand(SMPC_GETPER, SMPC_NO_WAIT)
#define	slSetSmpcMemory()		slRequestCommand(SMPC_SETMEM, SMPC_NO_WAIT)
#define	slSetDateTime()			slRequestCommand(SMPC_SETTIM, SMPC_NO_WAIT)

#define	slMasterOnWait()		slRequestCommand(SMPC_MSHON , SMPC_WAIT)
#define	slSlaveOnWait()			slRequestCommand(SMPC_SSHON , SMPC_WAIT)
#define	slSlaveOffWait()		slRequestCommand(SMPC_SSHOFF, SMPC_WAIT)
#define	slSoundOnWait()			slRequestCommand(SMPC_SNDON , SMPC_WAIT)
#define	slSoundOffWait()		slRequestCommand(SMPC_SNDOFF, SMPC_WAIT)
#define	slCDOnWait()			slRequestCommand(SMPC_CDON  , SMPC_WAIT)
#define	slCDOffWait()			slRequestCommand(SMPC_CDOFF , SMPC_WAIT)
#define	slNMIRequestWait()		slRequestCommand(SMPC_NMIREQ, SMPC_WAIT)
#define	slResetEnableWait()		slRequestCommand(SMPC_RESENA, SMPC_WAIT)
#define	slResetDisableWait()		slRequestCommand(SMPC_RESDIS, SMPC_WAIT)
#define	slSetSmpcMemoryWait()		slRequestCommand(SMPC_SETMEM, SMPC_WAIT)
#define	slSetDateTimeWait()		slRequestCommand(SMPC_SETTIM, SMPC_WAIT)

/*  Event library	*/

    extern void    slInitEvent(void) ;
    extern EVENT  *slGetEvent(void) ;
    extern void    slReturnEvent(EVENT *) ;
    extern EVENT  *slSetEvent(void (*func)()) ;
    extern void    slCloseEvent(EVENT *) ;
    extern EVENT  *slSetEventNext(EVENT * , void (*func)()) ;
    extern void    slExecuteEvent(void) ;
    extern WORK   *slGetWork(void) ;
    extern void    slReturnWork(WORK *) ;

/*  DMA control */

    extern void    slDMACopy(void * , void * , Uint32) ;
    extern void    slDMAXCopy(void * , void * , Uint32 , Uint16) ;
    extern void    slDMAWait() ;
    extern Bool    slDMAStatus() ;

/* DMA control with SBL */

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */

/* �萔(CPU-DMA) */
/**** DMA���W�X�^�r�b�g�ʒu **************************************************/
/******** �`���l���R���g���[�����W�X�^ ***************************************/
#define DMA_CHCR_DE     0       /* DMA�C�l�[�u���r�b�g                       */
#define DMA_CHCR_TE     1       /* �g�����X�t�@�G���h�t���O�r�b�g            */
#define DMA_CHCR_IE     2       /* �C���^���v�g�C�l�[�u���r�b�g              */
#define DMA_CHCR_TA     3       /* �g�����X�t�@�A�h���X���[�h�r�b�g          */
#define DMA_CHCR_TB     4       /* �g�����X�t�@�o�X���[�h�r�b�g              */
#define DMA_CHCR_DL     5       /* DREQ���x���r�b�g�@�@                      */
#define DMA_CHCR_DS     6       /* DREQ�Z���N�g�r�b�g                        */
#define DMA_CHCR_AL     7       /* �A�N�m���b�W���x���r�b�g                  */
#define DMA_CHCR_AM     8       /* �A�N�m���b�W/�g�����X�t�@���[�h�r�b�g     */
#define DMA_CHCR_AR     9       /* �I�[�g���N�G�X�g�r�b�g                    */
#define DMA_CHCR_TS     10      /* �g�����X�t�@�T�C�Y�r�b�g                  */
#define DMA_CHCR_SM     12      /* �\�[�X�A�h���X���[�h�r�b�g                */
#define DMA_CHCR_DM     14      /* �f�B�X�e�B�l�[�V�����A�h���X���[�h�r�b�g  */
/******** DMA�I�y���[�V�������W�X�^ ******************************************/
#define DMA_DMAOR_DME   0               /* DMA�}�X�^�C�l�[�u���r�b�g         */
#define DMA_DMAOR_NMIF  1               /* NMI�t���O�r�b�g                   */
#define DMA_DMAOR_AE    2               /* �A�h���X�G���[�t���O�r�b�g        */
#define DMA_DMAOR_PR    3               /* �v���C�I���e�B���[�h�r�b�g        */

/*****************************************************************************/
/**** DMA�`���l�� ************************************************************/
/*****************************************************************************/
#define DMA_CPU_CH0 0                           /* �`���l��0                 */
#define DMA_CPU_CH1 1                           /* �`���l��1                 */

/**** (DmaCpuComPrm)���ʓ]���p�����[�^ ***************************************/
/******** (pr)�v���C�I���e�B���[�h  ******************************************/
#define DMA_CPU_FIX (0 << DMA_DMAOR_PR)         /* �D�揇�ʂ͌Œ�            */
#define DMA_CPU_ROR (1 << DMA_DMAOR_PR)         /* �D�揇�ʂ̓��E���h���r��- */
                                                /* �ɂ��                    */
/******** (dme)DMA�}�X�^�C�l�[�u�� *******************************************/
#define DMA_CPU_DIS     (0 << DMA_DMAOR_DME)    /* �S�`���l����DMA�]�����֎~ */
#define DMA_CPU_ENA     (1 << DMA_DMAOR_DME)    /* �S�`���l����DMA�]�������� */
/******** (msk)�}�X�N�r�b�g **************************************************/
#define DMA_CPU_M_PR    (1 << 0)                /* �v���C�I���e�B���[�h�@�@  */
#define DMA_CPU_M_AE    (1 << 1)                /* �A�h���X�G���[�t���O�@�@  */
#define DMA_CPU_M_NMIF  (1 << 2)                /* NMI�t���O           �@�@  */
#define DMA_CPU_M_DME   (1 << 3)                /* DMA�}�X�^�C�l�[�u�� �@�@  */

/**** (DmaCpuPrm)�]���p�����[�^ **********************************************/
/******** (dm)�f�B�X�e�B�l�[�V�����A�h���X���[�h�r�b�g ***********************/
/******** (sm)�\�[�X�A�h���X���[�h�r�b�g *************************************/
#define DMA_CPU_AM_NOM  0x0                    /* �Œ�                      */
#define DMA_CPU_AM_ADD  0x1                    /* ����                      */
#define DMA_CPU_AM_SUB  0x2                    /* ����                      */
/******** (ts)�g�����X�t�@�T�C�Y *********************************************/
#define DMA_CPU_1   (0 << DMA_CHCR_TS)          /* �o�C�g�P��                */
#define DMA_CPU_2   (1 << DMA_CHCR_TS)          /* ���[�h(2�o�C�g)�P��       */
#define DMA_CPU_4   (2 << DMA_CHCR_TS)          /* �����O���[�h(4�o�C�g)�P�� */
#define DMA_CPU_16  (3 << DMA_CHCR_TS)          /* 16�o�C�g�]��              */
/******** (ar)�I�[�g���N�G�X�g���[�h *****************************************/
#define DMA_CPU_MOD     (0 << DMA_CHCR_AR)      /* ���W���[�����N�G�X�g      */
#define DMA_CPU_AUTO    (1 << DMA_CHCR_AR)      /* �I�[�g���N�G�X�g          */
/******** (ie)�C���^���v�g�C�l�[�u�� *****************************************/
#define DMA_CPU_INT_ENA (1 << DMA_CHCR_IE)      /* ���荞�ݗv��������        */
#define DMA_CPU_INT_DIS (0 << DMA_CHCR_IE)      /* ���荞�ݗv�����֎~(�����l)*/
/******** (drcr)DMA�v���^�����I�𐧌� ****************************************/
#if 0
#define DMA_CPU_DREQ    0x0                     /* DREQ(�O�����N�G�X�g)      */
/*
 *  DMA�v���ɊO�����N�G�X�g���w�肷��̂͋֎~('95-11/14 H.O)
 */
#endif
#define DMA_CPU_RXI     0x1                     /* RXI(����SCI�̎�M�f�[�^-  */
                                                /* �t�����荞�ݓ]���v��)     */
#define DMA_CPU_TXI     0x2                     /* TXI(����SCI�̑��M�f�[�^-  */
                                                /* �G���v�e�B���荞�ݓ]���v��*/
/******** (msk)�}�X�N�r�b�g **************************************************/
#define DMA_CPU_M_SAR   (1 << 0)                /* DMA�\�[�X�A�h���X         */
#define DMA_CPU_M_DAR   (1 << 1)                /* DMA�f�B�X�e�B�l�[�V����-  */
                                                /* �A�h���X                  */
#define DMA_CPU_M_TCR   (1 << 2)                /* DMA�g�����X�t�@�J�E���g   */
#define DMA_CPU_M_DM    (1 << 3)                /* DMA�f�B�X�e�B�l�[�V����-  */
                                                /* �A�h���X���[�h�r�b�g      */
#define DMA_CPU_M_SM    (1 << 4)                /* DMA�\�[�X�A�h���X���[�h-  */
                                                /* �r�b�g                    */
#define DMA_CPU_M_TS    (1 << 5)                /* �g�����X�t�@�T�C�Y        */
#define DMA_CPU_M_AR    (1 << 6)                /* �I�[�g���N�G�X�g���[�h    */
#define DMA_CPU_M_IE    (1 << 7)                /* �C���^���v�g�C�l�[�u��    */
#define DMA_CPU_M_DRCR  (1 << 8)                /* DMA�v���^�����I�𐧌�     */
#define DMA_CPU_M_TE    (1 << 9)                /* �g�����X�t�@�G���h�t���O  */

/**** (DmaCpuComStatus)���ʃX�e�[�^�X ****************************************/
/******** (ae)�A�h���X�G���[�t���O *******************************************/
#define DMA_CPU_ADR_ERR     (1 << DMA_DMAOR_AE) /* DMAC�ɂ��A�h���X�G���[- */
                                                /* ����                      */
#define DMA_CPU_ADR_NO_ERR  (0 << DMA_DMAOR_AE) /* DMAC�ɂ��A�h���X�G���[- */
                                                /* �Ȃ�                      */
                                                
/******** (nmif)NMI�t���O ****************************************************/
#define DMA_CPU_NMI_ON  (1 << DMA_DMAOR_NMIF)   /* NMI���荞�ݔ���           */
#define DMA_CPU_NMI_OFF (0 << DMA_DMAOR_NMIF)   /* NMI���荞�݂Ȃ�           */

/**** (DmaCpuComStatus)���ʃX�e�[�^�X ****************************************/
#define DMA_CPU_TE_MV   (0 << DMA_CHCR_TE)      /* DMA�]�����܂���DMA�]�����f*/
#define DMA_CPU_TE_SP   (1 << DMA_CHCR_TE)      /* DMA�]������I��           */


typedef struct {
    Uint32 pr;                                  /* �v���C�I���e�B���[�h      */
    Uint32 dme;                                 /* DMA�}�X�^�C�l�[�u��       */
    Uint32 msk;                                 /* �}�X�N�r�b�g              */
}DmaCpuComPrm;                                  /* ���ʓ]���p�����[�^        */

typedef struct {
    Uint32 sar;                                 /* DMA�\�[�X�A�h���X         */
    Uint32 dar;                                 /* DMA�f�B�X�e�B�l�[�V����-  */
                                                /* �A�h���X                  */
    Uint32 tcr;                                 /* DMA�g�����X�t�@�J�E���g   */
    Uint32 dm;                                  /* �f�B�X�e�B�l�[�V�����A�h- */
                                                /* ���X���[�h�r�b�g          */
    Uint32 sm;                                  /* �\�[�X�A�h���X���[�h�r�b�g*/
    Uint32 ts;                                  /* �g�����X�t�@�T�C�Y        */
    Uint32 ar;                                  /* �I�[�g���N�G�X�g���[�h    */
    Uint32 ie;                                  /* �C���^���v�g�C�l�[�u��    */
    Uint32 drcr;                                /* DMA�v���^�����I�𐧌�     */
    Uint32 msk;                                 /* �}�X�N�r�b�g              */
}DmaCpuPrm;                                     /* �]���p�����[�^            */

typedef struct {
    Uint32 ae;                                  /* �A�h���X�G���[�t���O      */
    Uint32 nmif;                                /* NMI�t���O                 */
}DmaCpuComStatus;                               /* ���ʃX�e�[�^�X            */

typedef Uint32 DmaCpuStatus;                    /* �X�e�[�^�X                */

/* �萔(SCU-DMA) */
/*****************************************************************************/
/**** DMA�`���l�� ************************************************************/
/*****************************************************************************/
#define DMA_SCU_CH0 0x0                         /* �`���l��0                 */
#define DMA_SCU_CH1 0x1                         /* �`���l��1                 */
#define DMA_SCU_CH2 0x2                         /* �`���l��2                 */

/**** DMA���W�X�^�r�b�g�ʒu **************************************************/
/******** ���Z�l���W�X�^ *****************************************************/
#define DMA_DXAD_DXRAD  8                       /* �ǂݍ��݃A�h���X���Z�l    */
#define DMA_DXAD_DXWAD  0                       /* �������݃A�h���X���Z�l    */
/******** DMA�����W�X�^ ****************************************************/
#define DMA_DXEN_DXGO   0                       /* DMA�N���r�b�g             */
#define DMA_DXEN_DXEN   8                       /* DMA���r�b�g             */
/******** DMA���[�h�A�A�h���X�X�V�A�N���v�����W�X�^ **************************/
#define DMA_DXMD_DXMOD  24                      /* ���[�h�r�b�g              */
#define DMA_DXMD_DXRUP  16                      /* �ǂݍ��݃A�h���X�X�V�r�b�g*/
#define DMA_DXMD_DXWUP   8                      /* �����o���A�h���X�X�V�r�b�g*/
#define DMA_DXMD_DXFT    0                      /* �N���v���I���r�b�g        */


/*****************************************************************************/
/**** (DmaScuPrm)�]���p�����[�^ **********************************************/
/*****************************************************************************/
/******** (dxad_r)�ǂݍ��݃A�h���X���Z�l *************************************/
#define DMA_SCU_R0  (0x0 << DMA_DXAD_DXRAD)     /* ���Z���Ȃ�                */
#define DMA_SCU_R4  (0x1 << DMA_DXAD_DXRAD)     /* 4�o�C�g���Z����           */
/******** (dxad_w)�������݃A�h���X���Z�l *************************************/
#define DMA_SCU_W0     (0x0 << DMA_DXAD_DXWAD)  /* ���Z���Ȃ�                */
#define DMA_SCU_W2     (0x1 << DMA_DXAD_DXWAD)  /* 2�o�C�g���Z����           */
#define DMA_SCU_W4     (0x2 << DMA_DXAD_DXWAD)  /* 4�o�C�g���Z����           */
#define DMA_SCU_W8     (0x3 << DMA_DXAD_DXWAD)  /* 8�o�C�g���Z����           */
#define DMA_SCU_W16    (0x4 << DMA_DXAD_DXWAD)  /* 16�o�C�g���Z����          */
#define DMA_SCU_W32    (0x5 << DMA_DXAD_DXWAD)  /* 32�o�C�g���Z����          */
#define DMA_SCU_W64    (0x6 << DMA_DXAD_DXWAD)  /* 64�o�C�g���Z����          */
#define DMA_SCU_W128   (0x7 << DMA_DXAD_DXWAD)  /* 128�o�C�g���Z����         */
/******** (dxmod)���[�h�r�b�g ************************************************/
#define DMA_SCU_DIR    (0x0 << DMA_DXMD_DXMOD)  /* ���ڃ��[�h                */
#define DMA_SCU_IN_DIR (0x1 << DMA_DXMD_DXMOD)  /* �Ԑڃ��[�h                */
/******** (dxrup)�ǂݍ��݃A�h���X�X�V�r�b�g **********************************/
/******** (dxwup)�����o���A�h���X�X�V�r�b�g **********************************/
#define DMA_SCU_KEEP    0x0                     /* �ێ�����                  */
#define DMA_SCU_REN     0x1                     /* �X�V����                  */

/******** (dxft)�N���v���I���r�b�g *******************************************/
#define DMA_SCU_F_VBLK_IN  (0x0 << DMA_DXMD_DXFT)/* V-�u�����N-IN            */
#define DMA_SCU_F_VBLK_OUT (0x1 << DMA_DXMD_DXFT)/* V-�u�����N-OUT           */
#define DMA_SCU_F_HBLK_IN  (0x2 << DMA_DXMD_DXFT)/* H-�u�����N-IN            */
#define DMA_SCU_F_TIM0     (0x3 << DMA_DXMD_DXFT)/* �^�C�}0                  */
#define DMA_SCU_F_TIM1     (0x4 << DMA_DXMD_DXFT)/* �^�C�}1                  */
#define DMA_SCU_F_SND      (0x5 << DMA_DXMD_DXFT)/* �T�E���h-Req             */
#define DMA_SCU_F_SPR      (0x6 << DMA_DXMD_DXFT)/* �X�v���C�g�`��           */
#define DMA_SCU_F_DMA      (0x7 << DMA_DXMD_DXFT)/* DMA�N���v���r�b�g�̃Z�b�g*/
/******** (msk)�}�X�N�r�b�g *************************************************/
#define DMA_SCU_M_DXR      (1 << 0)             /* �ǂݍ��݃A�h���X          */
#define DMA_SCU_M_DXW      (1 << 1)             /* �����o���A�h���X          */

/*****************************************************************************/
/**** (DmaScuStatus)�X�e�[�^�X ***********************************************/
/*****************************************************************************/
/******** (dxbkr)DMA���쒆�t���O *********************************************/
/******** (ddmv)DSP��DMA���쒆�t���O *****************************************/
#define DMA_SCU_MV      0x1                     /* ���쒆                    */
#define DMA_SCU_NO_MV   0x0                     /* ���쒆�łȂ�              */

typedef struct {
    Uint32 dxr;                                 /* �ǂݍ��݃A�h���X          */
    Uint32 dxw;                                 /* �����o���A�h���X          */
    Uint32 dxc;                                 /* �]���o�C�g��              */
    Uint32 dxad_r;                              /* �ǂݍ��݃A�h���X���Z�l    */
    Uint32 dxad_w;                              /* �������݃A�h���X���Z�l    */
    Uint32 dxmod;                               /* ���[�h�r�b�g              */
    Uint32 dxrup;                               /* �ǂݍ��݃A�h���X�X�V�r�b�g*/
    Uint32 dxwup;                               /* �������݃A�h���X�X�V�r�b�g*/
    Uint32 dxft;                                /* �N���v���I���r�b�g        */
    Uint32 msk;                                 /* �}�X�N�r�b�g              */
}DmaScuPrm;                                     /* �]���p�����[�^            */

typedef struct {
    Uint32 dxmv;                            /* DMA���쒆�t���O               */
}DmaScuStatus;                              /* �X�e�[�^�X                    */

    extern void    DMA_CpuSetComPrm(DmaCpuComPrm *) ;
    extern void    DMA_CpuSetPrm(DmaCpuPrm *, Uint32) ;
    extern void    DMA_CpuStart(Uint32) ;
    extern void    DMA_CpuStop(Uint32) ;
    extern void    DMA_CpuAllStop() ;
    extern void    DMA_CpuGetComStatus(DmaCpuComStatus *) ;
    extern DmaCpuStatus DMA_CpuGetStatus(Uint32) ;
    extern void    DMA_ScuSetPrm(DmaScuPrm *, Uint32) ;
    extern void    DMA_ScuStart(Uint32) ;
    extern void    DMA_ScuGetStatus(DmaScuStatus *, Uint32) ;

    extern void    DMA_ScuMemCopy(void *, void *, Uint32) ;
    extern Uint32  DMA_ScuResult() ;

    extern void    DMA_CpuMemCopy1(void *, void *, Uint32) ;
    extern void    DMA_CpuMemCopy2(void *, void *, Uint32) ;
    extern void    DMA_CpuMemCopy4(void *, void *, Uint32) ;
    extern void    DMA_CpuMemCopy16(void *, void *, Uint32) ;
    extern Uint32  DMA_CpuResult() ;

/*  Slave CPU control */
    extern void    slSlaveFunc(void (*func)() , void *) ;

/*  System library */
    extern  void   slInitSystem(Uint16 , TEXTURE * , Sint8) ;
    extern  void   slSetTVMode(Uint16) ;
    extern  void   slSetScrTVMode(Uint16) ;
    extern  void   slSetSprTVMode(Uint16) ;
    extern  Bool   slDynamicFrame(Uint16) ;
    extern  void   slSynch(void) ;
    extern  void   slInitSynch(void) ;
    extern  Bool   slTransferEntry(void * , void * , Uint16) ;
    extern  void   slIntFunction(void (*func)()) ;
    extern  void   slSynchFunction(void (*func)()) ;
    extern  void   SetCDFunc(void (*func)()) ;
    extern  void   slCashPurge() ;

/*  Sound library */
    extern  void   slInitSound(Uint8 * , Uint32 , Uint8 * , Uint32) ;
    extern  Bool   slBGMOn(Uint16 , Uint8 , Uint8 , Uint8) ;
    extern  Bool   slBGMPause() ;
    extern  Bool   slBGMCont() ;
    extern  Bool   slBGMOff() ;
    extern  Bool   slBGMFade(Uint8 , Uint8) ;
    extern  Bool   slBGMTempo(Sint16) ;
#define     slBGMStat()    slSequenceStat(0)
    extern  Bool   slSoundAllOff() ;
    extern  Bool   slSoundAllPause() ;
    extern  Bool   slSoundAllCont() ;
    extern  Bool   slDSPOff() ;
    extern  Bool   slSndVolume(Uint8) ;
    extern  Uint8  slSequenceOn(Uint16 , Uint8 , Uint8 , Sint8) ;
    extern  Bool   slSequenceOff(Uint8) ;
    extern  Bool   slSequenceFade(Uint8 , Uint8 , Uint8) ;
    extern  Bool   slSequenceTempo(Uint8 , Sint16) ;
    extern  Bool   slSequencePause(Uint8) ;
    extern  Bool   slSequenceCont(Uint8) ;
    extern  Bool   slSequencePan(Uint8 , Sint8) ;
    extern  Bool   slSequenceStat(Uint8) ;
    extern  Bool   slSequenceReset(Uint8) ;
    extern  void  *slSndMapChange(Uint8) ;
    extern  void   slWaitSound(void *) ;
    extern  Bool   slCDDAOn(Uint8 , Uint8 , Sint8 , Sint8) ;
    extern  Bool   slCDDAOff() ;
    extern  Sint8  slPCMOn(PCM * , void * , Uint32) ;
    extern  Bool   slPCMOff(PCM *) ;
    extern  Bool   slPCMParmChange(PCM *) ;
    extern  Bool   slPCMStat(PCM *) ;
    extern  Sint8  slSndPCMNum() ;
    extern  Uint8  slSndSeqNum() ;
    extern  Bool   slSndEffect(Uint8) ;
    extern  Bool   slSndMixChange(Uint8 , Uint8) ;
    extern  Bool   slSndMixParmChange(Uint8 , Uint8 , Sint8) ;
    extern  Sint8  slSoundRequest(const char*,...) ;
    extern  void   slSndFlush() ;

/*  Gouraud library */

    typedef Uint16 GOURAUDTBL[4];

    extern  void slInitGouraud( GOURAUDTBL *, Uint32, Uint32, Uint8 *);
    extern  void slPutPolygonX( XPDATA *, FIXED *);
    extern  void slGouraudTblCopy(void);
    extern  void slSetGouraudTbl( Uint16 *);
    extern  void slSetGouraudColor( Uint16 );
    extern  void slSetAmbient( Uint16);

    extern  void slSetFlatColor( Uint16);

    extern  void slSetDepthLimit( Uint32, Uint16, Uint16 );
    extern  void slSetDepthTbl( Uint16 *, Uint16, Uint16 );

    extern  void slNearClipFlag( Uint32 );
    extern  void slDispPlaneLevel( Sint32 );
    extern  void slWindowClipLevel( Sint32 );

/*------------------------------------------------------------------------*/

#endif