#ifndef _TICKET_H
#define _TICKET_H

// uncomment this line to enable passing invalid arguments to client processes
// (or define it through command line/Makefile)
//#define ADDITIONAL_CHECK

#define MAX_ROOM_SEATS 9999             /* maximum number of room seats/tickets available       */
#define MAX_CLI_SEATS 99                /* maximum number of seats/tickets per request          */
#define WIDTH_PID 5                     /* length of the PID string                             */
#define WIDTH_XXNN 5                    /* length of the XX.NN string (reservation X out of N)  */
#define WIDTH_SEAT 4                    /* length of the seat number id string                  */
#define CLIENT_TIMEOUT 60
#define CLIENT_TIMEOUT_LEN 3

// maximum length of the preference list string (the +1 is for the space character)
#define MAX_PREFERENCES_LEN ((WIDTH_SEAT+1)*(MAX_CLI_SEATS))

#define MAX_TOKEN_LEN 1024              /* length of the largest string within config file      */
#define PREF_LIST_END "END"             /* terminator string for the list of seat preferences   */

#endif //_TICKET_H