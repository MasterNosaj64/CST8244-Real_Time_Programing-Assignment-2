struct Metronomeattr_s;
#define IOFUNC_ATTR_T struct Metronomeattr_s
struct Metronomeattrocb_s;
#define IOFUNC_OCB_T struct Metronomeattrocb_s

#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/netmgr.h>
#include <sys/neutrino.h>
#include "./metronome.h"

char data[255];
int metronome_coid;

int bpm; // beats per minute
int time_signature_TOP;
int time_signature_BOTTOM;
int intervalsPerBeat;
int nanoSecs;
float secsPerBeat;
float secondPerMeasure;
int patternIndex;
int patternLen;

//DONE
int io_read(resmgr_context_t *ctp, io_read_t *msg, Metronomeattrocb_t *mocb) {

	int nb;

	if (mocb->buffer == NULL)
		return 0;

	int i;

	//check device
	if (mocb->ocb.attr->device == Metronome) {

		//calc seconds per beat
		secsPerBeat = 60 / (float) bpm;

		//calc second per measure
		secondPerMeasure = secsPerBeat * time_signature_TOP;

		//Get intervalsPerBeat from table
		for (i = 0; i < 8; i++) {

			if (t[i].time_signature_top == time_signature_TOP
					&& t[i].time_signature_bottom == time_signature_BOTTOM) {
				intervalsPerBeat = t[i].intervalsPerBeat;
				break;
			}

		}

		secsPerBeat = secondPerMeasure / intervalsPerBeat;

		//calc nanoSecs
		nanoSecs = secsPerBeat * 1e+9;

		sprintf(mocb->buffer,
				"[metronome: %d beats/min, time signature %d/%d, secs-per-beat: %.2f, nanoSecs: %ld]\n",
				bpm, time_signature_TOP, time_signature_BOTTOM, secsPerBeat,
				nanoSecs);

	} else if (mocb->ocb.attr->device == Metronome_help) {

		sprintf(mocb->buffer,
				"Metronome Resource Manager (ResMgr)\n\n"
						"Usage: metronome <bpm> <ts-top> <ts-bottom>\n\n"
						"API:\n"
						"pause [1-9]\t\t\t- pause the mtronome for 1-9 seconds\n"
						"quit\t\t\t\t- quit the metronome\n"
						"set <bpm> <ts-top> <ts-bottom>\t- set the metronome to <bpm> ts-top/ts-bottom\n"
						"start\t\t\t\t- start the metronome from stopped state\n"
						"stop\t\t\t\t- stop the metronome; use 'start' to resume\n");
	}

	nb = strlen(mocb->buffer);

	//test to see if we have already sent the whole message.
	if (mocb->ocb.offset == nb)
		return 0;

	//We will return which ever is smaller the size of our data or the size of the buffer
	nb = min(nb, msg->i.nbytes);

	//Set the number of bytes we will return
	_IO_SET_READ_NBYTES(ctp, nb);

	//Copy data into reply buffer.
	SETIOV(ctp->iov, mocb->buffer, nb);

	//update offset into our data used to determine start position for next read.
	mocb->ocb.offset += nb;

	//If we are going to send any bytes update the access time for this resource.
	if (nb > 0)
		mocb->ocb.attr->attr.flags |= IOFUNC_ATTR_ATIME;

	return (_RESMGR_NPARTS(1));
}

int io_write(resmgr_context_t *ctp, io_write_t *msg, Metronomeattrocb_t *mocb) {
	int nb = 0;

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		/* have all the data */
		char *buf;
		char *alert_msg;
		int i, pauseValue;	//changed small integer from lab7 to pauseValue;
		buf = (char *) (msg + 1);

		if (mocb->ocb.attr->device == Metronome) {

			//Note to self: This will check for "pause" then when it finds it, it knows this line should only have two things, pause + " " + pauseValue.
			//Loops twice to get the second value which is the pause value
			if (strstr(buf, "pause") != NULL) {

				for (i = 0; i < 2; i++) {

					alert_msg = strsep(&buf, " ");

				}

				if (alert_msg != NULL) {
					pauseValue = atoi(alert_msg);

					if (pauseValue >= 1 && pauseValue <= 9) {

						MsgSendPulse(metronome_coid, SchedGet(0, 0, NULL),
						PAUSE_PULSE, pauseValue);

					} else {

						fprintf(stderr,
								"pause <int> - <int> must be in range : 1 - 9\n");

					}
				} else {

					fprintf(stderr,
							"pause <int> - <int> must be in range : 1 - 9\n");

				}
			} else if (strstr(buf, "quit") != NULL) {

				MsgSendPulse(metronome_coid, SchedGet(0, 0, NULL),
				QUIT_PULSE, 0);

			} else if (strstr(buf, "start") != NULL) {

				MsgSendPulse(metronome_coid, SchedGet(0, 0, NULL),
				START_PULSE, 0);

			} else if (strstr(buf, "set") != NULL) {

				//My method for avoiding and set operations if any of the required param are not entered
				//either all pass and get entered or none do
				int bpmValidate;
				int time_signature_TOPValidate;
				int time_signature_BOTTOMValidate;

				for (i = 0; i < 4; i++) {

					alert_msg = strsep(&buf, " ");

					if (alert_msg == NULL) {
						break;
					}

					switch (i) {

					case 1:

						bpmValidate = atoi(alert_msg);
						break;
					case 2:

						time_signature_TOPValidate = atoi(alert_msg);
						break;
					case 3:

						time_signature_BOTTOMValidate = atoi(alert_msg);
						break;
					}

				}

				if (alert_msg == NULL) {
					fprintf(stderr, "Usage: set <bpm> <ts-top> <ts-bottom>\n",
							buf);
				} else {

					bpm = bpmValidate;
					time_signature_TOP = time_signature_TOPValidate;
					time_signature_BOTTOM = time_signature_BOTTOMValidate;

					MsgSendPulse(metronome_coid, SchedGet(0, 0, NULL),
					SET_PULSE, 0);
				}
			} else if (strstr(buf, "stop") != NULL) {

				MsgSendPulse(metronome_coid, SchedGet(0, 0, NULL),
				STOP_PULSE, 0);

			} else {

				fprintf(stderr, "Error  - '%s' is not a valid command\n", buf);

			}

		} else {
			fprintf(stderr,
					"Error - cannot write to device /dev/local/metronome-help\n");
		}

		nb = msg->i.nbytes;
	}
	_IO_SET_WRITE_NBYTES(ctp, nb);

	if (msg->i.nbytes > 0)
		mocb->ocb.attr->attr.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS(0));
}

int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra) {
	if ((metronome_coid = name_open("metronome", 0)) == -1) {

		perror("name_open failed.");
		return EXIT_FAILURE;

	}

	return (iofunc_open_default(ctp, msg, &handle->attr, extra));
}

void* metronome_thread() { // create thread in main; thread runs this function
	// 							purpose: "drive" metronome

	name_attach_t *attach; // receives pulse from interval timer; each time the timer expires
	// receives pulses from io_write (quit and pause <int>)

	my_message_t msg;
	int rcvid;
	int resmgr_coid;
	int i;
	struct sigevent event;
	struct itimerspec itime;
	timer_t timer_id;

	// Phase I - create a named channel to receive pulses
	attach = name_attach( NULL, "metronome", 0);

	if (attach == NULL) {
		fprintf(stderr, "failed to create the channel.\n");
		exit(EXIT_FAILURE);
	}

	resmgr_coid = name_open("/dev/local/metronome", 0);

	//calc seconds per beat
	secsPerBeat = 60 / (float) bpm;

	//calc second per measure
	secondPerMeasure = secsPerBeat * time_signature_TOP;

	//Get intervalsPerBeat from table
	for (i = 0; i < 8; i++) {

		if (t[i].time_signature_top == time_signature_TOP
				&& t[i].time_signature_bottom == time_signature_BOTTOM) {
			intervalsPerBeat = t[i].intervalsPerBeat;
			break;
		}

	}

	secsPerBeat = secondPerMeasure / intervalsPerBeat;

	//calc nanoSecs
	nanoSecs = secsPerBeat * 1e+9;

	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0, attach->chid,
	_NTO_SIDE_CHANNEL, 0);
	event.sigev_priority = 10;
	event.sigev_code = METRONOME_PULSE;

	//Timer
	timer_create(CLOCK_MONOTONIC, &event, &timer_id);
	itime.it_value.tv_sec = 0;
	itime.it_value.tv_nsec = nanoSecs;
	itime.it_interval.tv_sec = 0;
	itime.it_interval.tv_nsec = nanoSecs;
	timer_settime(timer_id, 0, &itime, NULL);

	int patternCharIndex = 0;

	// Phase II - receive pulses from interval timer OR io_write(pause, quit)
	for (;;) {
		rcvid = MsgReceivePulse(attach->chid, &msg, sizeof(msg), NULL);

		if (rcvid == 0) {
			switch (msg.pulse.code) {

			case METRONOME_PULSE:

				//    display the beat to stdout
				//    must handle 3 cases:
				//    start-of-measure: |1

				if (patternCharIndex == 0) {

					printf("|1");
					patternCharIndex = 2;
					break;
				}

				//    mid-measure: the symbol, as seen in the column
				//    "Pattern for Intervals within Each Beat"
				//    Get intervalsPerBeat from table
				for (i = 0; i < 8; i++) {

					if (t[i].time_signature_top == time_signature_TOP
							&& t[i].time_signature_bottom
									== time_signature_BOTTOM) {
						patternIndex = i;
						patternLen = strlen(t[patternIndex].pattern);
						break;
					}

				}

				if (patternCharIndex < patternLen) {
					printf("%c", t[patternIndex].pattern[patternCharIndex]);
					patternCharIndex++;
				}

				//    end-of-measure: \n
				else {
					printf("\n");
					patternCharIndex = 0;
				}

				break;

			case START_PULSE:

				//If stops the timer from being changed if it is already started
				if (itime.it_interval.tv_nsec == 0) {

					itime.it_value.tv_sec = 0;
					itime.it_value.tv_nsec = nanoSecs;
					itime.it_interval.tv_sec = 0;
					itime.it_interval.tv_nsec = nanoSecs;
					timer_settime(timer_id, 0, &itime, NULL);
				}

				break;

			case STOP_PULSE:

				//If stops the timer from being changed if it is already stopped
				if (itime.it_interval.tv_nsec != 0) {

					itime.it_value.tv_sec = 0;
					itime.it_value.tv_nsec = nanoSecs;
					itime.it_interval.tv_sec = 0;
					itime.it_interval.tv_nsec = 0;
					timer_settime(timer_id, 0, &itime, NULL);
				}

				break;

			case SET_PULSE:

				for (i = 0; i < 8; i++) {

					if (t[i].time_signature_top == time_signature_TOP
							&& t[i].time_signature_bottom
									== time_signature_BOTTOM) {
						patternIndex = i;
						patternLen = strlen(t[i].pattern);
						break;
					}

				}

				itime.it_value.tv_nsec = nanoSecs;
				itime.it_value.tv_sec = 0;
				itime.it_interval.tv_sec = 0;
				itime.it_interval.tv_nsec = nanoSecs;
				timer_settime(timer_id, 0, &itime, NULL);

				break;

			case PAUSE_PULSE:

				itime.it_value.tv_nsec = nanoSecs;
				itime.it_value.tv_sec = msg.pulse.value.sival_int;
				itime.it_interval.tv_sec = 0;
				itime.it_interval.tv_nsec = nanoSecs;
				timer_settime(timer_id, 0, &itime, NULL);
				break;

			case QUIT_PULSE:
				// implement Phase III:
				timer_delete(timer_id);
				name_detach(attach, 0);
				name_close(resmgr_coid);
				ConnectDetach(event.sigev_coid);
				exit(EXIT_SUCCESS);

				break;
			}

		}
		fflush(stdout);

	}

	return NULL;
}

int main(int argc, char *argv[]) {

	dispatch_t* dpp;
	resmgr_io_funcs_t io_funcs;
	resmgr_connect_funcs_t connect_funcs;
	dispatch_context_t *ctp;

	int i;

//verify number of command-line args
	if (argc != 4) {
		//print usage message
		fprintf(stderr,
				"Missing Arguments\n Usage: <beats-per-minute> <time-signature-top> <time-signature-bottom>\n");
		exit(EXIT_FAILURE);

	}

//process the command-line arguments:
	bpm = atoi(argv[1]);
	time_signature_TOP = atoi(argv[2]);
	time_signature_BOTTOM = atoi(argv[3]);

	iofunc_funcs_t metronome_ocb_funcs = {
	_IOFUNC_NFUNCS, metronome_ocb_calloc, metronome_ocb_free };

	iofunc_mount_t metronome_mount = { 0, 0, 0, 0, &metronome_ocb_funcs };

	if ((dpp = dispatch_create()) == NULL) {

		fprintf(stderr, "%s:  Unable to allocate dispatch context.\n", argv[0]);
		return (EXIT_FAILURE);

	}

	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
	_RESMGR_IO_NFUNCS, &io_funcs);
	connect_funcs.open = io_open;
	io_funcs.read = io_read;
	io_funcs.write = io_write;

//register devices
	for (i = 0; i < NumDevices; i++) {
		iofunc_attr_init(&metronomeattrs[i].attr, S_IFCHR | 0666, NULL, NULL);
		metronomeattrs[i].device = i;
		metronomeattrs[i].attr.mount = &metronome_mount;
		resmgr_attach(dpp, NULL, devnames[i], _FTYPE_ANY, 0, &connect_funcs,
				&io_funcs, &metronomeattrs[i]);
	}

	ctp = dispatch_context_alloc(dpp);

	pthread_attr_t attr;

	pthread_attr_init(&attr);

	if (pthread_create(NULL, &attr, &metronome_thread, NULL) == -1) {
		fprintf(stderr, "Error creating thread\n");
		return EXIT_FAILURE;
	}

	pthread_attr_destroy(&attr);

	while (1) {

		ctp = dispatch_block(ctp);
		dispatch_handler(ctp);

	}

	return EXIT_SUCCESS;

}

Metronomeattrocb_t * metronome_ocb_calloc(resmgr_context_t *ctp,
		Metronomeattr_t *tattr) {
	Metronomeattrocb_t *tocb;
	tocb = calloc(1, sizeof(Metronomeattrocb_t));
	tocb->ocb.offset = 0;
	return (tocb);
}

void metronome_ocb_free(Metronomeattrocb_t *tocb) {
	free(tocb);
}
