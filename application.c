#include "application.h"

u8 example1, example2;
bool example1_validator(u8  str[])
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
void example1_updater(u8  str[])
{
	u32 set_value = (str[0] - 48) * 100 + (str[1] - 48) * 10 + (str[2] - 48);
	if (str[1] == '0' && str[0] == '1') set_value = 10;
	else  set_value = str[0] - 48;

	example1 = set_value;
}
void example1_printer(char *buffer)
{
	sprintf(buffer, "example1=%d", example1);
}

bool example2_validator(u8  str[])
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
void example2_updater(u8  str[])
{
	u32 set_value = (str[0] - 48);
	example2 = set_value;
}
void example2_printer(char *buffer)
{
	sprintf(buffer, "example2=%d", example2);
}


static void init(void)
{
	Disk.register_entry("example1", "2", "#(2~10)", &example1_validator, &example1_updater, &example1_printer);
	Disk.register_entry("example2", "0", "#(0~5)", &example2_validator, &example2_updater, &example2_printer);	
}

const struct application Application= { 
	.init = init,		
};

