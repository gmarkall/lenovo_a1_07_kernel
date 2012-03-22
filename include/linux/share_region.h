/*
 * 1. The size of share region is 4KB.
 * 2. The share region locates at the end of the memory. 
 */
#define CRC32_RESIDUE 0xdebb20e3UL /* ANSI X3.66 residue */

#ifndef PAGE_SIZE
# define PAGE_SIZE 4096
#endif

#define SHARE_REGION_SIZE PAGE_SIZE
#define UPGRADE_MEM_SIZE PAGE_SIZE

#define RESERVED_SIZE_T (SHARE_REGION_SIZE - sizeof(unsigned int) - \
											sizeof(enum BoardID_enum) - \
											sizeof(unsigned int) - \
											(sizeof(char)*64) - \
											sizeof(unsigned int) - \
											sizeof(unsigned int) -\	
											sizeof(unsigned long))

//#define RESERVED_SIZE (SHARE_REGION_SIZE - sizeof(unsigned int) - sizeof(enum BoardID_enum) - sizeof(unsigned long) - (sizeof(char)*64) - sizeof(unsigned int) - sizeof(unsigned int) - sizeof(struct share_region_flags))
#define RESERVED_SIZE (UPGRADE_MEM_SIZE - sizeof(struct share_region_flags) - sizeof(unsigned long))

/* Board Version */
/* <--LH_SWRD_CL1_Mervins@2011.06.28:for hardware version. */
enum BoardID_enum {
	BOARD_VERSION_UNKNOWN = 0,
	BOARD_ID_PRO  = 1,
	BOARD_ID_EVT  = 2,
	BOARD_ID_DVT1 = 3,
	BOARD_ID_WIFI_DVT2 = 4,
	BOARD_ID_WIFI_PVT = 5,
	BOARD_ID_WIFI_MP = 6,
	BOARD_ID_3G_DVT2 = 7,
	BOARD_ID_3G_PVT  = 8,
	BOARD_ID_3G_MP   = 9,
};
/* LH_SWRD_CL1_Mervins@2011.06.28--> */	

/* Function Stat */
enum FlagStat_enum {
	FLAG_NOMALL						= 0,
	FLAG_FACTORY_DEFAULT  = 1,
	FLAG_RECOVER_MODE     = 2,
	FLAG_SHUTDOWN3				= 4,
	FLAG_REBOOT						= 8,
};

/* Language ID */
enum Language_enum {
	LANG_ENGLISH						 = 0x0C09,
	LANG_END								 = 0xffff,
};

/* proc entry and it`s oprating functions. */
typedef struct flag_cb {
        char *path;
        unsigned long (*flag_get)(void);
        void (*flag_set)(unsigned long);
} flag_cb;


struct share_region_flags
{
        unsigned int boot_flag;
        unsigned int update_flag;
};

struct upgrade_mem_t
{
        struct share_region_flags flags;
        unsigned char reserved[RESERVED_SIZE];
        unsigned long checksum;
};

struct share_region_t
{
	unsigned int flags;
	enum BoardID_enum hardware_id;
	unsigned int language;
	char software_version[64];
        unsigned int debug_flag;
	unsigned int status_3G_exist;
	unsigned char reserved[RESERVED_SIZE_T];
	unsigned long checksum;
};

typedef struct share_region_t share_region_t;
typedef struct upgrade_mem_t upgrade_mem_t; 
#define SHARE_REGION_BASE 	(0x9FE00000 - 0x10000)
#define MEM_256MB_SHARE_REGION_BASE 	(0x8FE00000 - 0x10000)
#define UPGRADE_MEM_BASE (0x84000000)
//extern upgrade_mem_t * const upgrade_mem;
extern  share_region_t const *share_region;
//extern upgrade_mem_t const *upgrade_mem;
extern void get_share_region(void);
extern int ep_get_hardware_id(void);
extern int ep_get_software_version(char *ver, const int len);
extern int ep_get_debug_flag(void);
extern int ep_get_3G_exist(void);
