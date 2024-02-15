#ifndef SRC_METRONOME_H_
#define SRC_METRONOME_H_

typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;

#define NumDevices 2
#define Metronome 0
#define Metronome_help 1

//pulses
#define METRONOME_PULSE _PULSE_CODE_MINAVAIL
#define PAUSE_PULSE 1
#define START_PULSE 2
#define QUIT_PULSE 3
#define SET_PULSE 4
#define STOP_PULSE 5

IOFUNC_OCB_T* metronome_ocb_calloc(resmgr_context_t *ctp, IOFUNC_ATTR_T *tattr);
void metronome_ocb_free(IOFUNC_OCB_T *tocb);

char *devnames[NumDevices] = { "/dev/local/metronome",
		"/dev/local/metronome-help" };

typedef struct Metronomeattr_s {
	iofunc_attr_t attr; //i did it again Gerry
	int device;
} Metronomeattr_t;

typedef struct Metronomeattrocb_s {
	iofunc_ocb_t ocb;
	char buffer[50];
} Metronomeattrocb_t;

Metronomeattr_t metronomeattrs[NumDevices];

typedef struct {
	int time_signature_top;
	int time_signature_bottom;
	int intervalsPerBeat;
	char pattern[50];
} DataTableRow;

//check if first two vals match user input, if yes take 3rd and 4th column
DataTableRow t[] = { { 2, 4, 4, "|1&2&" }, { 3, 4, 6, "|1&2&3&" }, { 4, 4, 8,
		"|1&2&3&4&" }, { 5, 4, 10, "|1&2&3&4-5-" }, { 3, 8, 6, "|1-2-3-" }, { 6,
		8, 6, "|1&a2&a" }, { 9, 8, 9, "|1&a2&a3&a" }, { 12, 8, 12,
		"|1&a2&a3&a4&a" } };

#endif /* SRC_METRONOME_H_ */
