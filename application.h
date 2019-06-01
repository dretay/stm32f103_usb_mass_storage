#pragma once
#include "disk.h"

struct application {
	void(*init)(void);			
};

extern const struct application Application;