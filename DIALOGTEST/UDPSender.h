#pragma once

#include "common.h"

int InitUDPSender(unsigned long ulIP, int iPort);
int SendUDP(T6DOF *headpose);
int CloseUDPSender();
