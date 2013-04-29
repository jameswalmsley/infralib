/*
 * RC6.c
 *
 *  Created on: 18 Mar 2011
 *      Author: James
 */

#include "../irManager.h"
#include "../irTransmitter.h"

#include "RC6.h"
#include <stdlib.h>

static const IR_PROTOCOL g_irRC6Protocol;

static IR_DATA_ITEM g_irDataDescriptor[] = {
	{
		.szpName 	= "HEADER",
		.eType		= IR_T_CHAR,
	},
	{
		.szpName	= "PAYLOAD_0",	// Standard mode will only populate this field.
		.eType		= IR_T_LONG,	// Also RC6-A (MCE 32-bit protocol).
	},
	{
		.szpName	= "PAYLOAD_1",	// Other proprietary devices might fill these up.
		.eType		= IR_T_LONG,
	},
	{
		.szpName	= "PAYLOAD_2",
		.eType		= IR_T_LONG,
	},
	{
		.szpName	= "PAYLOAD_3",
		.eType		= IR_T_LONG,
	},
	{
		.szpName	= "PAYLOAD_4",
		.eType		= IR_T_LONG,
	},
};

uint8_t g_bDataObjectInUse = 0;					// Free the object on transmit encode complete.

static IR_DATA_ITEM g_irDataObject[] = {	// This object is for Transmission populating.
	{
		.szpName 	= "HEADER",
		.eType		= IR_T_CHAR,
	},
	{
		.szpName	= "PAYLOAD_0",	// Standard mode will only populate this field.
		.eType		= IR_T_LONG,	// Also RC6-A (MCE 32-bit protocol).
	},
	{
		.szpName	= "PAYLOAD_1",	// Other proprietary devices might fill these up.
		.eType		= IR_T_LONG,
	},
	{
		.szpName	= "PAYLOAD_2",
		.eType		= IR_T_LONG,
	},
	{
		.szpName	= "PAYLOAD_3",
		.eType		= IR_T_LONG,
	},
	{
		.szpName	= "PAYLOAD_4",
		.eType		= IR_T_LONG,
	},
};

static int		g_bIsDataReady = 0;
static int 		g_nItems = 0;

int IR_RC6_GetData(IR_DATA *poData) {
	if(g_bIsDataReady) {
		if(poData) {
			poData->nItems = g_nItems;
			poData->poItems = g_irDataDescriptor;
			poData->szpProtocolName = g_irRC6Protocol.szpName;
			poData->eIdent = IR_PROTOCOL_RC6;
		}

		g_bIsDataReady = 0;

		return 0;
	}
	return -1;
}

static int IR_RC6_GetNumBits(uint32_t ulUs) {
	int i = 0;

	while(ulUs >= IR_RC6_BIT_LENGTH_US) {
		ulUs -= IR_RC6_BIT_LENGTH_US;
		i++;
	}

	if(ulUs >= (IR_RC6_BIT_LENGTH_US - (i*IR_RC6_BIT_TOLERENCE_US))) {
		i++;
	}

	return i;
}

static int IR_RC6Rx(IR_WAVEFORM *poWaveform) {
	int i,y;
	IR_WAVE *pWp;
	int pulseBits, periodBits;
	int bit = 0;
	uint32_t manBits, nextManBits, nManBit, decodedBit;

	uint32_t data[8] = {
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	};

	for(i = 1; i < poWaveform->usWavePoints; i++) {
		pWp = &poWaveform->arrWavePoints[i];

		// Number of bits this wavepoint contains. (Bi-phase bits).
		periodBits = IR_RC6_GetNumBits(pWp->usPeriodUs);
		pulseBits = IR_RC6_GetNumBits(pWp->usDutyCycleUs);

		for(y = 0; y < pulseBits; y++) {
			data[(bit/32)] |= (0x80000000 >> (bit % 32));
			bit++;
		}

		if(periodBits > 6) {	// Final pulse, with indefinite period. Don't add more bits.
			break;
		}

		bit += (periodBits - pulseBits);
	}

	// Clear the data descriptors.
	for(i = 0; i < (sizeof(g_irDataDescriptor)/sizeof(IR_DATA_ITEM)); i++) {
		g_irDataDescriptor[i].unData.u32 = 0;
	}

	// Now run a Manchester decoder on the bitstream.
	for(i = 0, nManBit = 0; i < bit; i += 2) {
		manBits = data[i/32] >> ((32 - (i % 32)) - 2);// & (0xC000000 >> (i % 32));
		manBits &= 0x3;

		switch(manBits) {
			case 0x2: {		// A manchester encoded 1 was found.
					decodedBit = 1;
				break;
			}

			case 0x1: {		// A manchester encoded 0 was found.
					decodedBit = 0;
				break;
			}

			case 0x0: {		// Either start of a manchester encoded 0 (Trailer double length bit) or EOS.
				if(nManBit == 4) { // This should be a Trailer bit.
					nextManBits = data[(i+2)/32] >> ((32 - ((i+2) % 32)) - 2);// & (0xC000000 >> (i % 32));
					nextManBits &= 0x3;
					if(nextManBits == 3) {	// This is a real Trailer.
						decodedBit = 0;
						i += 2;	// Skip an extra 2 bits.
					}
				}
				break;
			}

			case 0x3: {		// A manchester encoded 1, for Trailer bits (double length).
				if(nManBit == 4) { // This should be a Trailer bit.
					nextManBits = data[(i+2)/32] >> ((32 - ((i+2) % 32)) - 2);// & (0xC000000 >> (i % 32));
					nextManBits &= 0x3;
					if(nextManBits == 0) {	// This is a real Trailer.
						decodedBit = 1;
						i += 2;	// Skip an extra 2 bits.
					}
				}
				break;
			}
		}

		if(nManBit < 5) {
			g_irDataDescriptor[0].unData.u8 |= (decodedBit << (4 - nManBit));	// Fill in the header field.
			nManBit++;
		} else {
			g_irDataDescriptor[((nManBit-5) / 32) + 1].unData.u32 |= (decodedBit << (((32 - (nManBit - 5))) - 1));
			nManBit++;
		}
	}

	g_nItems = ((nManBit-5) / 32) + 1;

	// Now we have our final RC-6 data :D -- Wasn't too hard :D

	g_bIsDataReady = 1;

	return 0;
}

static int IR_RC6CanHandle(IR_WAVEFORM *poWaveform) {
	uint16_t usPeriodUs, usDutyCycleUs;

	usPeriodUs = poWaveform->arrWavePoints[0].usPeriodUs;
	usDutyCycleUs = poWaveform->arrWavePoints[0].usDutyCycleUs;

	if((usPeriodUs >= (IR_RC6_LEADIN_PERIOD_US - IR_RC6_LEADIN_TOLERENCE_US)
			&& usPeriodUs <= (IR_RC6_LEADIN_PERIOD_US + IR_RC6_LEADIN_TOLERENCE_US))
			&& usDutyCycleUs >= (IR_RC6_LEADIN_DUTY_US - IR_RC6_LEADIN_TOLERENCE_US)
			&& usDutyCycleUs <= (IR_RC6_LEADIN_DUTY_US + IR_RC6_LEADIN_TOLERENCE_US)) {
		// Check the bit lengths here (for more Guarantee).
		return 1;
	}
	return 0;
}

static int IR_RC6_GetDataObject(IR_DATA *poData) {
	if(g_bDataObjectInUse) {
		return -1;
	}

	poData->nItems 			= 6;
	poData->poItems 		= g_irDataObject;
	poData->szpProtocolName = g_irRC6Protocol.szpName;
	poData->eIdent			= IR_PROTOCOL_RC6;

	return 0;
}

static int IR_RC6_Encode(IR_DATA *poData, IR_WAVEFORM *poWaveform) {

	IR_DATA_ITEM 	*poItem;
	uint16_t 		usBit;
	uint8_t			manBits, bit;
	uint32_t		usManBit, i,y, ulCurrentWaveform, ulPulse, ulDown, item;

	uint32_t data[8] = {
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
	};

	// Lead in.
	poWaveform->arrWavePoints[0].usPeriodUs 	= IR_RC6_LEADIN_PERIOD_US;
	poWaveform->arrWavePoints[0].usDutyCycleUs 	= IR_RC6_LEADIN_DUTY_US;

	// Manchester encode the header.
	poItem = IR_GetDataItem(poData, "HEADER");
	for(usBit = 0, usManBit = 0; usBit < 5; usBit++) {
		if(poItem->unData.u8 & (0x10 >> usBit)) {
			manBits = 0x2;
		} else {
			manBits = 0x1;
		}

		if(usBit == 4) {
			if(manBits == 0x2) {
				data[0] |= (0x3 << (32 - (usManBit *2) - 2));
				manBits = 0;
				usManBit++;
			}
			if(manBits == 0x1) {
				manBits = 3;
				usManBit++;
			}
		}

		data[0] |= (manBits << (32 - (usManBit *2) - 2));
		usManBit++;
	}
	// Encode the provided data.
	if(poData->nItems >= 2) {
		poItem = IR_GetDataItem(poData, "PAYLOAD_0");
		for(usBit = 0; usBit < 32; usBit++) {
			if(poItem->unData.u32 & (0x80000000 >> usBit)) {
				manBits = 0x2;
			} else {
				manBits = 0x1;
			}

			data[(usManBit*2) / 32] |= manBits << (32 - ((usManBit * 2) % 32) - 2);
			usManBit++;
		}
	}

	// Convert to a waveform.
	ulCurrentWaveform = 1;
	i = 0;
	do {
		// Count number of pulse lengths.
		y = 0;
		do {
			bit = data[(i+y)/32] >> ((32 - ((y+i)%32)) - 1);
			bit &= 1;
			if(bit) {
				y++;
			}
		}while(bit);

		ulPulse = y;
		i+= y;
		y = 0;

		do {
			bit = data[(i+y)/32] >> ((32 - ((y+i)%32)) - 1);
			bit &= 1;
			if(!bit) {
				y++;
			}
		}while(!bit && y < 4);

		ulDown = y;
		i += y;

		poWaveform->arrWavePoints[ulCurrentWaveform].usDutyCycleUs = (IR_RC6_BIT_LENGTH_US * ulPulse);
		poWaveform->arrWavePoints[ulCurrentWaveform].usPeriodUs = (IR_RC6_BIT_LENGTH_US * (ulPulse + ulDown));

		ulCurrentWaveform++;
	} while(y < 4);

	poWaveform->usWavePoints = ulCurrentWaveform;

	g_bDataObjectInUse = 0;	// Should check that this really was our own data object.

	return 0;
}

static const IR_PROTOCOL g_irRC6Protocol = {
	.pfnProtocolHandler = IR_RC6Rx,							///< Decode Waveform handler.
	.pfnCanHandle		= IR_RC6CanHandle,					///< Determine quickly if this module can process the waveform.
	.pfnGetData			= IR_RC6_GetData,					///< User-mode function to get data from a decoded waveform.
	.pfnGetDataObject	= IR_RC6_GetDataObject,				///< Get a data object for population of fields for transmission.
	.pfnEncode			= IR_RC6_Encode,					///< Encode a data object into a Waveform.
	.ulCarrierFrequency	= 36000,
	.eIdent				= IR_PROTOCOL_RC6,
	.szpName			= "RC6",
	.szpVersionString 	= "v1.0.0 (C)2011 James Walmsley",
};

const IR_PROTOCOL *IR_RC6_GetHandlerInfo(void) {
	return &g_irRC6Protocol;
}
