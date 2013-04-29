/*
 * protocols.h
 *
 *  Created on: 20 Apr 2011
 *      Author: James
 */

#ifndef PROTOCOLS_H_
#define PROTOCOLS_H_

typedef enum {
	IR_PROTOCOL_NOTFOUND = 0,	///< Special non-existent protocol ident.
	IR_PROTOCOL_GENERIC,
	IR_PROTOCOL_NEC,
	IR_PROTOCOL_PANASONIC,
	IR_PROTOCOL_RC6,
} IR_PROTOCOL_IDENT;


#endif /* PROTOCOLS_H_ */
