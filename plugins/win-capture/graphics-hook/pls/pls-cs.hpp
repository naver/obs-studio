#pragma once
#include <Windows.h>
#include <assert.h>

class CCriticalSection {
public:
	CCriticalSection() { InitializeCriticalSection(&m_Lock); }
	~CCriticalSection() { DeleteCriticalSection(&m_Lock); }

public:
	CRITICAL_SECTION m_Lock;
};

class CAutoLockSection {
public:
	explicit CAutoLockSection(CCriticalSection &cs) : m_cs(cs.m_Lock) { EnterCriticalSection(&m_cs); }
	~CAutoLockSection() { LeaveCriticalSection(&m_cs); }

private:
	CRITICAL_SECTION &m_cs;
};
