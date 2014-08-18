// terminal.c

/*============================ INCLUDES ======================================*/
#include ".\app_cfg.h"

#if USE_SERVICE_GUI_TGUI == ENABLED
#include "..\grid\interface.h"

/*============================ MACROS ========================================*/
#define TGUI_TERMINAL_CLEAR_CODE	0x0C

#define WIDTH   80
#define HEIGHT  24

// 写数据插入宏
#ifndef TGUI_TERMINAL_WRITE_BYTE
	#error No defined TGUI_TERMINAL_WRITE_BYTE
#endif

// 读取数据插入宏
#ifndef TGUI_TERMINAL_READ_BYTE
    #error No defined TGUI_TERMINAL_READ_BYTE
#endif

/*============================ MACROFIED FUNCTIONS ===========================*/

/*============================ TYPES =========================================*/

// 字符串发送数据结构
typedef struct {
    enum {
        TERMINAL_PRN_STR_START = 0,
        TERMINAL_PRN_STR_CHECK,
        TERMINAL_PRN_STR_SEND
    } tState;                   // 状态变量
    uint8_t *pchStr;            // 待发送字符首地址
    uint_fast16_t hwSize;       // 待发送字符个数
} terminal_prn_str_t;

/*============================ GLOBAL VARIABLES ==============================*/
/*============================ LOCAL VARIABLES ===============================*/
static grid_brush_t s_tCurrentGridBrush;

/*============================ PROTOTYPES ====================================*/
static bool terminal_init_prn_str(terminal_prn_str_t *ptPRN, uint8_t *pchStr, uint_fast16_t hwSize);
static fsm_rt_t terminal_prn_str(terminal_prn_str_t *ptPRN);

/*============================ IMPLEMENTATION ================================*/

// terminal_set_grid 设置光标位置函数
// 参数:    tGrid   -- 光标位置
// 返回值:  状态机运行状态
#define TERMINAL_SET_GRID_RESET()    do { s_tState = TERMINAL_SET_GRID_START; } while(0)
fsm_rt_t terminal_set_grid(grid_t tGrid)
{
    static enum {
        TERMINAL_SET_GRID_START = 0,
        TERMINAL_SET_GRID_SEND
    } s_tState = TERMINAL_SET_GRID_START;
    uint8_t s_chRow, s_chColumn;
    static uint8_t s_chSetCode[8];
    static terminal_prn_str_t s_tPrn;
    fsm_rt_t tfsm;
    
    switch ( s_tState ) {
        case TERMINAL_SET_GRID_START:
            s_chRow = HEIGHT - tGrid.chTop;
            s_chColumn = WIDTH - tGrid.chLeft;
            s_chSetCode[0] = 0x1B;
            s_chSetCode[1] = '[';
            s_chSetCode[2] = ( s_chRow / 10 ) + 30;
            s_chSetCode[3] = ( s_chRow % 10 ) + 30;
            s_chSetCode[4] = ';';
            s_chSetCode[5] = ( s_chColumn / 10 ) + 30;
            s_chSetCode[6] = ( s_chColumn % 10 ) + 30;
            s_chSetCode[7] = 'H';
            terminal_init_prn_str(&s_tPrn, s_chSetCode, 8);
            s_tState = TERMINAL_SET_GRID_SEND;
            // break;
        case TERMINAL_SET_GRID_SEND:
            tfsm = terminal_prn_str(&s_tPrn);
            if ( IS_FSM_ERR(tfsm) ) {
                return tfsm;
            } else if ( fsm_rt_cpl == tfsm ) {
                TERMINAL_SET_GRID_RESET();
                return tfsm;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

// terminal_get_grid 获取光标位置函数
// 参数:    ptGrid  -- 返回光标位置的指针
// 返回值:  状态机运行状态
#define TERMINAL_GET_GRID_RESET()    do { s_tState = TERMINAL_GET_GRID_START; } while(0)
fsm_rt_t terminal_get_grid(grid_t *ptGrid)
{
    static enum {
        TERMINAL_GET_GRID_START = 0,
        TERMINAL_GET_GRID_SEND,
        TERMINAL_GET_GRID_RECEIVE
    } s_tState = TERMINAL_GET_GRID_START;
    
    static uint8_t s_chCmdCode[4];
    static uint8_t s_chReceiveCode[8];
    static uint8_t s_chReceiveCnt;
    uint8_t s_chRow, s_chColumn;
    static terminal_prn_str_t s_tPrn;
    fsm_rt_t tfsm;

	if ( NULL == ptGrid ) {
		return fsm_rt_err;
	}
    
    switch ( s_tState ) {
        case TERMINAL_GET_GRID_START:
            s_chCmdCode[0] = 0x1B;
            s_chCmdCode[1] = '[';
            s_chCmdCode[2] = '6';
            s_chCmdCode[3] = 'n';
            terminal_init_prn_str(&s_tPrn, s_chCmdCode, 4);
            s_tState = TERMINAL_GET_GRID_SEND;
            // break;
        case TERMINAL_GET_GRID_SEND:
            tfsm = terminal_prn_str(&s_tPrn);
            if ( IS_FSM_ERR(tfsm) ) {
                return tfsm;
            } else if ( fsm_rt_cpl == tfsm ) {
                s_chReceiveCnt = 0;
                s_tState = TERMINAL_GET_GRID_RECEIVE;
            }
            break;
        case TERMINAL_GET_GRID_RECEIVE: {
            uint8_t chTemp;
            if ( TGUI_TERMINAL_READ_BYTE(&chTemp) ) {
                s_chReceiveCode[s_chReceiveCnt] = chTemp;
                s_chReceiveCnt++;
                if ( 'R' == chTemp ) {
                    
                    // 未完成
                }
            }
            break;
        }
    }
    
    return fsm_rt_on_going;
}

// terminal_save_current 保存光标位置函数
// 返回值:  状态机运行状态
#define TERMINAL_SAVE_CURRENT_RESET()   do { s_tState = TERMINAL_SAVE_CURRENT_START; } while(0)
fsm_rt_t terminal_save_current(void)
{
    static enum {
        TERMINAL_SAVE_CURRENT_START = 0,
        TERMINAL_SAVE_CURRENT_SEND
    } s_tState = TERMINAL_SAVE_CURRENT_START;
    static uint8_t s_chSaveCode[3];
    static terminal_prn_str_t s_tPrn;
    fsm_rt_t tfsm;
    
    switch ( s_tState ) {
        case TERMINAL_SAVE_CURRENT_START:
            s_chSaveCode[0] = 0x1B;
            s_chSaveCode[1] = '[';
            s_chSaveCode[2] = 's';
            terminal_init_prn_str(&s_tPrn, s_chSaveCode, 3);
            s_tState = TERMINAL_SAVE_CURRENT_SEND;
            // break;
        case TERMINAL_SAVE_CURRENT_SEND:
            tfsm = terminal_prn_str(&s_tPrn);
            if ( IS_FSM_ERR(tfsm) ) {
                return tfsm;
            } else if ( fsm_rt_cpl == tfsm ) {
                TERMINAL_SAVE_CURRENT_RESET();
                return tfsm;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

// terminal_resume 恢复保存的光标位置函数
// 返回值:  状态机运行状态
#define TERMINAL_RESUME_RESET() do { s_tState = TERMINAL_RESUME_START; } while(0)
fsm_rt_t terminal_resume(void)
{
    static enum {
        TERMINAL_RESUME_START = 0,
        TERMINAL_RESUME_SEND
    } s_tState = TERMINAL_RESUME_START;
    static uint8_t s_chResumeCode[3];
    static terminal_prn_str_t s_tPrn;
    fsm_rt_t tfsm;
    
    switch ( s_tState ) {
        case TERMINAL_RESUME_START:
            s_chResumeCode[0] = 0x1B;
            s_chResumeCode[1] = '[';
            s_chResumeCode[2] = 'u';
            terminal_init_prn_str(&s_tPrn, s_chResumeCode, 3);
            s_tState = TERMINAL_RESUME_SEND;
            break;
        case TERMINAL_RESUME_SEND:
            tfsm = terminal_prn_str(&s_tPrn);
            if ( IS_FSM_ERR(tfsm) ) {
                return tfsm;
            } else if ( fsm_rt_cpl == tfsm ) {
                TERMINAL_RESUME_RESET();
                return tfsm;
            }
            break;
    }
    
    return fsm_rt_on_going;
}

// terminal_set_brush 设置颜色属性函数
// 参数:	tBrush	-- 颜色属性
// 返回值:  状态机运行状态
#define TERMINAL_SET_BRUSH_RESET()	do { s_tState = TERMINAL_SET_BRUSH_START; }	while(0)
fsm_rt_t terminal_set_brush(grid_brush_t tBrush)
{
	static enum {
		TERMINAL_SET_BRUSH_START = 0,
		TERMINAL_SET_BRUSH_SEND
	} s_tState = TERMINAL_SET_BRUSH_START;
	static uint8_t s_chCmdCode[8];
	static terminal_prn_str_t s_tPrn;
    fsm_rt_t tfsm;
	
	switch ( s_tState ) {
		case TERMINAL_SET_BRUSH_START:
			s_tCurrentGridBrush = tBrush;
			if ( ( tBrush.tForeground.tValue > 7 ) || ( tBrush.tBackground.tValue > 7 ) ) {
				return fsm_rt_err;
			}
			s_chCmdCode[0] = 0x1B;
			s_chCmdCode[1] = '[';
			s_chCmdCode[2] = '3';
			s_chCmdCode[3] = tBrush.tForeground.tValue + 30;
			s_chCmdCode[4] = ';';
			s_chCmdCode[5] = '4';
			s_chCmdCode[6] = tBrush.tBackground.tValue + 30;
			s_chCmdCode[7] = 'm';
			terminal_init_prn_str(&s_tPrn, s_chCmdCode, 8);
			break;
		case TERMINAL_SET_BRUSH_SEND:
			tfsm = terminal_prn_str(&s_tPrn);
			if ( IS_FSM_ERR(tfsm) ) {
				return tfsm;
			} else if ( fsm_rt_cpl == tfsm ) {
				TERMINAL_SET_BRUSH_RESET();
				return tfsm;
			}
			break;
	}

	return fsm_rt_on_going;
}

// terminal_get_brush 获取颜色属性函数
// 返回值:  状态机运行状态
grid_brush_t terminal_get_brush(void)
{
	return s_tCurrentGridBrush;
}

// terminal_clear 清屏函数
fsm_rt_t terminal_clear(void)
{
	if ( TGUI_TERMINAL_WRITE_BYTE(TGUI_TERMINAL_CLEAR_CODE) ) {
		return fsm_rt_cpl;
	}
	return fsm_rt_on_going;
}

// terminal_prn_str 字符串输出函数
// 参数:    ptPRN   -- 数据结构指针
// 返回值:  状态机运行状态
#define TERMINAL_PRN_STR_RESET()     do { ptPRN->tState = TERMINAL_PRN_STR_START; } while(0)

static fsm_rt_t terminal_prn_str(terminal_prn_str_t *ptPRN)
{
    if ( NULL == ptPRN ) {
        return fsm_rt_err;
    }
    
    switch( ptPRN->tState ) {
        case TERMINAL_PRN_STR_START:
            if( ( NULL == ptPRN->pchStr ) || ( 0 == ptPRN->hwSize ) ) {
				return fsm_rt_err;              // 参数错误
            } else {
                ptPRN->tState = TERMINAL_PRN_STR_SEND;
            }
            break;
        case TERMINAL_PRN_STR_SEND:
            if( TGUI_TERMINAL_WRITE_BYTE(*(ptPRN->pchStr)) ) {
                ptPRN->pchStr++;
				ptPRN->hwSize--;
                ptPRN->tState = TERMINAL_PRN_STR_CHECK;
            }
            break;
		case TERMINAL_PRN_STR_CHECK:
			if ( 0 == ptPRN->hwSize ) {
				TERMINAL_PRN_STR_RESET();
				return fsm_rt_cpl;
			}
			ptPRN->tState = TERMINAL_PRN_STR_SEND;
			break;
    }
    return fsm_rt_on_going;
}

// terminal_init_prn_str 发送字符串初始化函数
static bool terminal_init_prn_str(terminal_prn_str_t *ptPRN, uint8_t *pchStr, uint_fast16_t hwSize)
{
    if ( (NULL == ptPRN) || (NULL == pchStr) ) {
        return false;
    }
    ptPRN->tState = TERMINAL_PRN_STR_START;
    ptPRN->pchStr = pchStr;
    ptPRN->hwSize = hwSize;
    return true;
}


#endif  /* USE_SERVICE_GUI_TGUI == ENABLED */

/* EOF */
