#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

#define MAX_SEQ 15
#define NR_BUFS 8
#define DATA_TIMER 3198
#define ACK_TIMER 264

#define ACK_NONE 0
#define ACK_SELECTIVE 1
#define ACK_CUMULATIVE 2

struct FRAME {
	unsigned char kind;
	unsigned char ack;
	unsigned char seq;
	unsigned char ack_valid;
	unsigned char data[PKT_LEN];
	unsigned int padding;
};

static unsigned char ack_expected = 0;
static unsigned char next_frame_to_send = 0;
static unsigned char frame_expected = 0;
static unsigned char too_far = NR_BUFS;
static unsigned char nbuffered = 0;

static unsigned char out_buf[NR_BUFS][PKT_LEN];
static unsigned char in_buf[NR_BUFS][PKT_LEN];
static unsigned char out_seq[NR_BUFS];
static unsigned char in_seq[NR_BUFS];
static int arrived[NR_BUFS];
static int acked[NR_BUFS];

static int phl_ready = 0;
static int ack_pending = 0;
static int nak_sent = 0;
static unsigned char pending_ack = MAX_SEQ;

static unsigned char inc(unsigned char seq)
{
	return (seq + 1) % (MAX_SEQ + 1);
}

static int between(unsigned char a, unsigned char b, unsigned char c)
{
	return ((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a));
}

static void put_frame(unsigned char *frame, int len)
{
	*(unsigned int *)(frame + len) = crc32(frame, len);
	send_frame(frame, len + 4);
	phl_ready = 0;
}

static void send_data_frame(unsigned char frame_nr)
{
	struct FRAME s;

	s.kind = FRAME_DATA;
	s.seq = frame_nr;
	s.ack = (frame_expected + MAX_SEQ) % (MAX_SEQ + 1);
	s.ack_valid = ACK_CUMULATIVE;
	memcpy(s.data, out_buf[frame_nr % NR_BUFS], PKT_LEN);

	dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack_valid ? s.ack : 255, *(short *)s.data);

	if (ack_pending) {
		ack_pending = 0;
		stop_ack_timer();
	}
	put_frame((unsigned char *)&s, 4 + PKT_LEN);
	start_timer(frame_nr, DATA_TIMER);
}

static void send_ack_frame(unsigned char seq)
{
	struct FRAME s;

	s.kind = FRAME_ACK;
	s.ack = seq;
	s.ack_valid = ACK_SELECTIVE;

	dbg_frame("Send ACK  %d\n", s.ack);

	if (ack_pending && pending_ack == seq) {
		ack_pending = 0;
		stop_ack_timer();
	}
	put_frame((unsigned char *)&s, 3);
}

static void delay_ack(unsigned char seq)
{
	if (ack_pending && pending_ack != seq)
		send_ack_frame(pending_ack);
	pending_ack = seq;
	ack_pending = 1;
	start_ack_timer(ACK_TIMER);
}

static void send_nak_frame(unsigned char seq)
{
	struct FRAME s;

	s.kind = FRAME_NAK;
	s.ack = seq;
	s.ack_valid = ACK_SELECTIVE;

	dbg_frame("Send NAK  %d\n", s.ack);

	nak_sent = 1;
	put_frame((unsigned char *)&s, 3);
}

static void mark_acked(unsigned char seq)
{
	if (between(ack_expected, seq, next_frame_to_send) &&
	    out_seq[seq % NR_BUFS] == seq && !acked[seq % NR_BUFS]) {
		acked[seq % NR_BUFS] = 1;
		nbuffered--;
		stop_timer(seq);
	}
}

static void handle_ack(unsigned char seq, unsigned char ack_type)
{
	unsigned char s;

	if (ack_type == ACK_CUMULATIVE && between(ack_expected, seq, next_frame_to_send)) {
		s = ack_expected;
		while (between(ack_expected, s, inc(seq))) {
			mark_acked(s);
			s = inc(s);
		}
	} else if (ack_type == ACK_SELECTIVE) {
		mark_acked(seq);
	}

	while (ack_expected != next_frame_to_send &&
	       out_seq[ack_expected % NR_BUFS] == ack_expected &&
	       acked[ack_expected % NR_BUFS]) {
		acked[ack_expected % NR_BUFS] = 0;
		ack_expected = inc(ack_expected);
	}
}

static void resend_one(unsigned char seq)
{
	if (between(ack_expected, seq, next_frame_to_send) &&
	    out_seq[seq % NR_BUFS] == seq && !acked[seq % NR_BUFS])
		send_data_frame(seq);
}

static void accept_data_frame(struct FRAME *f, int len)
{
	if (between(frame_expected, f->seq, too_far) && !arrived[f->seq % NR_BUFS]) {
		arrived[f->seq % NR_BUFS] = 1;
		in_seq[f->seq % NR_BUFS] = f->seq;
		memcpy(in_buf[f->seq % NR_BUFS], f->data, len - 8);
		delay_ack(f->seq);
	} else {
		send_ack_frame(f->seq);
	}

	if (f->seq != frame_expected && !nak_sent)
		send_nak_frame(frame_expected);

	while (arrived[frame_expected % NR_BUFS] && in_seq[frame_expected % NR_BUFS] == frame_expected) {
		put_packet(in_buf[frame_expected % NR_BUFS], PKT_LEN);
		arrived[frame_expected % NR_BUFS] = 0;
		frame_expected = inc(frame_expected);
		too_far = inc(too_far);
		nak_sent = 0;
	}
}

int main(int argc, char **argv)
{
	int event, arg;
	struct FRAME f;
	int len = 0;

	protocol_init(argc, argv);
	lprintf("Selective Repeat with piggybacked ACK, ACK timer and NAK, build: " __DATE__ "  " __TIME__ "\n");

	disable_network_layer();

	for (;;) {
		event = wait_for_event(&arg);

		switch (event) {
		case NETWORK_LAYER_READY:
			get_packet(out_buf[next_frame_to_send % NR_BUFS]);
			out_seq[next_frame_to_send % NR_BUFS] = next_frame_to_send;
			acked[next_frame_to_send % NR_BUFS] = 0;
			nbuffered++;
			send_data_frame(next_frame_to_send);
			next_frame_to_send = inc(next_frame_to_send);
			break;

		case PHYSICAL_LAYER_READY:
			phl_ready = 1;
			break;

		case FRAME_RECEIVED:
			len = recv_frame((unsigned char *)&f, sizeof f);
			if (len < 5 || crc32((unsigned char *)&f, len) != 0) {
				dbg_event("**** Receiver Error, Bad CRC Checksum\n");
				if (!nak_sent)
					send_nak_frame(frame_expected);
				break;
			}

			if (f.kind == FRAME_ACK) {
				dbg_frame("Recv ACK  %d\n", f.ack);
				handle_ack(f.ack, f.ack_valid);
			}

			if (f.kind == FRAME_NAK) {
				dbg_frame("Recv NAK  %d\n", f.ack);
				resend_one(f.ack);
			}

			if (f.kind == FRAME_DATA) {
				dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack_valid ? f.ack : 255, *(short *)f.data);
				if (f.ack_valid)
					handle_ack(f.ack, f.ack_valid);
				accept_data_frame(&f, len);
			}
			break;

		case DATA_TIMEOUT:
			dbg_event("---- DATA %d timeout\n", arg);
			resend_one((unsigned char)arg);
			break;

		case ACK_TIMEOUT:
			if (ack_pending) {
				dbg_event("---- ACK timeout\n");
				send_ack_frame(pending_ack);
			}
			break;
		}

		if (nbuffered < NR_BUFS && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}
