#ifndef HISTORY_H
#define HISTORY_H

/* Number of HistItem entries in history buffer */
#define HISTSIZE 	512

typedef struct {
   uint64_t timeStamp;
   char *message;
   char padding;
   char padding1;
   unsigned char associatedData;
   /* add more fields here � */
} HistItem;
/*
* Define bits to enable/disable circular history debugging 
* for certain subsystems
*/
#define HISTORY_BOOT		0x00000001
#define HISTORY_XYZ_MODULE		0x00000002
/* add more subsystem bit defines here� */


/* Variable value used in history events with no associated data */	
#define NOVAL				0xff

typedef unsigned int UINT32;

#ifdef HISTORY

#ifdef __cplusplus
extern "C" {
#endif
   void hist_init(UINT32 subsystemsEnabled);
   void hist_enableSubsystem(UINT32 bit);
   void hist_disableSubsystem(UINT32 bit);
   void hist_add(UINT32 subsystem, char *msg);
   void hist_addx(UINT32 subsystem, char *msg, unsigned char data);
   void hist_print(int doReturn);
#ifdef __cplusplus
}
#endif

/* MACRO�s for XYZ module */
#define HIST_XYZ(s)		hist_add(HISTORY_XYZ_MODULE,s)
#define HIST_XYZx(s,x)	hist_add(HISTORY_XYZ_MODULE,s,(UINT32)x);

#else
   #define hist_init(subsystemsEnabled)
   #define hist_enableSubsystem(bit)
   #define hist_disableSubsystem(bit)
   #define hist_add(subsystem, msg)
   #define hist_addx(subsystem, msg, data)
   #define hist_print(doReturn)
   
   /* MACRO�s for XYZ module */
   #define HIST_XYZ(s)	
   #define HIST_XYZx(s,x)	
#endif  


#endif /* HISTORY_H */
