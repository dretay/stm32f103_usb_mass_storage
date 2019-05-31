#pragma once

#include <string.h>

#include "stm32f1xx_hal.h"
#include "flashpages.h"
#include "LOG.h"
#include "types_shortcuts.h"

typedef struct {
	u8 ver[16];
	u8 start_angle;   //Starting angle
	u8 torque_level;   //Torque level
	u32 moto_timecnt;   //Motor rotation time
}INFO_SYS;

struct disk {
	void(*init)(void);		
	u8(*Disk_SecWrite)(u8* pbuffer, u32 diskaddr, u32 length);
	void(*Disk_SecRead)(u8* pbuffer, u32 disk_addr);
	u32(*get_sector_size)(void);
	u32(*get_sector_count)(void);
};

extern const struct disk Disk;