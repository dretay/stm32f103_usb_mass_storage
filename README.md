## STM32HAL-based library that uses internal FLASH to store an editable text file exposed over USB ##

### Overview ###

Sometimes when developing a uController project it is convenient to allow variables to be edited without needing to recomplile code. In such cases it's typical to either attach an EEPROM or set aside a portion of the internal uController Flash an simulate an eeprom. Both of these approaches are limited in that they require developer tools (either a programmer or something like a buspirate) to query and update configuration values. Thus I wrote this simple library that exposes a very simple FAT12 filesystem over usb that contains a single `config.txt` file. This file can have entries in it corresponding to configuration values that an end-user should be able to edit. 
![](https://github.com/dretay/stm32f103_usb_mass_storage/blob/master/screenshot.png)

### Main Features ###
- Only takes up 8K of flash on the uController.
- Based entirely on STM32 CubeMX libraries so should be portable across all STM32 uControllers (although only tested on an STM32F103C8).
- Can pass in validators to ensure user-entered data is valid. 
- When a user attempts to save a bad entry the file is "restored" to a set of reasonable default values. 

### Summary ###
Make sure to check out the full example [here](https://github.com/dretay/stm32f103_usb_mass_storage/blob/master/application.c) however when everything is set up all you need to do to create a configuration file is call helpers like this:
```
Disk.register_entry("example1", "2", "#(2~10)", &example1_validator, &example1_updater, &example1_printer);
Disk.register_entry("example2", "0", "#(0~5)", &example2_validator, &example2_updater, &example2_printer);	
```
### Details ###
First make sure that in the [linker](https://github.com/dretay/stm32f103_usb_mass_storage/blob/master/STM32F103C8_flash.lds#L11) file for your project you set aside enough space in flash to store at least 8K.:
```
MEMORY
{
	FLASH (RX) : ORIGIN = 0x08000000, LENGTH = 48K
	EEPROM (RWX) : ORIGIN = 0x0801BC00, LENGTH = 16K
	SRAM (RWX) : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = 0x20005000;

SECTIONS
{
	.user_data :
	{
		. = ALIGN(4);
		*(.user_data)
		. = ALIGN(4);
	} > EEPROM
	...
```
If you do not use the same flash settings I used here you'll probably also need to modify the starting page of flash that should be used [here](https://github.com/dretay/stm32f103_usb_mass_storage/blob/master/disk.c#L8).
```
static uc32 APP_BASE = ADDR_FLASH_PAGE_111;
```
Finally just create some file entries. 
```
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
Disk.register_entry("example1", "2", "#(2~10)", &example1_validator, &example1_updater, &example1_printer);
```
