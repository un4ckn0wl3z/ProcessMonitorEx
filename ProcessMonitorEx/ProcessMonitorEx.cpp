#include "pch.h"
#include "ProcessMonitorEx.h"

ProcessMonitorExState g_State;

VOID OnProcessCallback(
	_Inout_ PEPROCESS Process,
	_In_ HANDLE ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
	);

void ProcessMonitorExUnload(PDRIVER_OBJECT DriverObject);

extern "C" 
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	NTSTATUS status;
	PDEVICE_OBJECT devObj = nullptr;
	bool symLinkCreated = false;
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\ProcessMonitorEx");
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\ProcessMonitorEx");

	do
	{
		status = IoCreateDevice(
			DriverObject, 
			0, 
			&devName, 
			FILE_DEVICE_UNKNOWN, 
			0, 
			TRUE, 
			&devObj);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		status = IoCreateSymbolicLink(&symName, &devName);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		symLinkCreated = true;

		status = PsSetCreateProcessNotifyRoutineEx(OnProcessCallback, FALSE);
		if (!NT_SUCCESS(status))
		{
			break;
		}

	} while (false);

	if (!NT_SUCCESS(status))
	{
		KdPrint((DRIVER_PREFIX "ERROR in DriverEntry (0x%X)\n", status));
		if (devObj)
		{
			IoDeleteDevice(devObj);
		}
		if (symLinkCreated)
		{
			IoDeleteSymbolicLink(&symName);
		}
		return status;
	}

	g_State.Lock.Init();
	InitializeListHead(&g_State.ItemsHead);

	DriverObject->DriverUnload = ProcessMonitorExUnload;

	return status;
}

VOID OnProcessCallback(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);
	if (CreateInfo)
	{
		KdPrint((DRIVER_PREFIX "Process (%u) Created\n", HandleToUlong(ProcessId)));
	}
	else
	{
		KdPrint((DRIVER_PREFIX "Process (%u) Exited\n", HandleToUlong(ProcessId)));
	}

	return VOID();
}

void ProcessMonitorExUnload(PDRIVER_OBJECT DriverObject)
{
	PsSetCreateProcessNotifyRoutineEx(OnProcessCallback, TRUE);
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\ProcessMonitorEx");
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(DriverObject->DeviceObject);

}
