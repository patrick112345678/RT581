/**
 * @file status.h
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#ifndef CPC_STATUS_H
#define CPC_STATUS_H 

#define CPC_STATUS_OK    ((status_t)0x0000)  ///< No error.
#define CPC_STATUS_FAIL  ((status_t)0x0001)  ///< Generic error.

// State Errors
#define CPC_STATUS_INVALID_STATE         ((status_t)0x0002)  ///< Generic invalid state error.
#define CPC_STATUS_NOT_READY             ((status_t)0x0003)  ///< Module is not ready for requested operation.
#define CPC_STATUS_BUSY                  ((status_t)0x0004)  ///< Module is busy and cannot carry out requested operation.
#define CPC_STATUS_IN_PROGRESS           ((status_t)0x0005)  ///< Operation is in progress and not yet complete (pass or fail).
#define CPC_STATUS_ABORT                 ((status_t)0x0006)  ///< Operation aborted.
#define CPC_STATUS_TIMEOUT               ((status_t)0x0007)  ///< Operation timed out.
#define CPC_STATUS_PERMISSION            ((status_t)0x0008)  ///< Operation not allowed per permissions.
#define CPC_STATUS_WOULD_BLOCK           ((status_t)0x0009)  ///< Non-blocking operation would block.
#define CPC_STATUS_IDLE                  ((status_t)0x000A)  ///< Operation/module is Idle, cannot carry requested operation.
#define CPC_STATUS_IS_WAITING            ((status_t)0x000B)  ///< Operation cannot be done while construct is waiting.
#define CPC_STATUS_NONE_WAITING          ((status_t)0x000C)  ///< No task/construct waiting/pending for that action/event.
#define CPC_STATUS_SUSPENDED             ((status_t)0x000D)  ///< Operation cannot be done while construct is suspended.
#define CPC_STATUS_NOT_AVAILABLE         ((status_t)0x000E)  ///< Feature not available due to software configuration.
#define CPC_STATUS_NOT_SUPPORTED         ((status_t)0x000F)  ///< Feature not supported.
#define CPC_STATUS_INITIALIZATION        ((status_t)0x0010)  ///< Initialization failed.
#define CPC_STATUS_NOT_INITIALIZED       ((status_t)0x0011)  ///< Module has not been initialized.
#define CPC_STATUS_ALREADY_INITIALIZED   ((status_t)0x0012)  ///< Module has already been initialized.
#define CPC_STATUS_DELETED               ((status_t)0x0013)  ///< Object/construct has been deleted.
#define CPC_STATUS_ISR                   ((status_t)0x0014)  ///< Illegal call from ISR.
#define CPC_STATUS_NETWORK_UP            ((status_t)0x0015)  ///< Illegal call because network is up.
#define CPC_STATUS_NETWORK_DOWN          ((status_t)0x0016)  ///< Illegal call because network is down.
#define CPC_STATUS_NOT_JOINED            ((status_t)0x0017)  ///< Failure due to not being joined in a network.
#define CPC_STATUS_NO_BEACONS            ((status_t)0x0018)  ///< Invalid operation as there are no beacons.

// Allocation/ownership Errors
#define CPC_STATUS_ALLOCATION_FAILED   ((status_t)0x0019)  ///< Generic allocation error.
#define CPC_STATUS_NO_MORE_RESOURCE    ((status_t)0x001A)  ///< No more resource available to perform the operation.
#define CPC_STATUS_EMPTY               ((status_t)0x001B)  ///< Item/list/queue is empty.
#define CPC_STATUS_FULL                ((status_t)0x001C)  ///< Item/list/queue is full.
#define CPC_STATUS_WOULD_OVERFLOW      ((status_t)0x001D)  ///< Item would overflow.
#define CPC_STATUS_HAS_OVERFLOWED      ((status_t)0x001E)  ///< Item/list/queue has been overflowed.
#define CPC_STATUS_OWNERSHIP           ((status_t)0x001F)  ///< Generic ownership error.
#define CPC_STATUS_IS_OWNER            ((status_t)0x0020)  ///< Already/still owning resource.

// Invalid Parameters Errors
#define CPC_STATUS_INVALID_PARAMETER       ((status_t)0x0021)  ///< Generic invalid argument or consequence of invalid argument.
#define CPC_STATUS_NULL_POINTER            ((status_t)0x0022)  ///< Invalid null pointer received as argument.
#define CPC_STATUS_INVALID_CONFIGURATION   ((status_t)0x0023)  ///< Invalid configuration provided.
#define CPC_STATUS_INVALID_MODE            ((status_t)0x0024)  ///< Invalid mode.
#define CPC_STATUS_INVALID_HANDLE          ((status_t)0x0025)  ///< Invalid handle.
#define CPC_STATUS_INVALID_TYPE            ((status_t)0x0026)  ///< Invalid type for operation.
#define CPC_STATUS_INVALID_INDEX           ((status_t)0x0027)  ///< Invalid index.
#define CPC_STATUS_INVALID_RANGE           ((status_t)0x0028)  ///< Invalid range.
#define CPC_STATUS_INVALID_KEY             ((status_t)0x0029)  ///< Invalid key.
#define CPC_STATUS_INVALID_CREDENTIALS     ((status_t)0x002A)  ///< Invalid credentials.
#define CPC_STATUS_INVALID_COUNT           ((status_t)0x002B)  ///< Invalid count.
#define CPC_STATUS_INVALID_SIGNATURE       ((status_t)0x002C)  ///< Invalid signature / verification failed.
#define CPC_STATUS_NOT_FOUND               ((status_t)0x002D)  ///< Item could not be found.
#define CPC_STATUS_ALREADY_EXISTS          ((status_t)0x002E)  ///< Item already exists.

// IO/Communication Errors
#define CPC_STATUS_IO                    ((status_t)0x002F)  ///< Generic I/O failure.
#define CPC_STATUS_IO_TIMEOUT            ((status_t)0x0030)  ///< I/O failure due to timeout.
#define CPC_STATUS_TRANSMIT              ((status_t)0x0031)  ///< Generic transmission error.
#define CPC_STATUS_TRANSMIT_UNDERFLOW    ((status_t)0x0032)  ///< Transmit underflowed.
#define CPC_STATUS_TRANSMIT_INCOMPLETE   ((status_t)0x0033)  ///< Transmit is incomplete.
#define CPC_STATUS_TRANSMIT_BUSY         ((status_t)0x0034)  ///< Transmit is busy.
#define CPC_STATUS_RECEIVE               ((status_t)0x0035)  ///< Generic reception error.
#define CPC_STATUS_OBJECT_READ           ((status_t)0x0036)  ///< Failed to read on/via given object.
#define CPC_STATUS_OBJECT_WRITE          ((status_t)0x0037)  ///< Failed to write on/via given object.
#define CPC_STATUS_MESSAGE_TOO_LONG      ((status_t)0x0038)  ///< Message is too long.

#endif