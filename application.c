#include "application.h"

static INFO_SYS info_def = { "1.7a", 0, 0, 0 };

bool startangle_validator(u8  str[])
{
	//Must be 10
	if(str[1] == '0' && str[0] != '1')
	{
		return false; 
	}
	//Greater than 10
	if(str[1] > '0' && str[1] <= '9')
	{
		return false; 
	}
	// 2 - 9
	if(str[1] <= '0' && str[1] >= '9' && str[0] < '2') 
	{			
		return false; 
	}	
	return true;
}
void startangle_updater(u8  str[])
{
	u32 set_ver = (str[0] - 48) * 100 + (str[1] - 48) * 10 + (str[2] - 48);
	if (str[1] == '0' && str[0] == '1') set_ver = 10;
	else  set_ver = str[0] - 48;

	info_def.start_angle = set_ver;
}
void startangle_printer(char *buffer)
{
	sprintf(buffer, "StartAngle=%d", info_def.start_angle);
}

bool gear_validator(u8  str[])
{
	//Cannot be greater than 10
	if(str[1] <= '9' && str[1] > '0')
	{
		return false; 
	}
	//Cannot be greater than 4, not less than 1
	if(str[0] >= '5' || str[0] < '0')
	{
		return false; 
	}
	return true;
}
void gear_updater(u8  str[])
{
	u32 set_ver = (str[0] - 48);
	info_def.torque_level = set_ver;
}
void gear_printer(char *buffer)
{
	sprintf(buffer, "Gear=%d", info_def.torque_level);
}

bool motortimecnt_validator(u8  str[])
{
	u16 set_ver = atoi((char const *)str);
	if (set_ver != 0)
	{
		return false;
	}
	return true;
}
void motortimecnt_updater(u8  str[])
{
	u32 set_ver = atoi((char const *)str);
	info_def.moto_timecnt = set_ver;
}
void motortimecnt_printer(char *buffer)
{
	sprintf(buffer, "MotorTimeCnt=%d", info_def.moto_timecnt);
}

bool ver_validator(u8  str[])
{
	return true;
}
void ver_updater(u8  str[])
{
	NULL;
}
void ver_printer(char *buffer)
{
	sprintf(buffer, "Ver=%s", info_def.ver);
}
static void init(void)
{
	Disk.register_entry(0, "StartAngle", "2", "#(2~10)", &startangle_validator, &startangle_updater, &startangle_printer);
	Disk.register_entry(1, "Gear", "0", "#(0~5)", &gear_validator, &gear_updater, &gear_printer);
	Disk.register_entry(2, "MotorTimeCnt", "0", "#ReadOnly", &motortimecnt_validator, &motortimecnt_updater, &motortimecnt_printer);
	Disk.register_entry(3, "Ver", "1.7a", "#ReadOnly", &ver_validator, &ver_updater, &ver_printer);
}

const struct application Application= { 
	.init = init,		
};

