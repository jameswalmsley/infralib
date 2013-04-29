/*
 * irDecoder.h
 *
 *  Created on: 18 Mar 2011
 *      Author: James
 */

#ifndef IRMANAGER_H_
#define IRMANAGER_H_

#include "irProtocols/protocols.h"
#include "stm32f10x.h"

#define IR_MAX_PROTOCOL_HANDLERS		10						///< Maximum number of protocols that can be registered.
#define IR_MAX_RX_WAVEFORM_BUFFERS		3						///< Maximum number of waveforms to buffer.
#define IR_MAX_TX_WAVEFORM_BUFFERS		2
#define IR_MAX_SIGNAL_LENGTH			128						///< Maximum number of wave points to record.

typedef struct {
	uint16_t 	usPeriodUs;										///< Period length of the wave in Microseconds.
	uint16_t 	usDutyCycleUs;									///< Dutycycle length of the wave in Microseconds.
} IR_WAVE;

typedef struct {
	IR_WAVE		arrWavePoints[IR_MAX_SIGNAL_LENGTH];			///< Waveform Data.
	uint16_t 	usWavePoints;									///< Number of items in the waveform data.
} IR_WAVEFORM;

typedef enum {
	IR_T_CHAR,
	IR_T_SHORT,
	IR_T_LONG,
} IR_DATA_TYPE;

typedef union {
	uint8_t 	u8;
	uint16_t	u16;
	uint32_t	u32;
} IR_UN_DATA;

typedef struct {
	const char 			*szpName;
	const IR_DATA_TYPE	 eType;
	IR_UN_DATA			 unData;
} IR_DATA_ITEM;

typedef struct {
	uint32_t			 nItems;								///< Number of items available.
	const IR_DATA_ITEM	*poItems;								///< Pointer to an array of items.
	const char			*szpProtocolName;						///< Zero terminated string of the protocol name.
	IR_PROTOCOL_IDENT	 eIdent;								///< Protocol's unique identifier.
} IR_DATA;

typedef int 	(*IR_PROTOCOL_RX) 				(IR_WAVEFORM *poWaveform);
typedef int 	(*IR_PROTOCOL_GET_DATA) 		(IR_DATA *poData);
typedef int		(*IR_PROTOCOL_GET_DATA_OBJECT)	(IR_DATA *poData);
typedef int		(*IR_PROTOCOL_ENCODE)			(IR_DATA *poData, IR_WAVEFORM *poWaveform);

typedef struct {
	const IR_PROTOCOL_RX				 pfnProtocolHandler;	///< Pointer to the protocol handler for this protocol.
	const IR_PROTOCOL_RX				 pfnCanHandle;			///< Determine if handler can process this protocol based on waveform.
	const IR_PROTOCOL_GET_DATA			 pfnGetData;			///< Protocol get latest data item.
	const IR_PROTOCOL_GET_DATA_OBJECT	 pfnGetDataObject;		///< Protocol function to get a protocol data object for filling.
	const IR_PROTOCOL_ENCODE			 pfnEncode;				///< Protocol function to encode data object, into a waveform.
	const uint32_t						 ulCarrierFrequency;	///< Frequency of the Carrier wave in Hz.
	const IR_PROTOCOL_IDENT				 eIdent;				///< Unique protocol identifier.
	const char 							*szpName;				///< Name of the protocol.
	const char							*szpVersionString;		///< Version info of this handler.
} IR_PROTOCOL;

typedef int		(*IR_DRIVER_INIT)				(void *pParam);
typedef int 	(*IR_DRIVER_CLOSE)				(void *pParam);
typedef int		(*IR_DRIVER_TRANSMIT)			(IR_WAVEFORM *poWaveform, uint32_t ulCarrierFrequency, void *pParam);

typedef struct {
	const IR_DRIVER_INIT				pfnInit;
	const IR_DRIVER_CLOSE				pfnClose;
	const IR_DRIVER_TRANSMIT			pfnTransmit;
	const char							*szpName;
	const char							*szpVersionString;
} IR_DRIVER;

int 				 IR_InitManager			(void);
int 				 IR_RegisterIRDriver	(const IR_DRIVER *poDriver, void *pParam);
int 				 IR_UnregisterIRDriver	(void);
int 				 IR_InitialiseDriver	(void);
int 				 IR_RegisterProtocol	(const IR_PROTOCOL *poProtocol);
int 				 IR_GetData				(IR_DATA *poData);
IR_DATA_ITEM 		*IR_GetDataItem			(IR_DATA *poData, const char *szpItemName);
IR_PROTOCOL_IDENT 	 IR_GetProtocolIdent	(const char *szpProtocolName);
int 				 IR_GetDataObject		(IR_PROTOCOL_IDENT eIdent, IR_DATA *poData);
int 		 		 IR_Event				(uint16_t usPeriodUs, uint16_t usDutyCycleUs, uint8_t bTimedOut);
int					 IR_Transmit			(IR_DATA *poData);

#endif /* IRDECODER_H_ */
