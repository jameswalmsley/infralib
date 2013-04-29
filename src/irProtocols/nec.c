/*
 * nec.c
 *
 *  Created on: 18 Mar 2011
 *      Author: James
 */

#include "../irManager.h"
#include "nec.h"
#include <string.h>

static int	g_bIsDataReady = 0;

static const IR_PROTOCOL g_irNecProtocol;

static IR_DATA_ITEM g_irDataDescriptor[] = {
	{
		.szpName 	= "ADDRESS",
		.eType		= IR_T_SHORT,
	},
	{
		.szpName	= "COMMAND",
		.eType		= IR_T_SHORT,
	},
};

static int IR_NEC_GetData(IR_DATA *poData) {

	if(!g_bIsDataReady) {
		return -1;
	}

	poData->nItems 			= (sizeof(g_irDataDescriptor) / sizeof(IR_DATA_ITEM));
	poData->poItems			= g_irDataDescriptor;
	poData->szpProtocolName = g_irNecProtocol.szpName;
	poData->eIdent			= IR_PROTOCOL_NEC;

	g_bIsDataReady = 0;

	return 0;
}

static int IR_NecRx(IR_WAVEFORM *poWaveform) {

	int i, byte;
	uint8_t data[4];

	for(byte = 0; byte < 4; byte++) {
		for(i = 0; i < 8; i++) {
			if(poWaveform->arrWavePoints[(byte * 8) + i + 1].usPeriodUs >= 2150) {
				data[byte] |= (0x01 << i);
			} else {
				data[byte] &= ~(0x01 << i);
			}
		}
	}

	if((data[0] & 0xff) == (~(data[1]) & 0xff)) {
			g_irDataDescriptor[0].unData.u16 = (uint16_t) data[0];
		} else {
			g_irDataDescriptor[0].unData.u16 = (uint16_t) data[0] | ((uint16_t) data[1] << 8);
		}
		if((data[2] & 0xff) == (~(data[3]) & 0xff)) {
			g_irDataDescriptor[1].unData.u16 = (uint16_t) data[2];
		} else {
			g_irDataDescriptor[1].unData.u16 = (uint16_t) data[2] | ((uint16_t) data[3] << 8);
	}

	g_bIsDataReady = 1;

	return 0;
}

static int IR_NecCanHandle(IR_WAVEFORM *poWaveform) {
	uint16_t usPeriod, usDutyCycle;

	// Test lead-in signature.

	usPeriod = poWaveform->arrWavePoints[0].usPeriodUs;
	usDutyCycle = poWaveform->arrWavePoints[0].usDutyCycleUs;

	if((usPeriod >= 13375 && usPeriod <= 13625) && usDutyCycle >= 8875 && usDutyCycle <= 9125) {
		// Do the bit lengths match?

		// Now check the length, is this NEC or something proprietary.
		if(poWaveform->usWavePoints != 34) {	// 32 bits + leading. + end.
			return 0;
		}
		return 1;
	}

	return 0;
}

static const IR_PROTOCOL g_irNecProtocol = {
	.pfnProtocolHandler = IR_NecRx,
	.pfnCanHandle		= IR_NecCanHandle,
	.pfnGetData			= IR_NEC_GetData,
	.pfnGetDataObject	= NULL,
	.pfnEncode			= NULL,
	.ulCarrierFrequency	= 38000,
	.szpName			= "NEC",
	.szpVersionString 	= "v1.0.0 (C)2011 James Walmsley",
};

const IR_PROTOCOL *IR_NEC_GetHandlerInfo(void) {
	return &g_irNecProtocol;
}
