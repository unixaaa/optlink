
#include <stdlib.h>

#include "all.h"

extern void (*CV_DWORD_ALIGN)();
extern void INIT_LOCAL_STORAGE();

extern SRC_STRUCT *_init_help_counts(SRC_STRUCT *EAX);

typedef struct HLP_STRUCT
{
    unsigned _HLP_LAST_LINHELP;
    unsigned _HLP_FIRST_LINHELP;
} HLP_STRUCT;


typedef struct CVLIN_STRUCT
{
    int CV_OFFSET_BLOCKS[(64 * 1024) / PAGE_SIZE * 4];
    int CV_LINNUM_BLOCKS[(64 * 1024) / PAGE_SIZE * 2];

    int CV_LIN_BLK;
    int CV_LIN_PHYS;
    int CV_LIN_FACTOR;
    int CV_SEG_NUMBER;
    int CV_LSEG_GINDEX;
    int CV_SEG_FRAME;
    LINNUM_HEADER_TYPE CV_LN_STUFF;
    int CV_INPUT_BLK_PTR;
    int CV_LINNUM_PTR;
    int CV_LINNUM_PTR_LIMIT;
    int CV_OFFSET_PTR;
    int CV_OFFSET_PTR_LIMIT;
    int CV_NEXT_LINNUM_BLK;
    int CV_NEXT_OFFSET_BLK;
    int CV_FIRST_OFFSET;
    int CV_LAST_OFFSET;
    int CV_LINE_COUNT;
    int CV_BASEMOD;
    int CV_M_FIRST_SRC_GINDEX;
    int CV_M_FIRST_SRC_NUMBER;
    int CV_M_FIRST_CSEGMOD_GINDEX;
    int CV_M_SRC_COUNT;
    int CV_M_CSEG_COUNT;
    HLP_STRUCT* CV_HELP_PTRS;
    int CV_SEGMOD_EXPECTED;
    int CV_OFFSET_SIZE;
    int CV_OFFSET_MASK;
    int CV_BLOCK_BASE;
    int CV_INPUT_PTR_LIMIT;
} CVLIN_STRUCT;

#define CV_LN_NEXT_LINNUM       CV_LN_STUFF._LN_NEXT_LINNUM
#define CV_LN_BLOCK_BASE        CV_LN_STUFF._LN_BLOCK_BASE
#define CV_LN_LENGTH            CV_LN_STUFF._LN_LENGTH
#define CV_LN_TYPE              CV_LN_STUFF._LN_TYPE
#define CV_LN_SRC_GINDEX        CV_LN_STUFF._LN_SRC_GINDEX

extern void _check_csegmod(int, int, CVLIN_STRUCT*);

struct SRC_LIN_HELP
{
    unsigned _SLH_NEXT_HELP;
    unsigned _SLH_FIRST_OFFSET;
    unsigned _SLH_LAST_OFFSET;
    unsigned _SLH_BASEMOD;
};


struct CSEG_DESC_STRUCT
{
    unsigned _CSD_NEXT_CSD;
    unsigned _CSD_START_OFFSET;
    unsigned _CSD_END_OFFSET;
    unsigned _CSD_SEG_GINDEX;
};

void print_int(int i)
{
    printf("i = %d\n", i);
}

void _cv_linnums_4()
{
        // OUTPUT LINE NUMBER INFO FOR THIS MODULE, CODEVIEW VERSION < 4.0
        // FIRST COPY ALL LINE NUMBERS FROM THIS MODULE TO EXETABLE
        // SORT FOR EACH CSEGMOD INDIVIDUALLY

        MODULE_STRUCT *ms = (MODULE_STRUCT *)CURNMOD_GINDEX;    // this module

        // SEE IF ANY LINE NUMBERS THIS MODULE

        if (!ms->_M_MDB_GINDEX)
            return;

        if (!(ms->_M_FLAGS & M_SRCS_PRESENT))
            return;

        struct CVLIN_STRUCT EBP;

        INIT_LOCAL_STORAGE();
        (*CV_DWORD_ALIGN)();

        unsigned EAXPUSH = BYTES_SO_FAR;        // SAVE FOR SECTION LENGTH

        EBP.CV_BASEMOD = EAXPUSH;

        MDB_STRUCT *ESI = (MDB_STRUCT *)ms->_M_MDB_GINDEX;

        unsigned count = ESI->_MD_CSEG_COUNT;

        if (!count)
            goto L99;

        EBP.CV_M_SRC_COUNT = ESI->_MD_SRC_COUNT;
        EBP.CV_M_CSEG_COUNT = count;

        unsigned EBX = ESI->_MD_FIRST_CSEGMOD_GINDEX;

        // CALCULATE SIZE OF HEADER

        EBP.CV_M_FIRST_SRC_GINDEX = ESI->_MD_FIRST_SRC_GINDEX;

        EBP.CV_M_FIRST_SRC_NUMBER = _init_help_counts((SRC_STRUCT*)EBP.CV_M_FIRST_SRC_GINDEX)->_SRC_NUMBER;

        unsigned EAX = EBP.CV_M_CSEG_COUNT * 2;
        unsigned ECX = EBP.CV_M_SRC_COUNT * 4;

        EBP.CV_M_FIRST_CSEGMOD_GINDEX = EBX;

        BYTES_SO_FAR = BYTES_SO_FAR + ((EAX * 4 + EAX + ECX + 7) & ~3);

        //void *p = _get_new_log_blk(); // PLACE FOR SRC BEGINNING AND ENDING HELP PTRS
        // PLACE FOR SRC BEGINNING AND ENDING HELP PTRS
        EBP.CV_HELP_PTRS = (HLP_STRUCT *)calloc(EBP.CV_M_SRC_COUNT, sizeof(HLP_STRUCT));
        //assert(EBP.CV_HELP_PTRS);

        //printf("cv_help_ptrs = %p, count = %d, max = %p\n", EBP.CV_HELP_PTRS, EBP.CV_M_SRC_COUNT, &EBP.CV_HELP_PTRS[EBP.CV_M_SRC_COUNT]);

        EBP.CV_SEG_NUMBER = 0;          // FORCE NEW RECORD
        EBP.CV_LSEG_GINDEX = 0;
        EBP.CV_SEGMOD_EXPECTED = 0;
        HIGH_WATER = 0;
        _check_csegmod(EBP.CV_M_FIRST_CSEGMOD_GINDEX, EAXPUSH, &EBP + 1);
        return;

L99:
        _release_local_storage();
}

/*
                ASSUME  EDI:PTR CVL_STRUCT,ESI:NOTHING

CV_LIN_SPEC:
                ;
                ;MAY CROSS BOUNDARIES
                ;
                CALL    GET2

                AND     EAX,0FFFFH

                MOV     [EDI]._CVL_LINNUM,EAX           ;LINE #
                CALL    GET2

                MOV     EDX,CV_OFFSET_SIZE
                AND     EAX,0FFFFH

                CMP     EDX,4
                JZ      L16_1

                MOV     EDX,EAX
                CALL    GET2

                SHL     EAX,16

                ADD     EAX,EDX
L16_1:
                ADD     EAX,CV_LIN_FACTOR
                MOV     EDX,CV_LN_SRC_GINDEX

                MOV     [EDI]._CVL_OFFSET,EAX
                MOV     [EDI]._CVL_SRC_GINDEX,EDX

                ADD     EDI,SIZE CVL_STRUCT
                JMP     LL_3

CSEGMOD_LOOP:
                ;
                ;START A NEW SEGMOD
                ;
                CALL    CSEGMOD_START

                PUSH    EAX             ;NEXT CSEGMOD
                JMP     CHECK_LINNUM

LLIN_LOOP:
                ;
                ;START A NEW LINNUM RECORD
                ;
                MOV     EAX,ECX
                CALL    LINE_START      ;INIT RECORD BUFFER,
                                        ;DX = OFFSET TO ADD, CX = # OF ITEMS,
                                        ;BX = 2 IF 32-BIT, ELSE 0
LINE_LOOP:
                CMP     ESI,EBX         ;AT LEAST 1 LEFT?
                JAE     CV_LIN_SPEC

                MOV     EAX,[ESI]       ;LINE #
                ADD     EDI,SIZEOF CVL_STRUCT

                AND     EAX,0FFFFH
                MOV     EDX,CV_OFFSET_MASK

                MOV     [EDI-SIZE CVL_STRUCT]._CVL_LINNUM,EAX
                MOV     EAX,[ESI+2]

                AND     EAX,EDX
                MOV     EDX,CV_LIN_FACTOR

                ADD     EAX,EDX
                MOV     EDX,CV_OFFSET_SIZE

                MOV     [EDI-SIZE CVL_STRUCT]._CVL_OFFSET,EAX
                MOV     EAX,CV_LN_SRC_GINDEX

                ADD     ESI,EDX
                MOV     [EDI-SIZE CVL_STRUCT]._CVL_SRC_GINDEX,EAX
LL_3:
                CMP     EDI,OFF CV_TEMP_RECORD+CV_TEMP_SIZE-SIZE CVL_STRUCT     ;ROOM FOR AT LEAST 1 MORE?
                JAE     L5$
L51$:
                DEC     ECX
                JNZ     LINE_LOOP

                JMP     LINE_DONE

L5$:
                PUSH    ECX
                CALL    FLUSH_LIN_TEMP

                POP     ECX
                JMP     L51$

LINE_DONE:
                CALL    FLUSH_LIN_TEMP
NEXT_LINE:
                ;
                ;DEC USAGE COUNT
                ;
                MOV     EAX,CV_BLOCK_BASE
                MOV     ECX,CV_LN_NEXT_LINNUM   ;ANOTHER RECORD?

                DEC     DPTR [EAX]
                JNZ     NY_1

                CALL    RELEASE_SEGMENT
NY_1:
CHECK_LINNUM:
                TEST    ECX,ECX
                JNZ     LLIN_LOOP

                ASSUME  EDI:NOTHING
                ;
                ;NO MORE LINES THIS SEGMOD, TRY NEXT
                ;
                POP     EAX
CHECK_CSEGMOD:
                TEST    EAX,EAX
                JNZ     CSEGMOD_LOOP

                CALL    HANDLE_SEGMOD_LINES     ;FLUSH LINES FROM LAST SEGMOD
                ;
                ;NO MORE CODE SEGMODS, YOU ARE DONE
                ;
                CALL    LINNUM_FLUSH            ;FLUSH COUNT FROM LAST LINNUM TYPE

                CALL    RELEASE_LOCAL_STORAGE

                MOV     EAX,CV_HELP_PTRS
                CALL    RELEASE_BLOCK
                ;
                ;NOW OUTPUT INDEX ENTRY
                ;
                POP     EAX                     ;OLD BYTES_SO_FAR (FOR INDEX)
                MOV     ECX,127H

                CALL    HANDLE_CV_INDEX

                CALL    CV_DWORD_ALIGN

                MOV     ESP,EBP

                POPM    EBX,ESI,EDI,EBP

                RESUME_CANCEL                   ;ALLOW CANCEL IF REQUESTED

                RET

CV_LINNUMS_4    ENDP

INIT_HELP_COUNTS        PROC    NEAR PRIVATE

                ASSUME  EAX:PTR SRC_STRUCT

                PUSH    EAX
                SUB     ECX,ECX
L1$:
                MOV     [EAX]._SRC_LINHELP_COUNT,ECX

                MOV     EAX,[EAX]._SRC_NEXT_GINDEX

                TEST    EAX,EAX
                JNZ     L1$

                POP     EAX
                RET

                ASSUME  EAX:NOTHING

INIT_HELP_COUNTS        ENDP

GET2            PROC    NEAR    PRIVATE
                ;
                ;GET NEXT TWO BYTES FROM INPUT STREAM
                ;
                LEA     EAX,[EBX+6]

                CMP     ESI,EAX
                JE      L1$

                MOV     AX,WPTR [ESI]
                ADD     ESI,2

                RET

L1$:
                MOV     EAX,CV_BLOCK_BASE

                DEC     DPTR [EAX]              ;LAST ITEM IN BLOCK?
                MOV     ESI,[EAX+4]

                MOV     CV_BLOCK_BASE,ESI
                JNZ     L2$

                CALL    RELEASE_SEGMENT
L2$:
                LEA     EBX,[ESI + PAGE_SIZE-6]

                MOV     AX,WPTR [ESI+8]
                ADD     ESI,10

                RET

GET2            ENDP


LINE_START      PROC    NEAR    PRIVATE
                ;
                ;EAX IS LOGICAL ADDRESS OF NEW LINNUM RECORD
                ;
                ;INIT RECORD BUFFER,
                ;
                ;RETURNS:
                ;DX = OFFSET TO ADD, CX = # OF ITEMS,
                ;BX = 2 IF 32-BIT, ELSE 0
                ;

                ASSUME  EAX:PTR LINNUM_HEADER_TYPE

                MOV     ECX,[EAX]._LN_BLOCK_BASE
                MOV     EDX,[EAX]._LN_NEXT_LINNUM

                MOV     CV_LN_BLOCK_BASE,ECX
                MOV     CV_BLOCK_BASE,ECX

                LEA     EBX,[ECX+PAGE_SIZE-6]

                MOV     CV_LN_NEXT_LINNUM,EDX
                MOV     ECX,DPTR [EAX]._LN_LENGTH

                MOV     EDX,[EAX]._LN_SRC_GINDEX
                MOV     DPTR CV_LN_LENGTH,ECX

                MOV     CV_LN_SRC_GINDEX,EDX
                LEA     ESI,[EAX + SIZEOF LINNUM_HEADER_TYPE]
                ;
                ;
                ;
                MOV     AL,CV_LN_TYPE

                MOV     ECX,6
                MOV     EDX,-1

                TEST    AL,MASK BIT_32
                JNZ     L1$

                MOV     CL,4
                MOV     EDX,0FFFFH
L1$:
                MOV     CV_OFFSET_SIZE,ECX
                MOV     CV_OFFSET_MASK,EDX
                ;
                ;CALCULATE # OF LINE #'S IN THIS RECORD
                ;
                MOV     ECX,DPTR CV_LN_LENGTH
                MOV     DL,CV_LN_TYPE

                AND     ECX,0FFFFH
                AND     DL,MASK BIT_32

                MOV     EAX,ECX
                JZ      L91$

                MOV     ECX,6
                XOR     EDX,EDX

                DIV     CX

                MOV     EDI,OFF CV_TEMP_RECORD  ;PLACE TO BUILD THIS RECORD
                MOV     ECX,EAX

                RET

L91$:
                SHR     ECX,2
                MOV     EDI,OFF CV_TEMP_RECORD  ;PLACE TO BUILD THIS RECORD

                RET

LINE_START      ENDP


CSEGMOD_START   PROC    NEAR    PRIVATE
                ;
                ;EAX IS SEGMOD
                ;
                ;RETURNS DX=OFFSET TO ADD TO EACH LINENUMBER
                ;       EDI IS PLACE TO STORE LINE#, OFFSET#, AND SRCFILE#
                ;       ESI IS VIRTUAL LINNUM RECORD ADDRESS
                ;
                MOV     ECX,CV_SEGMOD_EXPECTED
                PUSH    EBX

                CMP     ECX,EAX
                JZ      L0$

                PUSH    EAX
                CALL    HANDLE_SEGMOD_LINES                     ;FLUSH LINES CAUSE WE ARE CHANGING HERE...
                POP     EAX
L0$:
                CONVERT EAX,EAX,SEGMOD_GARRAY
                ASSUME  EAX:PTR SEGMOD_STRUCT

                MOV     ECX,[EAX]._SM_NEXT_SEGMOD_GINDEX
                MOV     EBX,[EAX]._SM_START

                MOV     CV_SEGMOD_EXPECTED,ECX
                MOV     ECX,[EAX]._SM_MODULE_CSEG_GINDEX

                MOV     EAX,[EAX]._SM_BASE_SEG_GINDEX
                MOV     EDX,CV_LSEG_GINDEX

                CMP     EDX,EAX
                JZ      L1$

                PUSH    ECX
                CALL    CSEGMENT_START

                POP     ECX
L1$:
                MOV     EAX,CV_SEG_FRAME
                CONVERT ECX,ECX,CSEG_GARRAY
                ASSUME  ECX:PTR CSEG_STRUCT

                SUB     EBX,EAX                         ;OVERALL OFFSET MAYBE MINUS FRAME
                MOV     EAX,[ECX]._CSEG_NEXT_CSEGMOD_GINDEX

                MOV     CV_LIN_FACTOR,EBX
                MOV     ECX,[ECX]._CSEG_FIRST_LINNUM

                POP     EBX

                RET

                ASSUME  EAX:NOTHING,ECX:NOTHING

CSEGMOD_START   ENDP


CSEGMENT_START  PROC    NEAR    PRIVATE
                ;
                ;EAX IS NEW SEGMENT_GINDEX
                ;
                GETT    CL,OUTPUT_PE
                MOV     EDX,EAX

                OR      CL,CL
                JZ      L5$

                CONVERT EAX,EAX,SEGMENT_GARRAY
                ASSUME  EAX:PTR SEGMENT_STRUCT

                MOV     EAX,[EAX]._SEG_PEOBJECT_NUMBER
                MOV     ECX,CV_SEG_NUMBER

                CMP     ECX,EAX
                JNZ     L2$

                MOV     CV_LSEG_GINDEX,EDX

                RET

L2$:
                PUSH    EDX
                CALL    HANDLE_SEGMOD_LINES             ;FLUSH ANY PREVIOUS

                POP     EAX

                MOV     CV_LSEG_GINDEX,EAX
                CONVERT EAX,EAX,SEGMENT_GARRAY
                ASSUME  EAX:PTR SEGMENT_STRUCT

                MOV     ECX,[EAX]._SEG_PEOBJECT_NUMBER
                MOV     EAX,[EAX]._SEG_PEOBJECT_GINDEX

                MOV     CV_SEG_NUMBER,ECX
                MOV     ECX,PE_BASE

                CONVERT EAX,EAX,PE_OBJECT_GARRAY
                ASSUME  EAX:PTR PE_OBJECT_STRUCT

                ADD     ECX,[EAX]._PEOBJECT_RVA

                MOV     CV_SEG_FRAME,ECX

                RET


L5$:
                PUSH    EDX
                CALL    HANDLE_SEGMOD_LINES             ;FLUSH ANY PREVIOUS

                POP     EAX

                MOV     CV_LSEG_GINDEX,EAX

                CONVERT EAX,EAX,SEGMENT_GARRAY
                ASSUME  EAX:PTR SEGMENT_STRUCT

                ;
                ;ALSO DEFINE SEG_INDEX FOR CV TYPE 2
                ;
                MOV     ECX,[EAX]._SEG_CV_NUMBER
                MOV     EAX,[EAX]._SEG_OFFSET

                MOV     CV_SEG_NUMBER,ECX
                MOV     CV_SEG_FRAME,EAX

                RET

CSEGMENT_START  ENDP


LINNUM_FLUSH    PROC    NEAR    PRIVATE
                ;
                ;FLUSH ALL THIS NONSENSE
                ;
                ;
                ;FIRST, FLUSH ALL SRC DESCRIPTORS
                ;
                MOV     EDX,CV_M_FIRST_SRC_NUMBER
                MOV     ECX,CV_M_SRC_COUNT
                MOV     EAX,CV_M_FIRST_SRC_GINDEX
L1$:
                PUSHM   ECX,EAX,EDX
                ;
                ;OUTPUT BASESRCLN FOR EACH LINHELP COUNT
                ;
                CALL    SET_OUTPUT_PTRS

                CALL    CV_DWORD_ALIGN

                POP     EAX
                MOV     EBX,CV_HELP_PTRS
                ASSUME  EBX:PTR HLP_STRUCT

                PUSH    EAX
                SUB     EAX,CV_M_FIRST_SRC_NUMBER

                MOV     EDX,BYTES_SO_FAR
                MOV     ECX,CV_BASEMOD

                SUB     EDX,ECX
                MOV     ESI,[EBX+EAX*8]._HLP_FIRST_LINHELP

                MOV     [EBX+EAX*8]._HLP_LAST_LINHELP,EDX
                JMP     CHECK_LINHELP

FIXIT:
                CALL    SET_NEXT_OFFSET_BLOCK

                MOV     EAX,CV_LINNUM_PTR
                MOV     ECX,CV_LINNUM_PTR_LIMIT

                CMP     EAX,ECX
                JNZ     FIXIT_RET

                CALL    SET_NEXT_LINNUM_BLOCK

                JMP     FIXIT_RET

                ASSUME  ESI:PTR SRC_LIN_HELP

LINHELP_LOOP:
                MOV     EAX,[ESI]._SLH_NEXT_HELP
                MOV     EDI,CV_OFFSET_PTR

                PUSH    EAX
                MOV     EAX,[ESI]._SLH_FIRST_OFFSET

                MOV     ECX,[ESI]._SLH_LAST_OFFSET
                MOV     [EDI],EAX

                MOV     EDX,CV_LINNUM_PTR
                MOV     [EDI+4],ECX

                ADD     EDI,8
                MOV     EAX,[ESI]._SLH_BASEMOD

                MOV     CV_OFFSET_PTR,EDI
                MOV     [EDX],EAX

                ADD     EDX,4
                MOV     EAX,CV_OFFSET_PTR_LIMIT

                MOV     CV_LINNUM_PTR,EDX
                CMP     EDI,EAX

                POP     ESI
                JZ      FIXIT
FIXIT_RET:

CHECK_LINHELP:
                TEST    ESI,ESI
                JNZ     LINHELP_LOOP
                ;
                ;OK, NEED CSEGS, PAD, ALL BASESRCLN'S, ALL START-ENDS, AND FILE NAME
                ;
                POPM    EAX,ESI

                MOV     EDI,OFF CV_TEMP_RECORD+4
                CONVERT ESI,ESI,SRC_GARRAY
                ASSUME  ESI:PTR SRC_STRUCT

                MOV     ECX,[ESI]._SRC_NEXT_GINDEX
                MOV     EBX,[ESI]._SRC_LINHELP_COUNT

                PUSHM   ECX,EAX

                MOV     [EDI-4],EBX
                CALL    FLUSH_CV_TEMP

                SHL     EBX,2                   ;LINHELP_COUNT
                LEA     ECX,CV_LINNUM_BLOCKS

                MOV     EAX,EBX

                push    ECX
                push    EAX
                call    _big_xdebug_write
                add     ESP,8

                LEA     ECX,CV_OFFSET_BLOCKS
                LEA     EAX,[EBX*2]

                push    ECX
                push    EAX
                call    _big_xdebug_write
                add     ESP,8

                MOV     EDI,OFF CV_TEMP_RECORD
                LEA     ESI,[ESI]._SRC_TEXT

                CALL    MOVE_TEXT_TO_OMF

                CALL    FLUSH_CV_TEMP

                POPM    EDX,EAX

                POP     ECX
                INC     EDX                     ;SRC # OF NEXT ONE

                DEC     ECX
                JNZ     L1$
                ;
                ;NOW NEED TO WRITE SRCMODULE HEADER RECORD
                ;
                CALL    CV_DWORD_ALIGN

                MOV     EAX,BYTES_SO_FAR
                MOV     EDX,CV_BASEMOD

                PUSH    EAX
                MOV     EAX,CV_M_CSEG_COUNT

                SHL     EAX,16
                MOV     ECX,CV_M_SRC_COUNT

                MOV     EDI,OFF CV_TEMP_RECORD+4
                MOV     ESI,CV_HELP_PTRS
                ASSUME  ESI:PTR HLP_STRUCT

                OR      EAX,ECX
                MOV     BYTES_SO_FAR,EDX

                MOV     [EDI-4],EAX
                ;
                ;WRITE CX BASESRCFILES
                ;
L5$:
                ; seg fault here <<>>
                MOV     EAX,[ESI]._HLP_LAST_LINHELP
                ADD     ESI,SIZE HLP_STRUCT

                MOV     [EDI],EAX
                ADD     EDI,4

                CMP     EDI,OFF CV_TEMP_RECORD+CV_TEMP_SIZE-4
                JA      L51$
L52$:
                DEC     ECX
                JNZ     L5$

                CALL    FLUSH_CV_TEMP
                ;
                ;NOW WRITE A START-END FOR EACH CSEGMOD
                ;
                CALL    SET_OUTPUT_PTRS

                MOV     ESI,CV_M_FIRST_CSEGMOD_GINDEX
L6$:
                CONVERT ESI,ESI,SEGMOD_GARRAY
                ASSUME  ESI:PTR SEGMOD_STRUCT

                MOV     ECX,[ESI]._SM_BASE_SEG_GINDEX           ;STORE BASE SEGMENT INDEX #
                MOV     EDI,CV_LINNUM_PTR

                MOV     EAX,[ESI]._SM_START
                MOV     EDX,ECX

                CONVERT ECX,ECX,SEGMENT_GARRAY
                ASSUME  ECX:PTR SEGMENT_STRUCT

                MOV     CV_FIRST_OFFSET,EAX
                MOV     EAX,[ECX]._SEG_CV_NUMBER

                MOV     [EDI],EAX
                ADD     EDI,2

                MOV     CV_LINNUM_PTR,EDI
                GETT    AL,OUTPUT_PE

                OR      AL,AL
                JZ      L65$

                MOV     EDI,[ECX]._SEG_PEOBJECT_GINDEX
                CALL    LFLUSH_32

                JMP     L66$

L51$:
                CALL    FLUSH_CV_TEMP
                JMP     L52$

L65$:
                CALL    LFLUSH_16
L66$:
                MOV     EDI,CV_OFFSET_PTR
                ASSUME  ECX:NOTHING

                CMP     ECX,EAX
                JZ      L62$

                DEC     ECX
L62$:
                MOV     [EDI],EAX

                MOV     [EDI+4],ECX
                ADD     EDI,8

                TEST    ESI,ESI
                MOV     CV_OFFSET_PTR,EDI

                MOV     EAX,CV_OFFSET_PTR_LIMIT
                JZ      L69$

                CMP     EDI,EAX
                JNZ     L6$

                CALL    SET_NEXT_OFFSET_BLOCK

                MOV     EAX,CV_LINNUM_PTR
                MOV     ECX,CV_LINNUM_PTR_LIMIT

                CMP     EAX,ECX
                JNZ     L6$

                CALL    SET_NEXT_LINNUM_BLOCK

                JMP     L6$

L69$:
                ;
                ;NOW FLUSH THOSE
                ;
                MOV     EAX,CV_M_CSEG_COUNT
                LEA     ECX,CV_OFFSET_BLOCKS

                SHL     EAX,3

                push    ECX
                push    EAX
                call    _big_xdebug_write
                add     ESP,8

                MOV     EAX,CV_M_CSEG_COUNT
                LEA     ECX,CV_LINNUM_BLOCKS

                ADD     EAX,EAX

                push    ECX
                push    EAX
                call    _big_xdebug_write
                add     ESP,8

                POP     EBX
                call    _cv_dword_align_rtn

                MOV     BYTES_SO_FAR,EBX

                RET

LINNUM_FLUSH    ENDP


LFLUSH_16       PROC    NEAR

                ;
                ;NOW DEFINE LAST OFFSET
                ;
                MOV     EAX,[ESI]._SM_LEN
L1$:
                MOV     ECX,[ESI]._SM_NEXT_SEGMOD_GINDEX
                MOV     ESI,[ESI]._SM_MODULE_CSEG_GINDEX

                MOV     CV_LAST_OFFSET,EAX

                CONVERT ESI,ESI,CSEG_GARRAY
                ASSUME  ESI:PTR CSEG_STRUCT

                TEST    ECX,ECX

                MOV     ESI,[ESI]._CSEG_NEXT_CSEGMOD_GINDEX
                JZ      L5$

                CMP     ECX,ESI
                JNZ     L5$

                CONVERT ESI,ESI,SEGMOD_GARRAY
                ASSUME  ESI:PTR SEGMOD_STRUCT

                MOV     EBX,[ESI]._SM_BASE_SEG_GINDEX
                MOV     EAX,[ESI]._SM_LEN

                CMP     EBX,EDX
                JZ      L1$

                MOV     ESI,ECX
L5$:
                CONVERT EDX,EDX,SEGMENT_GARRAY
                ASSUME  EDX:PTR SEGMENT_STRUCT

                MOV     EAX,CV_FIRST_OFFSET
                MOV     EBX,[EDX]._SEG_OFFSET

                MOV     ECX,CV_LAST_OFFSET
                MOV     EDX,[EDX]._SEG_OFFSET

                SUB     EAX,EBX
                SUB     ECX,EDX

                RET

LFLUSH_16       ENDP


if      fg_pe

LFLUSH_32       PROC    NEAR
                ;
                ;NOW DEFINE LAST OFFSET
                ;
                ;EDX IS SEGMENT GINDEX, EDI IS PEOBJECT GINDEX
                ;
                MOV     EAX,[ESI]._SM_LEN
L1$:
                MOV     ECX,[ESI]._SM_NEXT_SEGMOD_GINDEX
                MOV     ESI,[ESI]._SM_MODULE_CSEG_GINDEX

                MOV     CV_LAST_OFFSET,EAX

                CONVERT ESI,ESI,CSEG_GARRAY
                ASSUME  ESI:PTR CSEG_STRUCT

                TEST    ECX,ECX

                MOV     ESI,[ESI]._CSEG_NEXT_CSEGMOD_GINDEX
                JZ      L5$

                CMP     ECX,ESI
                JNZ     L5$

                CONVERT ESI,ESI,SEGMOD_GARRAY
                ASSUME  ESI:PTR SEGMOD_STRUCT

                MOV     EBX,[ESI]._SM_BASE_SEG_GINDEX
                MOV     EAX,[ESI]._SM_LEN

                CMP     EBX,EDX
                JZ      L1$

                MOV     EDX,EBX
                CONVERT EBX,EBX,SEGMENT_GARRAY
                ASSUME  EBX:PTR SEGMENT_STRUCT
                MOV     EBX,[EBX]._SEG_PEOBJECT_GINDEX

                CMP     EBX,EDI
                JZ      L1$

                MOV     ESI,ECX
L5$:
                CONVERT EDI,EDI,PE_OBJECT_GARRAY
                ASSUME  EDI:PTR PE_OBJECT_STRUCT

                MOV     EAX,CV_FIRST_OFFSET
                MOV     EBX,[EDI]._PEOBJECT_RVA

                MOV     ECX,CV_LAST_OFFSET
                SUB     EAX,EBX

                MOV     EBX,[EDI]._PEOBJECT_RVA
                MOV     EDX,PE_BASE

                SUB     ECX,EBX
                SUB     EAX,EDX

                SUB     ECX,EDX

                RET

                ASSUME  EDI:NOTHING

LFLUSH_32       ENDP

endif


HANDLE_SEGMOD_LINES     PROC    NEAR
                ;
                ;FINISHED BUILDING ARRAY OF JUNK, NOW BUILD OUTPUT ARRAY
                ;
                ;       1. SORT LINNUMS BY ADDRESS
                ;       2. BUILD OUTPUT LINENUMBER TABLES
                ;
                ;TEMP STUFF IS IN EXETABLE
                ;
                PUSH    EDI
                MOV     EAX,HIGH_WATER

                TEST    EAX,EAX
                JZ      L9$

                PUSHM   ESI,EBX

                CALL    QSORT_LINNUMS           ;RETURNS # OF LINES IN AX

                ;
                ;NOW SCAN TABLE, OUTPUTTING OFFSETS VS LINENUMBERS, PER SRC_FILE
                ;
                MOV     CV_LINE_COUNT,EAX
                CALL    SET_INPUT_PTR
                ASSUME  ESI:PTR CVL_STRUCT

                XOR     EDX,EDX
L1$:
                MOV     ECX,[ESI]._CVL_SRC_GINDEX
                MOV     EAX,[ESI]._CVL_OFFSET

                CMP     ECX,EDX                 ;CHANGING SOURCE FILE?
                JNZ     L5$

                MOV     ECX,[ESI]._CVL_LINNUM
                MOV     CV_LAST_OFFSET,EAX

                TEST    ECX,ECX
                JZ      L4$                             ;SKIP ZERO LINE NUMBER

                MOV     EBX,CV_LINNUM_PTR
                MOV     EDI,CV_OFFSET_PTR

                MOV     WPTR [EBX],CX
                ADD     EBX,2

                MOV     [EDI],EAX
                ADD     EDI,4

                MOV     CV_LINNUM_PTR,EBX
                MOV     CV_OFFSET_PTR,EDI

                MOV     ECX,CV_OFFSET_PTR_LIMIT
                MOV     EAX,CV_LINE_COUNT

                CMP     ECX,EDI
                JZ      L45$
L41$:
                ADD     ESI,SIZE CVL_STRUCT
                DEC     EAX

                MOV     CV_LINE_COUNT,EAX
                JZ      L8$

                CMP     ESI,CV_INPUT_PTR_LIMIT
                JNZ     L1$
                ;
                ;NEED TO UPDATE INPUT POINTER, AND MAYBE OUTPUT POINTERS
                ;
                PUSH    EDX
                CALL    SET_NEXT_INPUT_PTR

                POP     EDX
                JMP     L1$

L4$:
                MOV     EAX,CV_LINE_COUNT
                JMP     L41$

L9$:
                POP     EDI
                RET

L45$:
                PUSHM   EDX,EAX
                CALL    SET_NEXT_OFFSET_BLOCK

                POPM    EAX,EDX

                CMP     CV_LINNUM_PTR_LIMIT,EBX
                JNZ     L4$

                PUSHM   EDX,EAX
                CALL    SET_NEXT_LINNUM_BLOCK

                POPM    EAX,EDX
                JMP     L4$

L5$:
                ;
                ;CHANGED SOURCE  FILES.  UPDATE OLD ONE?
                ;
                TEST    EDX,EDX
                JZ      L6$                     ;JUMP IF FIRST SOURCE FILE

                PUSH    EAX
                CALL    FLUSH_PREV_SRCFILE

                POP     EAX
L6$:
                MOV     CV_FIRST_OFFSET,EAX
                CALL    SET_OUTPUT_PTRS

                MOV     EDX,[ESI]._CVL_SRC_GINDEX
                JMP     L1$

                ASSUME  ESI:NOTHING

L8$:
                MOV     ECX,CV_INPUT_BLK_PTR    ;RELEASE LAST INPUT BLOCK
                XOR     EBX,EBX

                MOV     EAX,[ECX]
                MOV     [ECX],EBX

                CALL    RELEASE_BLOCK

                CALL    FLUSH_PREV_SRCFILE      ;FLUSH ANY LINES BUFFERED

                POP     EBX
                XOR     EAX,EAX

                POPM    ESI,EDI

                MOV     HIGH_WATER,EAX

                RET                             ;GO PREPARE FOR MORE SEGMODS...

HANDLE_SEGMOD_LINES     ENDP


SET_OUTPUT_PTRS PROC    NEAR
                ;
                ;ALLOCATE AND SET UP OUTPUT POINTERS
                ;
                LEA     ECX,CV_OFFSET_BLOCKS

                MOV     CV_NEXT_OFFSET_BLK,ECX
                CALL    SET_NEXT_OFFSET_BLOCK

                LEA     ECX,CV_LINNUM_BLOCKS

                MOV     CV_NEXT_LINNUM_BLK,ECX
                JMP     SET_NEXT_LINNUM_BLOCK

SET_OUTPUT_PTRS ENDP


SET_NEXT_OFFSET_BLOCK   PROC    NEAR
                ;
                ;
                ;
                MOV     ECX,CV_NEXT_OFFSET_BLK
                CALL    GET_NEW_LOG_BLK

                MOV     [ECX],EAX
                ADD     ECX,4

                MOV     CV_OFFSET_PTR,EAX
                ADD     EAX,PAGE_SIZE

                MOV     CV_NEXT_OFFSET_BLK,ECX
                MOV     CV_OFFSET_PTR_LIMIT,EAX

                RET

SET_NEXT_OFFSET_BLOCK   ENDP


SET_NEXT_LINNUM_BLOCK   PROC    NEAR
                ;
                ;
                ;
                MOV     ECX,CV_NEXT_LINNUM_BLK
                CALL    GET_NEW_LOG_BLK

                MOV     [ECX],EAX
                ADD     ECX,4

                MOV     CV_LINNUM_PTR,EAX
                ADD     EAX,PAGE_SIZE

                MOV     CV_NEXT_LINNUM_BLK,ECX
                MOV     CV_LINNUM_PTR_LIMIT,EAX

                RET

SET_NEXT_LINNUM_BLOCK   ENDP


SET_INPUT_PTR   PROC    NEAR
                ;
                ;
                ;
                MOV     ECX,OFF EXETABLE
                JMP     SNIP_1

SET_INPUT_PTR   ENDP


SET_NEXT_INPUT_PTR      PROC    NEAR
                ;
                ;
                ;
                MOV     ECX,CV_INPUT_BLK_PTR
                XOR     EDX,EDX

                MOV     EAX,[ECX]
                MOV     [ECX],EDX

                ADD     ECX,4
                CALL    RELEASE_BLOCK

SNIP_1::
                MOV     EAX,[ECX]
                MOV     CV_INPUT_BLK_PTR,ECX

                MOV     ESI,EAX
                ADD     EAX,PAGE_SIZE

                MOV     CV_INPUT_PTR_LIMIT,EAX

                RET


SET_NEXT_INPUT_PTR      ENDP


FLUSH_LIN_TEMP  PROC    NEAR
                ;
                ;CV_TEMP_RECORD MAY CONTAIN DATA, IF SO, FLUSH IT TO EXETABLE
                ;
                MOV     ECX,EDI
                MOV     EDI,OFF CV_TEMP_RECORD

                SUB     ECX,EDI
                JZ      L9$

                MOV     EDX,HIGH_WATER
                MOV     EAX,EDI

                ADD     HIGH_WATER,ECX
                JMP     MOVE_LDATA_3            ;ES IS DGROUP ON RETURN

L9$:
                RET

FLUSH_LIN_TEMP  ENDP


FLUSH_PREV_SRCFILE      PROC    NEAR
                ;
                ;EDX IS SRC INDEX.  FLUSH LINE #S FOR THAT  FILE
                ;
                PUSHM   EDI,ESI

                PUSH    EBX
                MOV     EBX,EDX

                CALL    CV_DWORD_ALIGN

                MOV     EAX,SIZE SRC_LIN_HELP
                CALL    ALLOC_LOCAL

                CONVERT EBX,EBX,SRC_GARRAY
                ASSUME  EBX:PTR SRC_STRUCT

                MOV     EDX,[EBX]._SRC_NUMBER
                MOV     ECX,CV_M_FIRST_SRC_NUMBER

                MOV     ESI,CV_HELP_PTRS
                SUB     EDX,ECX
                ASSUME  ESI:PTR HLP_STRUCT

                MOV     EDI,EAX
                ASSUME  EDI:PTR SRC_LIN_HELP

                MOV     ECX,[ESI+EDX*8]._HLP_LAST_LINHELP
                MOV     [ESI+EDX*8]._HLP_LAST_LINHELP,EAX

                TEST    ECX,ECX
                JZ      L1$

                MOV     EDX,[EBX]._SRC_LINHELP_COUNT
                MOV     [ECX],EDI

                INC     EDX
                XOR     EAX,EAX

                MOV     [EBX]._SRC_LINHELP_COUNT,EDX
                JMP     L19$

L1$:
                MOV     EAX,1
                MOV     [ESI+EDX*8]._HLP_FIRST_LINHELP,EDI

                MOV     [EBX]._SRC_LINHELP_COUNT,EAX
                DEC     EAX
L19$:
                MOV     ECX,CV_FIRST_OFFSET
                MOV     [EDI]._SLH_NEXT_HELP,EAX                ;LINK TO NEXT

                MOV     EAX,CV_LAST_OFFSET
                MOV     [EDI]._SLH_FIRST_OFFSET,ECX

                MOV     ECX,BYTES_SO_FAR
                MOV     EDX,CV_BASEMOD

                SUB     ECX,EDX
                MOV     [EDI]._SLH_LAST_OFFSET,EAX

                MOV     [EDI]._SLH_BASEMOD,ECX
                MOV     EDI,OFF CV_TEMP_RECORD+4
                ASSUME  EDI:NOTHING
                ;
                ;OK, WRITE SEG_INDEX AND LINE_COUNT
                ;
                MOV     EAX,CV_NEXT_LINNUM_BLK
                LEA     EBX,CV_LINNUM_BLOCKS+4

                SUB     EAX,EBX
                MOV     EBX,CV_LINNUM_PTR

                SHL     EAX,PAGE_BITS-3
                SUB     EBX,CV_LINNUM_PTR_LIMIT

                ADD     EBX,PAGE_SIZE

                SHR     EBX,1
                MOV     ECX,CV_SEG_NUMBER

                SHL     ECX,16
                ADD     EAX,EBX                 ;# OF LINE NUMBERS KEPT...

                PUSH    EAX
                OR      EAX,ECX

                ROR     EAX,16

                MOV     [EDI-4],EAX
                CALL    FLUSH_CV_TEMP

                POP     EAX
                LEA     ECX,CV_OFFSET_BLOCKS

                POP     EBX
                PUSH    EAX

                SHL     EAX,2                   ;4 BYTES PER OFFSET

                push    ECX
                push    EAX
                call    _big_xdebug_write
                add     ESP,8

                POP     EAX
                LEA     ECX,CV_LINNUM_BLOCKS

                ADD     EAX,EAX                 ;2 BYTES PER LINNUM

                push    ECX
                push    EAX
                call    _big_xdebug_write
                add     ESP,8

                POPM    ESI,EDI

                JMP     CV_DWORD_ALIGN

FLUSH_PREV_SRCFILE      ENDP


                END

*/
