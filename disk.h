#pragma once

#include <string.h>

#include "stm32f1xx_hal.h"
#include "flashpages.h"
#include "LOG.h"
#include "types_shortcuts.h"
#include "minmax.h"
#include <stdbool.h>

#define MAX_ENTRY_LABEL_LENGTH 32
typedef struct {
	u8 ver[16];
	u8 start_angle;   //Starting angle
	u8 torque_level;   //Torque level
	u32 moto_timecnt;   //Motor rotation time
}INFO_SYS;

typedef struct {
	char entry[MAX_ENTRY_LABEL_LENGTH];
	char comment[MAX_ENTRY_LABEL_LENGTH];
	char default_line[MAX_ENTRY_LABEL_LENGTH * 3];
	bool(*validate)(u8 str[]);
	void(*update)(u8 str[]);
	void(*print)(char *buffer);
} FILE_ENTRY;

struct disk {
	void(*init)(void);		
	u8(*Disk_SecWrite)(u8* pbuffer, u32 diskaddr, u32 length);
	void(*Disk_SecRead)(u8* pbuffer, u32 disk_addr);
	u32(*get_sector_size)(void);
	u32(*get_sector_count)(void);
	void(*register_entry)(int idx, char* entry, char* default_val, char* comment, void* validator, void* updater, void* printer);	
};

extern const struct disk Disk;