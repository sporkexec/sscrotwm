#define FPARSELN_UNESCESC	0x01
#define FPARSELN_UNESCCONT	0x02
#define FPARSELN_UNESCCOMM	0x04
#define FPARSELN_UNESCREST	0x08
#define FPARSELN_UNESCALL	0x0f

char   *fgetln(FILE *, size_t *);
char   *fparseln(FILE *, size_t *, size_t *, const char [3], int);

#ifndef WAIT_ANY
#define WAIT_ANY		(-1)
#endif

/* there is no limit to ulrich drepper's crap */
#ifndef TAILQ_END
#define	TAILQ_END(head)			NULL
#endif
