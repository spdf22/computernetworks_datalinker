#include <stdio.h>
#include <string.h>

#include "protocol.h"
#include "datalink.h"

#define MAX_SEQ 7
#define NR_BUFS 4
#define DATA_TIMER 1200

struct FRAME {
	unsigned char kind;
	unsigned char ack;
	unsigned char seq;
	unsigned char data[PKT_LEN];
	unsigned int padding;
};

static unsigned char ack_expected = 0;
static unsigned char next_frame_to_send = 0;
static unsigned char frame_expected = 0;
static unsigned char nbuffered = 0;
static unsigned char out_buf[NR_BUFS][PKT_LEN];
static int phl_ready = 0;

static unsigned char inc(unsigned char seq)
{
	return (seq + 1) % (MAX_SEQ + 1);
}

static unsigned char prev(unsigned char seq)
{
	return (seq + MAX_SEQ) % (MAX_SEQ + 1);
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
	s.ack = prev(frame_expected);
	memcpy(s.data, out_buf[frame_nr % NR_BUFS], PKT_LEN);

	dbg_frame("Send DATA %d %d, ID %d\n", s.seq, s.ack, *(short *)s.data);

	put_frame((unsigned char *)&s, 3 + PKT_LEN);
	start_timer(frame_nr, DATA_TIMER);
}

static void send_ack_frame(void)
{
	struct FRAME s;

	s.kind = FRAME_ACK;
	s.ack = prev(frame_expected);

	dbg_frame("Send ACK  %d\n", s.ack);

	put_frame((unsigned char *)&s, 2);
}

static void resend_window(void)
{
	unsigned char seq = ack_expected;
	unsigned char i;

	for (i = 0; i < nbuffered; i++) {
		send_data_frame(seq);
		seq = inc(seq);
	}
}

int main(int argc, char **argv)
{
	int event, arg;
	struct FRAME f;
	int len = 0;

	protocol_init(argc, argv);
	lprintf("Go-Back-N basic, build: " __DATE__ "  " __TIME__ "\n");

	disable_network_layer();

	for (;;) {
		event = wait_for_event(&arg);

		switch (event) {
		case NETWORK_LAYER_READY:
			get_packet(out_buf[next_frame_to_send % NR_BUFS]);
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
				break;
			}

			if (f.kind == FRAME_ACK)
				dbg_frame("Recv ACK  %d\n", f.ack);

			if (f.kind == FRAME_DATA) {
				dbg_frame("Recv DATA %d %d, ID %d\n", f.seq, f.ack, *(short *)f.data);
				if (f.seq == frame_expected) {
					put_packet(f.data, len - 7);
					frame_expected = inc(frame_expected);
				}
				send_ack_frame();
			}

			while (between(ack_expected, f.ack, next_frame_to_send)) {
				nbuffered--;
				stop_timer(ack_expected);
				ack_expected = inc(ack_expected);
			}
			break;

		case DATA_TIMEOUT:
			dbg_event("---- DATA %d timeout\n", arg);
			resend_window();
			break;
		}

		if (nbuffered < NR_BUFS && phl_ready)
			enable_network_layer();
		else
			disable_network_layer();
	}
}
