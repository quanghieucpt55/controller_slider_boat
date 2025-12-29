/*
 * cytypes.h
 *
 *  Created on: Feb 24, 2025
 *      Author: trank
 */

#ifndef INC_CYTYPES_H_
#define INC_CYTYPES_H_

#include "stdint.h"

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

#define int8 int8_t
#define int16 int16_t
#define int32 int32_t

/*******************************************************************************
* Defines the standard return values used in PSoC content. A function is
* not limited to these return values but can use them when returning standard
* error values. Return values can be overloaded if documented in the function
* header. On the 8051 a function can use a larger return type but still use the
* defined return codes.
*
* Zero is successful, all other values indicate some form of failure. 1 - 0x7F -
* standard defined values; 0x80 - ...  - user or content defined values.
*******************************************************************************/
#define CYRET_SUCCESS           (0x00u)           /* Successful */
#define CYRET_BAD_PARAM         (0x01u)           /* One or more invalid parameters */
#define CYRET_INVALID_OBJECT    (0x02u)           /* Invalid object specified */
#define CYRET_MEMORY            (0x03u)           /* Memory related failure */
#define CYRET_LOCKED            (0x04u)           /* Resource lock failure */
#define CYRET_EMPTY             (0x05u)           /* No more objects available */
#define CYRET_BAD_DATA          (0x06u)           /* Bad data received (CRC or other error check) */
#define CYRET_STARTED           (0x07u)           /* Operation started, but not necessarily completed yet */
#define CYRET_FINISHED          (0x08u)           /* Operation completed */
#define CYRET_CANCELED          (0x09u)           /* Operation canceled */
#define CYRET_TIMEOUT           (0x10u)           /* Operation timed out */
#define CYRET_INVALID_STATE     (0x11u)           /* Operation not setup or is in an improper state */
#define CYRET_UNKNOWN           ((cystatus) 0xFFFFFFFFu)    /* Unknown failure */

/*******************************************************************************
*  Data manipulation defines
*******************************************************************************/

/* Get 8 bits of 16 bit value. */
#define LO8(x)                  ((uint8_t) ((x) & 0xFFu))
#define HI8(x)                  ((uint8_t) ((uint16_t)(x) >> 8))

/* Get 16 bits of 32 bit value. */
#define LO16(x)                 ((uint16_t) ((x) & 0xFFFFu))
#define HI16(x)                 ((uint16_t) ((uint32)(x) >> 16))

/* Swap the byte ordering of 32 bit value */
#define CYSWAP_ENDIAN32(x)  \
        ((uint32)((((x) >> 24) & 0x000000FFu) | (((x) & 0x00FF0000u) >> 8) | (((x) & 0x0000FF00u) << 8) | ((x) << 24)))

/* Swap the byte ordering of 16 bit value */
#define CYSWAP_ENDIAN16(x)      ((uint16)(((x) << 8) | (((x) >> 8) & 0x00FFu)))


#define CY_NORETURN         __attribute__ ((noreturn))
#define CY_SECTION(name)    __attribute__ ((section(name)))
#define CY_ALIGN(align)     __attribute__ ((aligned(align)))
#define CY_PACKED
#define CY_PACKED_ATTR      __attribute__ ((packed))
#define CY_INLINE           inline

#define CY_ISR(FuncName)        void FuncName (void)
#define CY_ISR_PROTO(FuncName)  void FuncName (void)

typedef void (* cyisraddress)(void);

#define NULL ((void *)0)

typedef unsigned long cystatus;

#endif /* INC_CYTYPES_H_ */
