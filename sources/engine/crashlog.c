// COMPILE: -lang c++ -bool off
/* crashlog.c -- debug logging. Notes: Research/notes/crashlog.md */
#include <stdio.h>

#include "burger.h"
#include "macport.h"
#include "crashlog.h"

EXTERN_C_BEGIN

// GLOBAL: EVNOVA 0x007cdab0
static FILE *logfile;

/* crashlog_open (mac _crashlog_open). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470000
// MAC: _crashlog_open i386 0x000a4778 ppc 0x000b02bc
void crashlog_open(char *name)
{
    DateTimeRec dtr;

    logfile = fopen(name, "r");
    if (logfile == NULL)
        return;
    fclose(logfile);
    logfile = fopen(name, "w");
    if (logfile) {
        fprintf(logfile, "EV Nova debug log\n\n");
        GetTime(&dtr);
        fprintf(logfile, "Output on %i/%i/%i at ", dtr.month, dtr.day, dtr.year);
        if (dtr.hour < 12)
            fprintf(logfile, "%i:%02i AM", dtr.hour, dtr.minute);
        else if (dtr.hour == 12)
            fprintf(logfile, "%i:%02i PM", dtr.hour, dtr.minute);
        else
            fprintf(logfile, "%i:%02i PM", dtr.hour - 12, dtr.minute);
        fprintf(logfile, "\n\n");
    }
}

/* crashlog_close (mac _crashlog_close). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470100
// MAC: _crashlog_close i386 0x000a48a9 ppc 0x000b03c8
void crashlog_close(void)
{
    if (logfile) {
        fclose(logfile);
        logfile = NULL;
    }
}

/* crashlog_print (mac _crashlog_print). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470120
// MAC: _crashlog_print i386 0x000a48cc ppc 0x000b040c
void crashlog_print(const char *str)
{
    if (logfile) {
        fprintf(logfile, "%s", str);
        fflush(logfile);
    }
}


/* crashlog_print_pstr (mac _crashlog_print_pstr). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470150
// MAC: _crashlog_print_pstr i386 0x000a4a44 ppc 0x000b0660
void crashlog_print_pstr(StringPtr pstr)
{
    char buf[256];

    if (logfile) {
        strncpy(buf, (char *)pstr, 255);
        PtoCstr((unsigned char *)buf);
        fprintf(logfile, "%s", buf);
        fflush(logfile);
    }
}

/* crashlog_print_mdy (mac _crashlog_print_mdy). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004701b0
// MAC: _crashlog_print_mdy i386 0x000a4923 ppc 0x000b049c
void crashlog_print_mdy(DateTimeRec *dtr)
{
    if (logfile) {
        fprintf(logfile, "%02i/%02i/%i", dtr->month, dtr->day, dtr->year);
        fflush(logfile);
    }
}

/* crashlog_print_num (mac _crashlog_print_num). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004701f0
// MAC: _crashlog_print_num i386 0x000a496d ppc 0x000b050c
void crashlog_print_num(int num)
{
    if (logfile) {
        fprintf(logfile, "%i", num);
        fflush(logfile);
    }
}

/* crashlog_print_ostype (mac _crashlog_print_ostype, dead on win: uncalled, the linker strips it).
   STATE: see the mac baseline. OPEN: none */
// MAC: _crashlog_print_ostype i386 0x000a49a3 ppc 0x000b056c
void crashlog_print_ostype(OSType type)
{
    short i;

    for (i = 0; i < 4; i++) {
        if (logfile) {
            fprintf(logfile, "%c", ((char *)&type)[i]);
            fflush(logfile);
        }
    }
}

/* crashlog_mission_bit_set (mac _crashlog_mission_bit_set). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470220
// MAC: _crashlog_mission_bit_set i386 0x000a4ab9 ppc 0x000b06f0
void crashlog_mission_bit_set(char *str, char *pstr, short num, short num2, char *setString)
{
    char buf[255];

    if (logfile && setString) {
        if (strlen(setString))
            fprintf(logfile, "evaluating ncb set string '%s'", setString);
        if (str) {
            if (strlen(str)) {
                if (*setString)
                    fprintf(logfile, " (%s", str);
                else
                    fprintf(logfile, "%s", str);
                if (pstr != NULL) {
                    if (strlen(pstr)) {
                        strcpy(buf, pstr); // BUG (all versions): no length bound on buf
                        if (num)
                            PtoCstr((unsigned char *)buf);
                        fprintf(logfile, " %s", buf);
                        if (num2 > 0)
                            fprintf(logfile, " (%i)", num2);
                    }
                }
                else {
                    if (num)
                        fprintf(logfile, " %i", num);
                }
                if (*setString)
                    fprintf(logfile, ")");
            }
        }
        fprintf(logfile, "\n");
        fflush(logfile);
    }
}

/* crashlog (mac _crashlog). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x00470390
// MAC: _crashlog i386 0x000a49e2 ppc 0x000b05cc
void crashlog(char *str)
{
    if (logfile) {
        fprintf(logfile, "%s\n", str);
        fflush(logfile);
    }
}

/* crashlog_isactive (mac _crashlog_isactive). STATE: MATCH, byte-identical. OPEN: none */
// FUNCTION: EVNOVA 0x004703c0
// MAC: _crashlog_isactive i386 0x000a4a18 ppc 0x000b062c
Boolean crashlog_isactive(void)
{
    return logfile ? 1 : 0;
}

/* crashlog_get_file (mac _crashlog_get_file, dead on win: uncalled, the linker strips it).
   STATE: see the mac baseline. OPEN: none */
// MAC: _crashlog_get_file i386 0x000a4a29 ppc 0x000b0640
FILE *crashlog_get_file(void)
{
    return logfile ? logfile : stderr;
}

EXTERN_C_END
