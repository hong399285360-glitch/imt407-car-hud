#ifndef SDLOG_H
#define SDLOG_H

#include "shared_data.h"

void SDLog_Init(void);
void SDLog_Write(VehicleData_t *vd);
void SDLog_Sync(void);
void SDLog_Log(const char *msg);

#endif
