/* Copyright 2013-2017 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __PHB4_CAPP_H
#define __PHB4_CAPP_H

#define CAPP_SNP_ARRAY_WRITE_REG		0x2010841  /* S2 */
#define CAPP_SNP_ARRAY_ADDR_REG 		0x2010828
#define CAPP_APC_MASTER_ARRAY_ADDR_REG		0x201082A
#define CAPP_APC_MASTER_ARRAY_WRITE_REG 	0x2010842  /* S2 */

#define APC_MASTER_PB_CTRL			0x2010818
#define APC_MASTER_CAPI_CTRL			0x2010819
#define LCO_MASTER_TARGET			0x2010821
#define EPOCH_RECOVERY_TIMERS_CTRL		0x201082C
#define SNOOP_CAPI_CONFIG			0x201081A
#define SNOOP_CONTROL				0x201081B
#define TRANSPORT_CONTROL			0x201081C
#define CAPP_TB 				0x2010826
#define CAPP_TFMR				0x2010827
#define CAPP_ERR_STATUS_CTRL			0x201080E
#define FLUSH_SUE_STATE_MAP			0x201080F
#define FLUSH_CPIG_STATE_MAP			0x2010820  /* TBD */
#define FLUSH_SUE_UOP1				0x2010843  /* S2 */
#define APC_FSM_READ_MASK			0x2010823
#define XPT_FSM_RMM				0x2010831

/* CAPP0 attached to PHB0(PEC0 - single port)
 * CAPP1 attached to PHB3(PEC2 - single or dual port)
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

#define PHB4_CAPP_REG_OFFSET(p) ((p)->index == 0 ? 0x0 : CAPP1_REG_OFFSET)

#endif /* __PHB4_CAPP_H */
