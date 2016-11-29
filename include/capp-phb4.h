/* Copyright 2013-2014 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* CAPP/PSL Lid Format */
struct capp_lid_hdr {
	be64 eyecatcher;	/* 'CAPPPSLL' in ASCII */
	be64 version;
	be64 lid_no;
	be64 pad;
	be64 ucode_offset;
	be64 total_size;
};

/* CAPP Microcode Data Format */
struct capp_ucode_data_hdr {
	be64 eyecatcher;  	/* 'CAPPUCOD' in ASCII */
	u8 version;
	u8 reg;
	u8 reserved[2];
	be32 chunk_count;	/* Num of 8-byte chunks that follow */
};

struct capp_ucode_data {
	struct capp_ucode_data_hdr hdr;
	be64 data[];
};

struct capp_ucode_lid {
	be64 eyecatcher;	/* 'CAPPULID' in ASCII */
	be64 version;
	be64 data_size;		/* Total size of all capp microcode data */
	u8 reserved[40];
	struct capp_ucode_data data; /* This repeats */
};


enum capp_reg {
	apc_master_cresp		= 0x1,
	apc_master_uop_table		= 0x2,
	snp_ttype			= 0x3,
	snp_uop_table			= 0x4,
	apt_master_capi_ctrl		= 0x5,
	snoop_capi_cnfg			= 0x6,
	canned_presp_map0		= 0x7,
	canned_presp_map1		= 0x8,
	canned_presp_map2		= 0x9,
	flush_sue_state_map		= 0xA,
	apc_master_powerbus_ctrl	= 0xB
};

#define CAPP_SNP_ARRAY_WRITE_REG		0x2010841  /* S2 */
#define CAPP_SNP_ARRAY_ADDR_REG			0x2010828
#define CAPP_APC_MASTER_ARRAY_ADDR_REG		0x201082A
#define CAPP_APC_MASTER_ARRAY_WRITE_REG		0x2010842  /* S2 */

#define APC_MASTER_PB_CTRL			0x2010818
#define APC_MASTER_CAPI_CTRL			0x2010819
#define LCO_MASTER_TARGET			0x2010821
#define EPOCH_RECOVERY_TIMERS_CTRL		0x201082C
#define SNOOP_CAPI_CONFIG			0x201081A
#define SNOOP_CONTROL				0x201081B
#define TRANSPORT_CONTROL			0x201081C
#define CANNED_PRESP_MAP0			0x201081D
#define CANNED_PRESP_MAP1			0x201081E
#define CANNED_PRESP_MAP2			0x201081F
#define CAPP_TFMR				0x2010827
#define CAPP_ERR_STATUS_CTRL			0x201080E
#define FLUSH_SUE_STATE_MAP			0x201080F
#define FLUSH_CPIG_STATE_MAP			0x2010820  /* TBD */
#define FLUSH_SUE_UOP1				0x2010843  /* S2 */
#define APC_FSM_READ_MASK			0x2010823
#define XPT_FSM_RMM				0x2010831

/* Milan has two CAPP units, statically mapped:
 * CAPP0 attached to PHB0(PEC0 - single port), and CAPP1 attached to 
 * PHB3(PEC2 - single or dual port)
 * 
 * SCOM address Base (Ring = ‘0010’b)
 * CAPP Unit Satellite           SCOM address Base
 * CAPP 0    S1 (sat = ‘0000’b)  x02010800
 * CAPP 0    S2 (sat = ‘0001’b)  x02010840
 * CAPP 1    S1 (sat = ‘0000’b)  x04010800
 * CAPP 1    S2 (sat = ‘0001’b)  x04010840
*/
#define CAPP1_REG_OFFSET 0x2000000

#define PHB4_CAPP_MAX_PHB_INDEX 3

#if 0
#define PHB4_CAPP_REG_OFFSET(p) ((p)->index == 0 ? 0x0: CAPP1_REG_OFFSET)
#else /* for simics - Address for CAPP1 not defined ??? */
#define PHB4_CAPP_REG_OFFSET(p) ((p)->index == 0 ? 0x0: 0x0)
#endif
