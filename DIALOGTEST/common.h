#pragma once

struct POSITION
{
	double x;
	double y;
	double z;
	double yaw;
	double pitch;
	double roll;
};

struct T6DOF
{
	POSITION position;
};
