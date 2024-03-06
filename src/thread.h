/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020-2023 Linutronix GmbH
 * Author Kurt Kanzenbach <kurt@linutronix.de>
 */

#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <linux/if_ether.h>

struct ring_buffer;
struct xdp_socket;
struct security_context;

struct thread_context {
	/* Task related */
	pthread_t rx_task_id;         /* Receiver Thread */
	uint64_t rx_sequence_counter; /* Rx cycle counter */
	pthread_t tx_task_id;         /* Sender Thread */
	pthread_t tx_gen_task_id;     /* Sender generation thread */
	volatile int stop;            /* Done? */

	/* RAW socket related */
	int socket_fd;                       /* Shared RAW socket */
	unsigned char *tx_frame_data;        /* Tx frame data */
	unsigned char *rx_frame_data;        /* Rx frame data */
	unsigned char source[ETH_ALEN];      /* Source MAC Address */
	struct sockaddr_storage destination; /* Where to send L3 frames to */
	struct ring_buffer *mirror_buffer;   /* Rx frames to be mirrored */

	/* XDP socket related */
	struct xdp_socket *xsk;         /* XDP socket reference */
	unsigned int received_frames;   /* Amount of frames received within cycle */
	pthread_mutex_t xdp_data_mutex; /* Protect concurrent access to Xsk */

	/* Data flow related */
	struct thread_context *next;  /* Pointer to next traffic class */
	pthread_mutex_t data_mutex;   /* Mutex to protect frame data */
	pthread_cond_t data_cond_var; /* Cond var to signal Tx thread */
	size_t num_frames_available;  /* How many frames are ready to be sent? */
	bool is_first;                /* Is this the first active traffic class? */

	/* Security related */
	struct security_context *tx_security_context; /* Tx context for Auth and Crypt */
	struct security_context *rx_security_context; /* Rx context for Auth and Crypt */

	/* Thread private data */
	void *private_data; /* Pointer to private data e.g, a structure */
};

enum pn_thread_type {
	TSN_HIGH_THREAD = 0,
	TSN_LOW_THREAD,
	RTC_THREAD,
	RTA_THREAD,
	DCP_THREAD,
	LLDP_THREAD,
	UDP_HIGH_THREAD,
	UDP_LOW_THREAD,
	NUM_PN_THREAD_TYPES,
};

int create_rt_thread(pthread_t *task_id, const char *thread_name, int priority, int cpu_core,
		     void *(*thread_routine)(void *), void *data);
void init_mutex(pthread_mutex_t *mutex);
void init_condition_variable(pthread_cond_t *cond_var);
int link_pn_threads(struct thread_context *pn_threads);

#endif /* _THREAD_H_ */
