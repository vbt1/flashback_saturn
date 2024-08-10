/*-----------------------------------------------------------------------------
 *  FILE: sega_mem.h
 *
 *      Copyright(c) 1994-1995 SEGA.
 *
 *  PURPOSE:
 *
 *      ���������C�u�����w�b�_�t�@�C���B
 *
 *  DESCRIPTION:
 *
 *      �������Ǘ�����������B
 *
 *  AUTHOR(S)
 *
 *      1994-07-06  N.T Ver.1.00
 *
 *  MOD HISTORY:
 *      1995-12-05  H.O Ver.1.01
 *
 *-----------------------------------------------------------------------------
 */

/*
 * C VIRTUAL TYPES DEFINITIONS
 */
#include "sega_xpt.h"

/*
 * USER SUPPLIED INCLUDE FILES
 */

#ifndef	_SEGA_MEM_H
#define	_SEGA_MEM_H

#ifdef	__MEM_LIB_SOURCE__

/*
 * GLOBAL DEFINES/MACROS DEFINES
 */

/*
 * STRUCTURE DECLARATIONS
 */

/*
 * TYPEDEFS
 */
typedef double MemAlign;                        /* 64�r�b�g�̃A���C�����g    */
union mem_head {                                /* �Z���̃w�b�_              */
    struct {
        union mem_head *next;                   /* ���̃Z���|�C���^          */
        unsigned size;                          /* �Z���̃T�C�Y              */
    }s;
    MemAlign damy;
};

typedef union mem_head MemHead;                 /* �Z���w�b�_�f�[�^�^        */

/*
 * EXTERNAL VARIABLE DECLARATIONS
 */
extern MemHead *MEM_empty_top;                  /* �󂫃Z���̐擪            */

#endif	/* __MEM_LIB_SOURCE__ */

/*
 * EXTERNAL FUNCTION PROTOTYPE  DECLARATIONS
 */
void	 MEM_Init(Uint32, Uint32);
void	*MEM_Calloc(Uint32, Uint32);
void	*MEM_Malloc(Uint32);
void	*MEM_Realloc(void *,Uint32);
void	 MEM_Free(void *);
#endif  /* ifndef _SEGA_MEM_H */
