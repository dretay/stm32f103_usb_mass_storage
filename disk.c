#include "disk.h"


//constants
#define SECTOR_SIZE 512
#define SECTOR_CNT  4096
static uc32 APP_BASE = ADDR_FLASH_PAGE_111;
static uc8 CONFIG_CONT = 4; //todo: what are you?
static uc8 ROW_CONT = 35; //todo: what are you?
static uc8 FILE_CONT = 254;//todo: what are you?
static const char *gKey_words[] = { "StartAngle", "Gear", "MotorTimeCnt", "Ver" };
static const char *gDef_set[] = { "StartAngle=2", "Gear=0", "MotorTimeCnt=0", "Ver=1.7a" };
static const char *gSet_range[] = { "      #(2~10)\r\n", "            #(0~5)\r\n", "    #ReadOnly\r\n", "           #ReadOnly\r\n" };
static uc32 VOLUME = 0x40DD8D18;
static const u8 fat_data[] = { 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; //todo: what are you?

//globals
static u8   disk_buffer[0x2600]; 
static u32  disk_buffer_temp[(SECTOR_SIZE + 32 + 28) / 4];     
static u8  *pdisk_buffer_temp = (u8*) &disk_buffer_temp[0];
static u8 file_buffer[SECTOR_SIZE]; 
static u8 page_dirty_mask[16];
static INFO_SYS info_def = { "1.7a", 0, 0, 0 };

//pointers
static u8* FAT1_SECTOR = &disk_buffer[0x000];
static u8* FAT2_SECTOR = &disk_buffer[0x200];
static u8* ROOT_SECTOR = &disk_buffer[0x400];
static u8* VOLUME_BASE = &disk_buffer[0x416];
static u8* OTHER_FILES = &disk_buffer[0x420];
static u8* FILE_SECTOR = &disk_buffer[0x600];

uc8 BOOT_SEC[SECTOR_SIZE] = {
	0xEB, 0x3C, 0x90, 								// code to jump to the bootstrap code
	'm', 'k', 'd', 'o', 's', 'f', 's', 0x00,		// OEM ID
	0x00, 0x02, 									// bytes per sector
	0x01, 											// sectors per cluster
	0x08, 0x00,										// # of reserved sectors
    0x02, 											// FAT copies
    0x00, 0x02, 									// root entries
    0x50, 0x00, 									// total number of sectors
    0xF8, 											// media descriptor (0xF8 = Fixed disk)
    0x0c, 0x00,										// sectors per FAT
    0x01, 0x00, 									// sectors per track
    0x01, 0x00, 									// number of heads
    0x00, 0x00, 0x00, 0x00,							// hidden sectors
    0x00, 0x00, 0x00, 0x00, 						// large number of sectors
    0x00, 											// drive number
    0x00, 											// reserved
    0x29, 											// extended boot signature
    0xA2, 0x98, 0xE4, 0x6C,							// volume serial number
    'R','A','M','D','I','S','K',' ',' ',' ',' ',	// volume label    
    'F', 'A', 'T', '1', '2', ' ', ' ', ' '			// filesystem type
};

//util functions
static void Upper(u8* str, u16 len)
{
	u16 i;
	for (i = 0; i < len; i++)
	{
		if (str[i] >= 'a' && str[i] <= 'z')
		{
				str[i] -= 32;		
		}		
	}
}

//flash interface functions
static HAL_StatusTypeDef erase_flash_page(u32 Address)
{
	unsigned long page_error;
	static FLASH_EraseInitTypeDef EraseInitStruct;
	HAL_StatusTypeDef status;
	EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	EraseInitStruct.PageAddress = Address;
	EraseInitStruct.NbPages     = 1;
	status = HAL_FLASHEx_Erase(&EraseInitStruct, &page_error);
	if (status != HAL_OK)
	{
		_ERROR("Unable to erase flash page: ", status);
	}
	return status;
}
static HAL_StatusTypeDef write_flash_halfword(u32 Address, u16 Data)
{		
	HAL_StatusTypeDef status;
	//https://stackoverflow.com/questions/28498191/cant-write-to-flash-memory-after-erase
	//grr... looks like by default you can only write once to flash w/o clearing this...
	CLEAR_BIT(FLASH->CR, (FLASH_CR_PG));	
	status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, Data);
	if (status != HAL_OK)
	{
		_ERROR("Unable to write halfword: ", status);
	}
	return status;

}
u8 rewrite_dirty_flash_pages(void)
{
	u32 i, j;
	u8  result;
	u16 *f_buff;
	HAL_StatusTypeDef status;

	status = HAL_FLASH_Unlock();
	if (status != HAL_OK)
	{
		_ERROR("Unable to unlock flash: ", status);
	}
	for (i = 0; i < 16; i++) 
	{
		if (page_dirty_mask[i]) 
		{
			page_dirty_mask[i] = 0;
			erase_flash_page(APP_BASE + i * FLASH_PAGE_SIZE);
			f_buff = (u16*)&disk_buffer[i * FLASH_PAGE_SIZE];
			for (j = 0; j < FLASH_PAGE_SIZE; j += 2) 
			{
				if (write_flash_halfword((u32)(APP_BASE + i * FLASH_PAGE_SIZE + j), *f_buff++) != HAL_OK)
				{
					_ERROR("Unable to program flash at index ", j);
				}
			}
			break;
		}
	}
	if (HAL_FLASH_Lock() != HAL_OK)
	{
		_ERROR("Unable to lock flash", NULL);
	}
	return 0;
}

u8 rewrite_all_flash_pages(void)
{
	u16 i;
	u8  result;
	u16 *f_buff = (u16*)disk_buffer;
	HAL_StatusTypeDef status;

	status = HAL_FLASH_Unlock();
	if (status != HAL_OK)
	{
		_ERROR("Unable to unlock flash: ", status);
	}
	for (i = 0; i < 8; i++) {
		result = erase_flash_page(APP_BASE + i * FLASH_PAGE_SIZE);
		if (result != HAL_OK) 
		{
			return result;
		}
	}
	for (i = 0; i < (SECTOR_CNT/2); i += 2) {
		result = write_flash_halfword((u32)(APP_BASE + i), *f_buff++);
		if (result != HAL_OK) 
		{
			_ERROR("Error while programming flash at ", i);
			return result;
		}
	}
	if (HAL_FLASH_Lock() != HAL_OK)
	{
		_ERROR("Unable to lock flash", NULL);
	}
	return HAL_OK;
}

void set_version(u8 str[], u8 k)
{
	u32 set_ver = 0;

	switch (k) {
	case 0:
		set_ver = (str[0] - 48) * 100 + (str[1] - 48) * 10 + (str[2] - 48);
		if (str[1] == '0' && str[0] == '1') set_ver = 10;
		else  set_ver = str[0] - 48;

		info_def.start_angle = set_ver;
		break;
	case 1:
		set_ver = (str[0] - 48);
		info_def.torque_level = set_ver;
		break;
	case 2:
		set_ver = atoi((char const *)str);
		info_def.moto_timecnt = set_ver;
		break;
	default:
		break;
	}
}
u8 validate_file_line(u8  str[], u8 k)
{
	u16 set_ver;

	switch (k) {
		case 0:
			//Must be 10
			if (str[1] == '0' && str[0] != '1')
			{
				return 0; 
			}
			//Greater than 10
			if (str[1] > '0' && str[1] <= '9')
			{
				return 0; 
			}
			// 2 - 9
			if(str[1] <= '0' && str[1] >= '9' && str[0] < '2') 
			{			
				return 0; 
			}
			break;
		case 1:
			//Cannot be greater than 10
			if (str[1] <= '9' && str[1] > '0')
			{
				return 0; 
			}
			//Cannot be greater than 4, not less than 1
			if (str[0] >= '5' || str[0] < '0')
			{
				return 0; 
			}
			break;
		case 2:
			set_ver = atoi((char const *)str);
			if (set_ver != info_def.moto_timecnt)
			{
				return 0;
			}
			break;
		case 3:
			if (info_def.ver[0] == str[0] && info_def.ver[1] == str[1] && info_def.ver[2] == str[2])
			{
				return 1;
			}			
			return 0;		
			break;
		default:
			break;
	}
	return 1;
}
u8 validate_file(u8* p_file, u16 root_addr)
{
	u32 i, j, k, m, flag;
	u8 t_p[CONFIG_CONT][ROW_CONT];
	u8 str[FILE_CONT];
	u8 illegal = 0;
    
	m = 0;
	j = 0;

	memset(t_p, 0x00, CONFIG_CONT * ROW_CONT);
	memcpy((u8*)file_buffer, p_file, SECTOR_SIZE);
	for (k = 0; k < CONFIG_CONT; k++)
	{
		//Take the CONFIG_CONT line
	    j = 0;
		for (i = m; i < strlen((char *)file_buffer); i++) 
		{
			//Calculate where the set value string is located
		    if(file_buffer[i] == 0x0D && file_buffer[i + 1] == 0x0A) 
			{
				break;
			}
			else 
			{
				if (j < ROW_CONT)
				{
					t_p[k][j++] = file_buffer[i];					
				}
				m++;
			}
		}
		t_p[k][j] = '\0';
		m = i + 2;
	}
	for (k = 0; k < CONFIG_CONT; k++) {
		//Analyze the CONFIG_CONT line
	    if(memcmp(t_p[k], gKey_words[k], strlen(gKey_words[k])) == 0) 
		{
			//Find keywords
			flag = 0;
			for (i = strlen(gKey_words[k]); i < strlen((char *)t_p[k]); i++) 
			{
				//Is the setting value legal?
			    if((t_p[k][i] >= '0' && t_p[k][i] <= '9') || t_p[k][i] == '.') 
				{
					flag = 1;
					break;
				} 
				else if((t_p[k][i] != 0x20) && (t_p[k][i] != 0x3d)) 
				{
					//Space and equal sign
					flag = 0;
					break;
				}
			}
			if (flag && validate_file_line(t_p[k] + i, k)) 
			{
				//The setting value is legal
			    set_version(t_p[k] + i, k);
				if (k == 0) 
				{
					sprintf((char *)t_p[k], "StartAngle=%d", info_def.start_angle);
				}        
				else if (k == 1)
				{
					sprintf((char *)t_p[k], "Gear=%d", info_def.torque_level);
				}
				else if (k == 2)
				{  
					sprintf((char *)t_p[k], "MotorTimeCnt=%d", info_def.moto_timecnt);
				}
				else if (k == 3)
				{  
					sprintf((char *)t_p[k], "Ver=%s", info_def.ver);
				}
			}
			else 
			{
				//Setting value is illegal
				memset(t_p[k], 0, strlen((char *)t_p[k]));
				memcpy(t_p[k], gDef_set[k], strlen((char *)gDef_set[k]));
				illegal = 1;
			}
		} 
		else 
		{
			//keyword not found 
			memset(t_p[k], 0, strlen((char *)t_p[k]));
			memcpy(t_p[k], gDef_set[k], strlen((char *)gDef_set[k]));
			illegal = 1;
		}
	}
        
	memset(str, 0x00, FILE_CONT);
	m = 0;
	for (k = 0; k < CONFIG_CONT; k++) 
	{
		strcat((char *)str, (char *)t_p[k]);
		strcat((char *)str, (char *)gSet_range[k]);
	}
	m = strlen((char *)str);
	disk_buffer[FLASH_PAGE_SIZE + root_addr * 32 + 0x1C] = m % 256;
	disk_buffer[FLASH_PAGE_SIZE + root_addr * 32 + 0x1D] = m / 256;
    
	page_dirty_mask[(p_file - ROOT_SECTOR + 0x200) / FLASH_PAGE_SIZE] = 1;
	memcpy(p_file, str, strlen((char *)str));
	
	return illegal;

}
u8* find_file(u8* pfilename, u16* pfilelen, u16* root_addr)
{
	u16 n, sector;
	u8  str_name[11];
	u8* pdiraddr;

	pdiraddr = ROOT_SECTOR;

	for (n = 0; n < 16; n++) {
		memcpy(str_name, pdiraddr, 11);
		Upper(str_name, 11);
		if (memcmp(str_name, pfilename, 11) == 0) {
			memcpy((u8*)pfilelen, pdiraddr + 0x1C, 2);
			memcpy((u8*)&sector, pdiraddr + 0x1A, 2);
			return (u8*)FILE_SECTOR + (sector - 2) * SECTOR_SIZE;
		}

		pdiraddr += 32;
		root_addr++;
	}
	_INFO("file search did not find requested file", NULL);
	return NULL;
}
static u8 flush_file(void)
{
	u32 k, m;
	u8 illegal;
	u16 file_len;
	u8* p_file;
	u16 root_addr;
    
	root_addr = 0;
    
	if ((p_file = find_file("CONFIG  TXT", &file_len, &root_addr))) 
	{
		illegal = validate_file(p_file, root_addr);
		if (illegal) 
		{			
			if (rewrite_dirty_flash_pages() != HAL_OK)
			{
				_ERROR("Error rewriting flash", NULL);
			}
		}
	}
	else {
		memset(disk_buffer, 0x00, 0x2600);
		memcpy(ROOT_SECTOR, "CONFIG  TXT", 0xC);
		memcpy(FAT1_SECTOR, fat_data, 6);
		memcpy(FAT2_SECTOR, fat_data, 6);

		m = 0;
		for (k = 0; k < CONFIG_CONT; k++) 
		{
			memcpy(FILE_SECTOR + m, gDef_set[k], strlen((char *)gDef_set[k]));
			m += strlen((char *)gDef_set[k]);
			memcpy(FILE_SECTOR + m, gSet_range[k], strlen((char *)gSet_range[k]));
			m += strlen((char *)gSet_range[k]);
		}
        
		disk_buffer[0x40B] = 0x0;   //attributes
		*(u32*)VOLUME_BASE = VOLUME;
		disk_buffer[0x41A] = 0x02;  //cluster number
		disk_buffer[0x41C] = m;  //file size
		if(rewrite_all_flash_pages() != HAL_OK)
		{
			_Error_Handler(__FILE__, __LINE__);
		}
		
	}   


	return 0;
}
void read_sector(u8* pbuffer, u32 disk_addr)
{	
	disk_addr *= SECTOR_SIZE;
	if (disk_addr == 0x0000)
	{
		_DEBUG("Reading BOOT sector: ", disk_addr);
		memcpy(pbuffer, BOOT_SEC, SECTOR_SIZE);
	}
	else if (disk_addr == 0x1000)
	{
		_DEBUG("Reading FAT1 sector: ", disk_addr);
		memcpy(pbuffer, FAT1_SECTOR, SECTOR_SIZE);
	}
	else if (disk_addr == 0x2800)
	{
		_DEBUG("Reading FAT2 sector: ", disk_addr);
		memcpy(pbuffer, FAT2_SECTOR, SECTOR_SIZE);
	}
	else if (disk_addr == 0x4000)
	{
		_DEBUG("Reading DIR sector: ", disk_addr);
		memcpy(pbuffer, (u8*)(ROOT_SECTOR), SECTOR_SIZE);
	}
	else if (disk_addr >= 0x8000 && disk_addr <= 0xA000)
	{
		_DEBUG("Reading FILE sector: ", disk_addr);
		memcpy(pbuffer, (u8*)(APP_BASE + 0x600 + (disk_addr - 0x8000)), SECTOR_SIZE);
	}
	else {
		_WARN("Unrecognized disk sector read attempt", disk_addr);
		memset(pbuffer, 0, SECTOR_SIZE);
	}
}
u8 write_sector(u8* buff, u32 diskaddr, u32 length)//PC Save data call
{
	u32 i;
	u8 illegal = 0;
	u8 ver[20];
	static u16 Config_flag = 0;
	static u8 txt_flag = 0;
	u8 config_filesize = 0;

	diskaddr *= SECTOR_SIZE;
	length *= SECTOR_SIZE;

	for (i = 0; i < length; i++)
	{
		*(u8 *)(pdisk_buffer_temp + i) = buff[i];
	}

	//todo: deleteme?
	if ((length % SECTOR_SIZE)) 
	{
		return 0;
	}
	if (diskaddr == 0x1000) 
	{
		// Write FAT1 sector
		if(memcmp(pdisk_buffer_temp, (u8*)FAT1_SECTOR, SECTOR_SIZE)) 
		{
			memcpy((u8*)FAT1_SECTOR, pdisk_buffer_temp, SECTOR_SIZE);
		}
	}
	else if (diskaddr == 0x2800) 
	{
		// Write FAT2 sector
	    if(memcmp(pdisk_buffer_temp, (u8*)FAT2_SECTOR, SECTOR_SIZE)) 
		{
			memcpy((u8*)FAT2_SECTOR, pdisk_buffer_temp, SECTOR_SIZE);
		}
	}
	else if (diskaddr == 0x4000) 
	{
		// Write DIR sector
	    if(memcmp(pdisk_buffer_temp, (u8*)ROOT_SECTOR, SECTOR_SIZE)) 
		{
			memcpy((u8*)ROOT_SECTOR, pdisk_buffer_temp, SECTOR_SIZE);
			page_dirty_mask[1] = 1;
			for (i = 0; i < 16; i++) 
			{
				memcpy((u8*)ver, (u8*)(pdisk_buffer_temp), 12);
				if (memcmp(ver, "CONFIG  TXT", 11) == 0) 
				{
					Config_flag = pdisk_buffer_temp[0x1A];
					config_filesize = pdisk_buffer_temp[0x1C];
					txt_flag = 1;
					break;
				}
				pdisk_buffer_temp += 32;
			}
			if (config_filesize == 0 && txt_flag == 1) 
			{
				txt_flag = 0;
				page_dirty_mask[1] = 0;
				page_dirty_mask[0] = 0;
			}
			else 
			{
				page_dirty_mask[0] = 1;
			}
		}
	}
	else if (diskaddr >= 0x8000 && diskaddr <= 0xA000) 
	{
		// Write FILE sector
	    if(memcmp(pdisk_buffer_temp, (u8*)(FILE_SECTOR + (diskaddr - 0x8000)), SECTOR_SIZE)) 
		{
			memcpy((u8*)(FILE_SECTOR + (diskaddr - 0x8000)), pdisk_buffer_temp, SECTOR_SIZE);
		}
        
		if ((((diskaddr - 0x8000) / 0x200) + 2) == Config_flag) 
		{
			//Cluster number
			illegal = validate_file((u8*)(FILE_SECTOR + (diskaddr - 0x8000)), (Config_flag - 2));
			if (!illegal) 
			{
				if (rewrite_dirty_flash_pages() != HAL_OK)
				{
					_Error_Handler(__FILE__, __LINE__);					
				}
			}
			else 
			{
				_Error_Handler(__FILE__, __LINE__);				
			}
		}
		else 
		{
			page_dirty_mask[((diskaddr - 0x8000 + 0x200) / FLASH_PAGE_SIZE) + 1] = 1;
		}
	}
	if (rewrite_dirty_flash_pages() != HAL_OK)
	{		
		_Error_Handler(__FILE__, __LINE__);				
	}
	return HAL_OK;
}
static u32 get_sector_size(void)
{
	return SECTOR_SIZE;
}
static u32 get_sector_count(void)
{
	return SECTOR_CNT;
}
static void init(void)
{
	memcpy(disk_buffer, (u8*)APP_BASE, 0x2600);
	memset(page_dirty_mask, 0, 16);	
	flush_file();
	_DEBUG("Finished initilization", NULL);
}
const struct disk Disk= { 
	.init = init,	
	.Disk_SecWrite = write_sector,
	.Disk_SecRead = read_sector,
	.get_sector_size = get_sector_size,
	.get_sector_count = get_sector_count,
};

