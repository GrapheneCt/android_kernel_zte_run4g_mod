
#include "wmt_idc.h"
#include "wmt_lib.h"

#if CFG_WMT_LTE_COEX_HANDLING

MTK_WCN_WMT_IDC_INFO gWmtIdcInfo;

INT32 wmt_idc_msg_from_lte_handing(ipc_ilm_t *ilm);

INT32 wmt_idc_init(VOID)
{
	INT32 iRet;
	
	osal_memset(&gWmtIdcInfo,0,osal_sizeof(gWmtIdcInfo));
	gWmtIdcInfo.iit.src_mod_id = AP_MOD_WMT;
	gWmtIdcInfo.iit.dest_mod_id = MD_MOD_EL1;
	gWmtIdcInfo.iit.sap_id = 0;
	gWmtIdcInfo.ops.rx_cb = wmt_idc_msg_from_lte_handing;

	iRet = mtk_conn_md_bridge_reg(gWmtIdcInfo.iit.src_mod_id, &gWmtIdcInfo.ops);
	if(iRet)
	{
		WMT_ERR_FUNC("mtk_conn_md_bridge_reg fail(%d)\n",iRet);
		return -1;
	}

	//mtk_wcn_stp_flush_rx_queue(COEX_TASK_INDX);
	return 0;
	
}


INT32 wmt_idc_deinit(VOID)
{
	INT32 iRet;
	
	iRet = mtk_conn_md_bridge_unreg(gWmtIdcInfo.iit.src_mod_id);
	if(iRet)
	{
		WMT_ERR_FUNC("mtk_conn_md_bridge_unreg fail(%d)\n",iRet);
	}

	osal_memset(&gWmtIdcInfo,0,osal_sizeof(gWmtIdcInfo));

	return 0;
}

INT32 wmt_idc_msg_from_lte_handing(ipc_ilm_t *ilm)
{
	MTK_WCN_BOOL bRet;

	if(NULL == ilm)
	{
		WMT_ERR_FUNC("NULL pointer\n");
		return -1;
	}
	
	bRet = wmt_lib_handle_idc_msg(ilm);
	if(MTK_WCN_BOOL_FALSE == bRet)
	{
		WMT_ERR_FUNC("wmt handing idc msg fail\n");
		return -2;
	}
	wmt_lib_notify_stp_sleep();

	return 0;
}

VOID wmt_idc_dump_debug_msg(UINT8 *str,UINT8 *p_buf,UINT32 buf_len)
{
	UINT32 idx = 0;
	printk("%s:, length:%d\n", str, buf_len);

	printk("ASCII output:\n");
	
	for (idx = 0; idx < buf_len;) {
		printk("%c", p_buf[idx]);
		idx++;
		if (0 == idx % 16)
			printk("\n");
	}

	printk("HEX output:\n");
	
	for (idx = 0; idx < buf_len;) {
		printk("%02x ", p_buf[idx]);
		idx++;
		if (0 == idx % 16)
			printk("\n");
	}
}

INT32 wmt_idc_msg_to_lte_handing(VOID)
{
	UINT32 readlen = 0;
	local_para_struct * p_lps = NULL;
	UINT8 *p_data = NULL;
	UINT8 opcode = 0;
	UINT16 msg_len = 0;
	UINT32 handle_len = 0;
	
	readlen = mtk_wcn_stp_receive_data(&gWmtIdcInfo.buffer[0], LTE_IDC_BUFFER_MAX_SIZE, COEX_TASK_INDX);
	if(readlen == 0)
	{
		osal_sleep_ms(5);
		readlen = mtk_wcn_stp_receive_data(&gWmtIdcInfo.buffer[0], LTE_IDC_BUFFER_MAX_SIZE, COEX_TASK_INDX);
	}

	if(readlen > 0)
	{
		WMT_DBG_FUNC("read data len from fw(%d)\n",readlen);
		wmt_idc_dump_debug_msg("WMT->LTE from STP buffer",&gWmtIdcInfo.buffer[0],readlen);
		p_data = &gWmtIdcInfo.buffer[0];
		
		while(handle_len < readlen)
		{
			p_data += 2;/*omit direction & opcode 2 bytes*/
			osal_memcpy(&msg_len,p_data,2);
			msg_len -= 1;/*flag byte*/
			WMT_DBG_FUNC("current raw data len(%d) from connsys firmware\n",msg_len);

			p_data += 2;/*length: 2 bytes*/

			/*how to handle flag(msg type) need to Scott comment*/
			/************************************************/

			if(*p_data == WMT_IDC_RX_OPCODE_DEBUG_MONITOR)
			/*do not need transfer to LTE*/
			{
				p_data += 1;/*flag : 1 byte*/
				/*need to handle these debug message*/
				wmt_idc_dump_debug_msg("WIFI DEBUG MONITOR",p_data,msg_len);
			}
			else
			/*need to transfer to LTE*/
			{
				p_lps = (local_para_struct *)osal_malloc(osal_sizeof(local_para_struct) + osal_sizeof(UINT8) * msg_len);
				if(NULL == p_lps)
				{
					WMT_ERR_FUNC("allocate local_para_struct memory fail\n");
					return -1;
				}
			
				p_lps->msg_len = msg_len + osal_sizeof(local_para_struct);

				opcode = *p_data;
				WMT_DBG_FUNC("current opcode(%d) to LTE\n",opcode);
				
				p_data += 1;/*flag : 1 byte*/
				osal_memcpy(p_lps->data,p_data,msg_len);

				gWmtIdcInfo.iit.local_para_ptr = p_lps;

				switch(opcode)
				{
					case WMT_IDC_RX_OPCODE_BTWF_DEF_PARA:
						gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_OPER_DEFAULT_PARAM_IND;
						break;
					case WMT_IDC_RX_OPCODE_BTWF_CHAN_RAN:
						gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_OPER_FREQ_IND;
						break;
					case WMT_IDC_RX_OPCODE_LTE_FREQ_IDX_TABLE:
						gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_FREQ_IDX_TABLE_IND;
						break;
					case WMT_IDC_RX_OPCODE_BTWF_PROFILE_IND:
						gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_PROFILE_IND;
						break;
					//case WMT_IDC_RX_OPCODE_TDM_REQ:
					//	gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_OPER_FREQ_IND;
					//	break;
					default:
						WMT_ERR_FUNC("unknow opcode(%d) from connsys firmware\n",opcode);
						break;
				}
				
				/*handling flag value in wmt cmd*/
				mtk_conn_md_bridge_send_msg(&gWmtIdcInfo.iit);
				
				osal_free(p_lps);
			}

			p_data += msg_len; /*point to next package header*/

			handle_len += (msg_len + 5);
		}
		
	}else
	{
		WMT_ERR_FUNC("there is no coex data in stp buffer\n");
	}

	osal_memset(&gWmtIdcInfo.buffer[0],0,LTE_IDC_BUFFER_MAX_SIZE);	

	return 0;
}

UINT32 wmt_idc_msg_to_lte_handing_for_test(UINT8 *p_buf,UINT32 len)
{
	UINT32 readlen = len;
	local_para_struct * p_lps = NULL;
	UINT8 *p_data = NULL;
	UINT8 opcode = 0;
	UINT16 msg_len = 0;
	UINT32 handle_len = 0;

	osal_memcpy(&gWmtIdcInfo.buffer[0],p_buf,len);

	if(readlen > 0)
	{
		WMT_INFO_FUNC("read data len from fw(%d)\n",readlen);
		p_data = &gWmtIdcInfo.buffer[0];
		
		while(handle_len < readlen)
		{
			p_data += 2;/*omit direction & opcode 2 bytes*/
			osal_memcpy(&msg_len,p_data,2);
			msg_len -= 1;/*flag byte*/
			WMT_INFO_FUNC("current raw data len(%d) from connsys firmware\n",msg_len);

			p_data += 2;/*length: 2 bytes*/

			/*how to handle flag(msg type) need to Scott comment*/
			/************************************************/

			if(*p_data == WMT_IDC_RX_OPCODE_DEBUG_MONITOR)
			/*do not need transfer to LTE*/
			{
				p_data += 1;/*flag : 1 byte*/
				/*need to handle these debug message*/
				wmt_idc_dump_debug_msg("WIFI DEBUG MONITOR",p_data,msg_len);
			}
			else
			/*need to transfer to LTE*/
			{
				p_lps = (local_para_struct *)osal_malloc(osal_sizeof(local_para_struct) + osal_sizeof(UINT8) * msg_len);
				if(NULL == p_lps)
				{
					WMT_ERR_FUNC("allocate local_para_struct memory fail\n");
					return -1;
				}
			
				p_lps->msg_len = msg_len + osal_sizeof(local_para_struct);

				opcode = *p_data;
				WMT_INFO_FUNC("current opcode(%d) to LTE\n",opcode);
				
				p_data += 1;/*flag : 1 byte*/
				osal_memcpy(p_lps->data,p_data,msg_len);

				gWmtIdcInfo.iit.local_para_ptr = p_lps;

				switch(opcode)
				{
					case WMT_IDC_RX_OPCODE_BTWF_DEF_PARA:
						gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_OPER_DEFAULT_PARAM_IND;
						break;
					case WMT_IDC_RX_OPCODE_BTWF_CHAN_RAN:
						gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_OPER_FREQ_IND;
						break;
					//case WMT_IDC_RX_OPCODE_TDM_REQ:
					//	gWmtIdcInfo.iit.msg_id = IPC_MSG_ID_EL1_WIFIBT_OPER_FREQ_IND;
					//	break;
					default:
						WMT_ERR_FUNC("unknow opcode(%d) from connsys firmware\n",opcode);
						break;
				}
				
				/*handling flag value in wmt cmd*/
				mtk_conn_md_bridge_send_msg(&gWmtIdcInfo.iit);
				
				osal_free(p_lps);
			}

			p_data += msg_len; /*point to next package header*/

			handle_len += (msg_len + 5);
		}
		
	}else
	{
		WMT_ERR_FUNC("there is no coex data in stp buffer\n");
	}

	osal_memset(&gWmtIdcInfo.buffer[0],0,LTE_IDC_BUFFER_MAX_SIZE);	

	return handle_len;
}

#endif

