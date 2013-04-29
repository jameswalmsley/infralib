/*
 * irManager.c
 *
 *  Created on: 18 Mar 2011
 *      Author: James
 */

#include "irManager.h"
#include <stdlib.h>
#include <string.h>

static IR_PROTOCOL 		g_arrProtocols[IR_MAX_PROTOCOL_HANDLERS];
static uint16_t			g_usRegisteredProtocols = 0;

static IR_WAVEFORM		g_arrWaveforms[IR_MAX_RX_WAVEFORM_BUFFERS];
static uint8_t			g_ucCurrentWaveform = 0;
static uint8_t			g_ucWaveformValid[IR_MAX_RX_WAVEFORM_BUFFERS];

static IR_WAVEFORM		g_arrTxWaveformBuffers[IR_MAX_TX_WAVEFORM_BUFFERS];
static uint8_t			g_ucCurrentTxWaveForm = 0;

static const IR_DRIVER	*g_poIrDriver 		= NULL;	///< Pointer to an IR Driver.
static void				*g_poIrDriverParam	= NULL;	///< IR driver parameter.

typedef enum {
	IR_STATE_IDLE,
	IR_STATE_FIRST_CAPTURE,
	IR_STATE_IN_PROTOCOL,
	IR_STATE_TIMEDOUT,
} IR_STATE;

static IR_STATE 	g_eState = IR_STATE_IDLE;

static void IR_ProcessData() {
	int i,y;

	for(i = 0; i < IR_MAX_RX_WAVEFORM_BUFFERS; i++) {
		if(g_ucWaveformValid[i]) {
			goto validData;
		}
	}

	if(0) {		// Using Clifford's device! -- Helping us to separate the code for better readability.
validData:
		for(y = 0; y < g_usRegisteredProtocols; y++) {
			if(g_arrProtocols[y].pfnCanHandle(&g_arrWaveforms[i])) {
				g_arrProtocols[y].pfnProtocolHandler(&g_arrWaveforms[i]);
				break;
			}
		}

		g_ucWaveformValid[i] = 0;	// Either it was handled, or we'll discard the data.
	}
}

int IR_GetData(IR_DATA *poData) {
	int i, RetVal;

	if(!poData) {
		return -2;
	}

	IR_ProcessData();	// Tick over the waveform processor.

	for(i = 0; i < g_usRegisteredProtocols; i++) {
		if(g_arrProtocols[i].pfnGetData) {
			RetVal = g_arrProtocols[i].pfnGetData(poData);
			if(!RetVal) {
				return 0;
			}
		}
	}

	return -1;
}

int IR_RegisterProtocol(const IR_PROTOCOL *poProtocol) {
	if(poProtocol) {
		if(g_usRegisteredProtocols >= IR_MAX_PROTOCOL_HANDLERS) {
			return -2;	// Cannot add more handlers to the table.
		}
		memcpy(&g_arrProtocols[g_usRegisteredProtocols++], poProtocol, sizeof(IR_PROTOCOL));
		return 0;
	}
	return -1;
}



int IR_Event(uint16_t usPeriodUs, uint16_t usDutyCycleUs, uint8_t bTimedOut) {
	uint16_t finishedWaveform;

	TIM7->CNT = 0;

	if(bTimedOut) {
		g_eState = IR_STATE_TIMEDOUT;
	}

	switch(g_eState) {
		case IR_STATE_IDLE: {												// Must be the lead in signal, determine protocol (if possible).
			g_eState = IR_STATE_FIRST_CAPTURE;								// Skip the first signal, its a reset.
			g_arrWaveforms[g_ucCurrentWaveform].usWavePoints = 0;
			TIM_Cmd(TIM7, ENABLE);
			break;
		}

		case IR_STATE_FIRST_CAPTURE: {

			g_arrWaveforms[g_ucCurrentWaveform].arrWavePoints[g_arrWaveforms[g_ucCurrentWaveform].usWavePoints].usPeriodUs = usPeriodUs;
			g_arrWaveforms[g_ucCurrentWaveform].arrWavePoints[g_arrWaveforms[g_ucCurrentWaveform].usWavePoints++].usDutyCycleUs = usDutyCycleUs;

			break;
		}

		case IR_STATE_IN_PROTOCOL: {
			g_arrWaveforms[g_ucCurrentWaveform].arrWavePoints[g_arrWaveforms[g_ucCurrentWaveform].usWavePoints].usPeriodUs = usPeriodUs;
			g_arrWaveforms[g_ucCurrentWaveform].arrWavePoints[g_arrWaveforms[g_ucCurrentWaveform].usWavePoints++].usDutyCycleUs = usDutyCycleUs;

			break;
		}

		case IR_STATE_TIMEDOUT: {
			g_arrWaveforms[g_ucCurrentWaveform].arrWavePoints[g_arrWaveforms[g_ucCurrentWaveform].usWavePoints].usDutyCycleUs = usDutyCycleUs;	// Add final point.
			g_arrWaveforms[g_ucCurrentWaveform].arrWavePoints[g_arrWaveforms[g_ucCurrentWaveform].usWavePoints++].usPeriodUs = 0xFFFF; // Indefinite period.

			finishedWaveform = g_ucCurrentWaveform;
			g_ucWaveformValid[finishedWaveform] = 1;	// There is valid data available.
			g_ucCurrentWaveform++;

			if(g_ucCurrentWaveform >= IR_MAX_RX_WAVEFORM_BUFFERS) {
				g_ucCurrentWaveform = 0;
			}

			g_eState = IR_STATE_IDLE;
			break;
		}

		default:
			g_eState = IR_STATE_IDLE;
			break;
	}

	return 0;
}

IR_DATA_ITEM *IR_GetDataItem(IR_DATA *poData, const char *szpItemName) {
	int i;
	for(i = 0; i < poData->nItems; i++) {
		if(!strcmp(poData->poItems[i].szpName, szpItemName)) {
			return (IR_DATA_ITEM *) &poData->poItems[i];
		}
	}
	return NULL;
}

int IR_RegisterIRDriver(const IR_DRIVER *poDriver, void *pParam) {
	if(!g_poIrDriver) {
		g_poIrDriver = poDriver;
		g_poIrDriverParam = pParam;
		return 0;
	}
	return -1;
}

int IR_UnregisterIRDriver(void) {
	if(!g_poIrDriver) {
		return -1;
	}
	if(g_poIrDriver->pfnClose) {
		g_poIrDriver->pfnClose(g_poIrDriverParam);
	}

	g_poIrDriverParam = NULL;
	g_poIrDriver = NULL;

	return 0;
}

int IR_InitManager(void) {
	int i;
	for(i = 0; i < IR_MAX_PROTOCOL_HANDLERS; i++) {
		memset((void *)&g_arrProtocols[i], 0, sizeof(IR_PROTOCOL));
	}

	for(i = 0; i < IR_MAX_RX_WAVEFORM_BUFFERS; i++) {
		memset((void *)&g_ucWaveformValid, 0, sizeof(g_ucWaveformValid));
	}

	return 0;
}

int IR_InitialiseDriver(void) {
	if(g_poIrDriver) {
		if(g_poIrDriver->pfnInit) {
			return g_poIrDriver->pfnInit(g_poIrDriverParam);
		}
	}
	return -1;
}

IR_PROTOCOL_IDENT IR_GetProtocolIdent(const char *szpProtocolName) {
	int i;
	for(i = 0; i < g_usRegisteredProtocols; i++) {
		if(!strcmp(g_arrProtocols[i].szpName, szpProtocolName)) {
			return g_arrProtocols[i].eIdent;
		}
	}

	return IR_PROTOCOL_NOTFOUND;
}

int IR_GetDataObject(IR_PROTOCOL_IDENT eIdent, IR_DATA *poData) {
	int i;
	for(i = 0; i < g_usRegisteredProtocols; i++) {
		if(g_arrProtocols[i].eIdent == eIdent) {
			g_arrProtocols[i].pfnGetDataObject(poData);
			return 0;
		}
	}
	return -1;
}

int IR_Transmit(IR_DATA *poData) {
	int i;
	uint8_t ucTxBuffer;
	IR_PROTOCOL *pProtocol = NULL;
	// Find a protocol that can handle this data.
	for(i = 0; i < g_usRegisteredProtocols; i++) {
		if(g_arrProtocols[i].eIdent == poData->eIdent) {
			pProtocol = &g_arrProtocols[i];
			break;
		}
	}

	if(!pProtocol) {
		return -1;		// No matching protocol to encode this data.
	}

	if(!pProtocol->pfnEncode) {
		return -1;		// No encoder found for selected protocol.
	}

	// Request encode.
	pProtocol->pfnEncode(poData, &g_arrTxWaveformBuffers[g_ucCurrentTxWaveForm]);
	ucTxBuffer = g_ucCurrentTxWaveForm++;

	if(g_ucCurrentTxWaveForm >= IR_MAX_TX_WAVEFORM_BUFFERS) {
		g_ucCurrentTxWaveForm = 0;
	}

	// Get the driver to transmit the IR wave.
	if(!g_poIrDriver->pfnTransmit) {
		return -1;
	}

	return g_poIrDriver->pfnTransmit(&g_arrTxWaveformBuffers[ucTxBuffer], pProtocol->ulCarrierFrequency, g_poIrDriverParam);
}
