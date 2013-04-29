/*
 * RC6.h
 *
 *  Created on: 18 Mar 2011
 *      Author: James
 */

#ifndef RC6_H_
#define RC6_H_

#include "../irManager.h"

#define IR_RC6_LEADIN_PERIOD_US		3555
#define IR_RC6_LEADIN_DUTY_US		2666
#define IR_RC6_LEADIN_TOLERENCE_US	100
#define IR_RC6_BIT_LENGTH_US		444
#define IR_RC6_BIT_TOLERENCE_US		10

const IR_PROTOCOL *IR_RC6_GetHandlerInfo(void);

#endif /* RC6_H_ */
