/**
 *  File	:	pru_rpmsg.h
 *
 *  Summary	:	An RPMsg interface for the PRU to use while communicating with
 *  			the ARM host.
 *
 *  Notes	:
 *  - This file creates a structure (pru_rpmsg_transport) that contains
 *    pointers to two pru_virtqueue structures. This structure is used as the
 *    underlying transport layer of all RPMsg communication. Only one
 *    pru_rpmsg_transport structure is needed because multiple logical channels
 *    can use the same underlying transport.
 *  - This pru_rpmsg interface is meant to sit on top of the pru_virtqueue
 *    interface and abstract the communication even further away from the
 *    underlying data structures. The goal is to make the communication as
 *    simple as possible at the user application level.
 *  - The logic for the PRU side is summarized below:
 *
 *    PRU Slave:
 *    - To receive buffer from the ARM host:
 *          pru_rpmsg_receive(*transport, *src, *dst, *data, *len);
 *    - To send buffer to the host:
 *          pru_rpmsg_send(*transport, src, dst, *data, len);
 */

#ifndef _PRU_RPMSG_H_
#define _PRU_RPMSG_H_

#include <pru_virtqueue.h>

/* Return value indicating no kick was sent */
#define PRU_RPMSG_NO_KICK				1
/* Return value indicating success */
#define PRU_RPMSG_SUCCESS				0
/* Return value indicating there were no available buffers */
#define PRU_RPMSG_NO_BUF_AVAILABLE		-1
/* Return value indicating that the buffer from the virtqueue was too small */
#define PRU_RPMSG_BUF_TOO_SMALL			-2
/* Return value indicating that an invalid head index was given */
#define PRU_RPMSG_INVALID_HEAD			-3

/* The maximum size of the channel name and description */
#define RPMSG_NAME_SIZE 32
/* The maximum size of the buffer (including the header) */
#define RPMSG_BUF_SIZE	512

enum pru_rpmsg_ns_flags {
    RPMSG_NS_CREATE = 0,
    RPMSG_NS_DESTROY = 1
};

/**
 * Summary		: 	pru_rpmsg_transport is a structure that groups together the
 * 					two pru_virtqueues that are needed for two-way communication
 * 					with the ARM. This structure provides a logical wrapper for
 * 					the transport layer of the application. NOTE: Multiple
 * 					(logical) channels can be implemented on top of the same
 * 					transport layer.
 *
 * Variables	:	virtqueue0: contains the pru_virtqueue that is used for the
 * 								PRU->ARM communication
 * 					virtqueue1: contains the pru_virtqueue that is used for
 * 								the ARM->PRU communication
 */
struct pru_rpmsg_transport {
	struct pru_virtqueue 	virtqueue0;
	struct pru_virtqueue 	virtqueue1;
};

/**
* Summary		:	pru_rpmsg_receive receives a message, if available, from
* 					the ARM host.
*
* Parameters	:	transport: a pointer to the transport layer from which the
* 							   message should be received
* 					src: a pointer that is populated with the source address
* 						 where the message originated
* 					dst: a pointer that is populated with the destination
* 						 address where the message was sent (can help determine
* 						 for which channel the message is intended on the PRU)
* 					data: a pointer that is populated with a local data buffer
* 						  containing the message payload
* 					len: a pointer that is populated with the length of the
* 						 message payload
*
* Description	:	pru_rpmsg_receive uses the pru_virtqueue interface to get
* 					an available buffer, copy the buffer into local memory,
* 					add the buffer as a used buffer to the vring, and then kick
* 					the remote processor if necessary. The src, dst, data, and
* 					len pointers are populated with the information about the
* 					message and local buffer data if the reception is
* 					successful.
*
* Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
* 					buffer available for receive. Returns PRU_RPMSG_INVALID_HEAD
* 					if the head index returned for the available buffer is
* 					invalid. Returns PRU_RPMSG_SUCCESS if the message is
* 					successfully received.
*/
int16_t pru_rpmsg_receive (
	struct pru_rpmsg_transport 	*transport,
	uint16_t 					*src,
	uint16_t 					*dst,
	void 						*data,
	uint16_t 					*len
);

/**
* Summary		:	pru_rpmsg_send sends a message to the ARM host using the
* 					virtqueues in the pru_rpmsg_transport structure. The
* 					source and destination address of the message are passed
* 					in as parameters to the function. The data to be sent and
* 					its length are passed in the data and len parameters.
*
* Parameters	:	transport: a pointer to the transport layer from which the
* 							   message should be sent
* 					src: the source address where this message will originate
* 					dst: the destination address where the message will be sent
* 					data: a pointer to a local data buffer containing the
* 						  message payload
* 					len: the length of the message payload
*
* Description	:	pru_rpmsg_send sends a message to the src parameter and
* 					from the dst parameter. The transport structure defines the
* 					underlying transport mechanism that will be used. The
* 					data parameter is a pointer to a local buffer that should
* 					be sent to the destination address and the len parameter is
* 					the length of that buffer.
*
* Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
* 					buffer available for send. Returns PRU_RPMSG_BUF_TOO_SMALL
* 					if the buffer from the vring is too small to hold the
* 					message payload being sent. Returns PRU_RPMSG_INVALID_HEAD
* 					if the head index returned for the send buffer is invalid.
* 					Returns PRU_RPMSG_SUCCESS if the message is successfully
* 					sent.
*/
int16_t pru_rpmsg_send (
	struct pru_rpmsg_transport 	*transport,
	uint32_t 					src,
	uint32_t 					dst,
	void 						*data,
	uint16_t 					len
);

/**
* Summary		:	pru_rpmsg_channel uses an RPMsg Name Service Announcment
* 					to either create or destroy an RPMsg channel depending on
* 					the pru_rpmsg_ns_flags parameter that is specified.
*
* Parameters	:	flags: an enum that is used to create (RPMSG_NS_CREATE) or
* 						   destroy (RPMSG_NS_DESTROY) an RPMsg channel
* 					transport: a pointer to the transport layer on which this
* 							   Name Service Announcement will be sent
* 					name: the name of the channel being created or destroyed
* 						  ******* The name of the channel is very important as
* 						  ******* it is the method that Linux on the ARM uses
* 						  ******* to connect a PRU firmware with a corresponding
* 						  ******* Linux driver
* 					desc: the description of the RPMsg channel being created
* 						  or destroyed
* 					port: the local source address of the RPMsg channel. This
* 						  is the address where PRU messages destined for the
* 						  ARM host will originate
*
* Description	:	pru_rpmsg_channel sends a message letting the ARM
* 					host know that a channel is to be created or destroyed. If
* 					a channel is to be created then this message will notify
* 					the name server on the ARM host to create a new channel. If
* 					a channel is to be destroyed this will tear down this
* 					logical channel of communication between the PRU and the
* 					ARM host.
*
* Return Value	:	Returns PRU_RPMSG_NO_BUF_AVAILABLE if there is currently no
* 					buffer available for send. Returns PRU_RPMSG_BUF_TOO_SMALL
* 					if the buffer from the vring is too small to hold the
* 					message payload being sent. Returns PRU_RPMSG_INVALID_HEAD
* 					if the head index returned for the send buffer is invalid.
* 					Returns PRU_RPMSG_SUCCESS if the message is successfully
* 					sent.
*/
int16_t pru_rpmsg_channel (
	enum pru_rpmsg_ns_flags 	flags ,
	struct pru_rpmsg_transport 	*transport,
	char 						*name,
	char 						*desc,
	int32_t 					port
);

#endif /* _PRU_RPMSG_H_ */