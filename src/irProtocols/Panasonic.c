/*
 * Panasonic.c
 *
 *  Created on: 18 Mar 2011
 *      Author: James
 */

#include "../irManager.h"
#include "Panasonic.h"
#include <string.h>

//static uint16_t g_usPeriods[48];
//static uint16_t g_bit = 0;
static int		g_bIsDataReady = 0;

static const IR_PROTOCOL g_irPanasonicProtocol;

static IR_DATA_ITEM g_irDataDescriptor[] = {
	{
		.szpName 	= "Byte1",
		.eType		= IR_T_CHAR,
	},
	{
		.szpName	= "Byte2",
		.eType		= IR_T_CHAR,
	},
	{
		.szpName	= "Byte3",
		.eType		= IR_T_CHAR,
	},
	{
		.szpName	= "Byte4",
		.eType		= IR_T_CHAR,
	},
	{
		.szpName		= "Byte5",
		.eType		= IR_T_CHAR,
	}
};

int IR_PANASONIC_GetData(IR_DATA *poData) {
	if(g_bIsDataReady) {
		if(poData) {
			poData->nItems 			= (sizeof(g_irDataDescriptor) / sizeof(IR_DATA_ITEM));
			poData->poItems 		= g_irDataDescriptor;
			poData->szpProtocolName	= g_irPanasonicProtocol.szpName;
		}

		g_bIsDataReady = 0;

		return 0;
	}
	return -1;
}

static int IR_PanasonicRx(IR_WAVEFORM *poWaveform) {

	//int i;
	//int byte;

	/*if(g_bit < 48) {
		g_usPeriods[g_bit++] = usPeriod;
		if(g_bit == 48) {
			g_bit = 0;
			for(byte = 0; byte < 5; byte++) {
				for(i = 0; i < 8; i++) {
					if(g_usPeriods[(byte * 8) + i] >= 6920) {
						g_irDataDescriptor[byte].unData.u8 |= (0x01 << i);
					} else {
						g_irDataDescriptor[byte].unData.u8 &= ~(0x01 << i);
					}
				}
			}

			g_bIsDataReady = 1;

			return -1;
		}
	}*/
	return 0;
}

static int IR_PanasonicCanHandle(IR_WAVEFORM *poWaveform) {
	uint16_t usPeriodUs, usDutyCycleUs;

	usPeriodUs 		= poWaveform->arrWavePoints[0].usPeriodUs;
	usDutyCycleUs 	= poWaveform->arrWavePoints[0].usDutyCycleUs;

	if((usPeriodUs >= 5100 && usPeriodUs <= 5300) && usDutyCycleUs >= 3400 && usDutyCycleUs <= 3600) {
		return 1;
	}

	return 0;
}

static const IR_PROTOCOL g_irPanasonicProtocol = {
	.pfnProtocolHandler = IR_PanasonicRx,
	.pfnCanHandle		= IR_PanasonicCanHandle,
	.pfnGetData			= IR_PANASONIC_GetData,
	.pfnGetDataObject	= NULL,
	.pfnEncode			= NULL,
	.ulCarrierFrequency	= 38000,							///< Just a guess that the carrier is 38Khz.
	.szpName			= "Panasonic (Modern)",
	.szpVersionString 	= "v1.0.0 (C)2011 James Walmsley",
};

const IR_PROTOCOL *IR_PANASONIC_GetHandlerInfo(void) {
	return &g_irPanasonicProtocol;
}
