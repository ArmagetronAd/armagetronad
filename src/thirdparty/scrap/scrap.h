
/* Handle clipboard text and data in arbitrary formats */

/* Miscellaneous defines */
#define SCRAP_TEXT 0x54455854

extern int init_scrap(void);
extern int lost_scrap(void);
extern void put_scrap(int type, int srclen, char *src);
extern void get_scrap(int type, int *dstlen, char **dst);
