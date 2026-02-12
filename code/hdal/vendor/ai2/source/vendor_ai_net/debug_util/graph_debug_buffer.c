/**
	@brief Source file of graph debug buffer.

	@file graph_debug_buffer.c

	@ingroup graph_debug

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "graph_debug_core.h"
#include "graph_debug_buffer.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Macros Declarations                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                         */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

INT graph_debug_buffer_open(CHAR *filepath, GRAPH_DEBUG_BUFFER_HANDLER *handler)
{
	GRAPH_DEBUG_CORE_OPEN_CFG open_cfg = {0};
	
	graph_debug_core_set_default_open_cfg(&open_cfg);
	open_cfg.show_detail_range_x = 1;
	open_cfg.show_detail_range_y = 1;
	return graph_debug_core_open(&open_cfg, filepath, handler);
}

INT graph_debug_buffer_add_block(GRAPH_DEBUG_BUFFER_HANDLER handler, UINT32 x1, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len)
{
	CHAR color_name[8] = "#00FF00";  // green
	return graph_debug_core_add_block(handler, x1, x2, y1, y2, block_name, max_block_name_len, color_name, 8);
}

INT graph_debug_buffer_set_var(GRAPH_DEBUG_BUFFER_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 value)
{
	return graph_debug_core_set_var(handler, var_name, max_var_name_len, value);
}

INT graph_debug_buffer_add_block_with_var_x1(GRAPH_DEBUG_BUFFER_HANDLER handler, CHAR *var_name, UINT max_var_name_len, UINT32 x2, UINT32 y1, UINT32 y2, CHAR *block_name, UINT max_block_name_len)
{
	CHAR color_name[8] = "#00FF00";  // green
	return graph_debug_core_add_block_with_var_x1(handler, var_name, max_var_name_len, x2, y1, y2, block_name, max_block_name_len, color_name, 8);
}

INT graph_debug_buffer_add_block_with_var_y1(GRAPH_DEBUG_BUFFER_HANDLER handler, UINT32 x1, UINT32 x2, CHAR *var_name, UINT max_var_name_len, UINT32 y2, CHAR *block_name, UINT max_block_name_len)
{
	CHAR color_name[8] = "#00FF00";  // green
	return graph_debug_core_add_block_with_var_y1(handler, x1, x2, var_name, max_var_name_len, y2, block_name, max_block_name_len, color_name, 8);
}

INT graph_debug_buffer_close(GRAPH_DEBUG_BUFFER_HANDLER handler)
{
	GRAPH_DEBUG_CORE_CLOSE_CFG close_cfg = {0};
	CHAR x_name[16] = "layer";
	CHAR y_name[16] = "memory_addr";

	close_cfg.x_axis.axis_name         = x_name;
	close_cfg.x_axis.max_axis_name_len = 16;
	close_cfg.x_axis.b_show_range      = 0;

	close_cfg.y_axis.axis_name         = y_name;
	close_cfg.y_axis.max_axis_name_len = 16;
	close_cfg.y_axis.b_show_range      = 1;
	
	return graph_debug_core_close(handler, &close_cfg);
}

