#include "stdafx.h"

#define INRANGE( x, a, b ) ( x >= a && x <= b )
#define getBits( x ) ( INRANGE( ( x & ( ~0x20 ) ), 'A', 'F' ) ? ( ( x & ( ~0x20 ) ) - 'A' + 0xa ) : ( INRANGE( x, '0', '9' ) ? x - '0' : 0 ) )
#define getByte( x ) ( getBits( x[ 0 ] ) << 4 | getBits( x[ 1 ] ) )
static bool ScreenTransform( const Vector& in, Vector& out )
{
	auto& w2sMatrix = g_Interfaces->EngineClient->WorldToScreenMatrix();

	out.x = w2sMatrix.m[0][0] * in.x + w2sMatrix.m[0][1] * in.y + w2sMatrix.m[0][2] * in.z + w2sMatrix.m[0][3];
	out.y = w2sMatrix.m[1][0] * in.x + w2sMatrix.m[1][1] * in.y + w2sMatrix.m[1][2] * in.z + w2sMatrix.m[1][3];
	out.z = 0.0f;

	float w = w2sMatrix.m[3][0] * in.x + w2sMatrix.m[3][1] * in.y + w2sMatrix.m[3][2] * in.z + w2sMatrix.m[3][3];

	if (w < 0.001f) {
		out.x *= 100000;
		out.y *= 100000;
		return false;
	}

	out.x /= w;
	out.y /= w;

	return true;
}

bool Utils::WorldToScreen( const Vector & in, Vector & out )
{
	static std::int32_t w = 0;
	static std::int32_t h = 0;
	if (!w || !h)
		g_Interfaces->EngineClient->GetScreenSize( w, h );

	if (!ScreenTransform( in, out ))
		return false;

	out.x = (w / 2.0f) + (out.x * w) / 2.0f;
	out.y = (h / 2.0f) - (out.y * h) / 2.0f;

	return true;
}

bool Utils::Visible( CBaseEntity * pEntity, std::uint8_t nBone )
{
	Ray_t ray;
	trace_t trace;

	auto pLocal = g_Interfaces->EntityList->GetClientEntity( g_Interfaces->EngineClient->GetLocalPlayer() );

	if (!pLocal)
		return false;

	ray.Init( pLocal->Eyes(), GetHitbox( pEntity, nBone ) );

	CTraceFilter filter;
	filter.pSkip = pLocal;

	g_Interfaces->Trace->TraceRay( ray, MASK_SOLID, &filter, &trace );

	if (trace.m_pEnt == pEntity || trace.fraction == 1.f)
		return true;

	return false;
}

Vector Utils::GetHitbox( CBaseEntity * pEntity, std::int8_t nHitbox )
{
	matrix3x4 matrix[128];

	if (pEntity->SetupBones( matrix, 128, 0x100, 0.f ))
	{
		studiohdr_t* hdr = g_Interfaces->ModelInfo->GetStudioModel( pEntity->GetModel() );
		mstudiohitboxset_t* set = hdr->GetHitboxSet( 0 );

		mstudiobbox_t* hitbox = set->GetHitbox( nHitbox );

		if (hitbox)
		{
			Vector vMin, vMax, vCenter;
			Math::VectorTransform( hitbox->bbmin, matrix[hitbox->bone], vMin );
			Math::VectorTransform( hitbox->bbmax, matrix[hitbox->bone], vMax );

			
			vCenter = ((vMin + vMax) * 0.5f);

			return vCenter;
		}

	}

	return Vector{ 0,0,0 };
}

std::uint8_t Utils::GetHeadHitboxID( const std::uint32_t & nClassID )
{
	switch (nClassID)
	{
	case ClassID::Infected:
		return 15;
		break;

	case ClassID::Tank:
		return 12;
		break;

	case ClassID::Spitter:
		return 4;
		break;

	case ClassID::Jockey:
		return 4;
		break;

	case ClassID::Charger:
		return 9;

	default:
		return 10;
		break;
	}
}

HMODULE Utils::GetModuleHandleSafe(const std::string & pszModuleName)
{
	HMODULE hmModuleHandle = NULL;

	do
	{
		if (pszModuleName.empty())
			hmModuleHandle = GetModuleHandleA(NULL);
		else
			hmModuleHandle = GetModuleHandleA(pszModuleName.c_str());

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	} while (hmModuleHandle == NULL);

	return hmModuleHandle;
}
DWORD Utils::FindPattern(const std::string & szModules, const std::string & szPattern)
{
	HMODULE hmModule = GetModuleHandleSafe(szModules);
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hmModule;
	PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((DWORD)hmModule) + pDOSHeader->e_lfanew);

	DWORD dwAddress = ((DWORD)hmModule) + pNTHeaders->OptionalHeader.BaseOfCode;
	DWORD dwLength = ((DWORD)hmModule) + pNTHeaders->OptionalHeader.SizeOfCode;

	return FindPattern(dwAddress, dwLength, szPattern);
}

DWORD Utils::FindPattern(DWORD dwBegin, DWORD dwEnd, const std::string & szPattern)
{
	const char *pat = szPattern.c_str();
	DWORD firstMatch = NULL;
	for (DWORD pCur = dwBegin; pCur < dwEnd; pCur++)
	{
		if (!*pat)
			return firstMatch;
		if (*(PBYTE)pat == '\?' || *(BYTE *)pCur == getByte(pat))
		{
			if (!firstMatch)
				firstMatch = pCur;
			if (!pat[2])
				return firstMatch;
			if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?')
				pat += 3;
			else
				pat += 2;
		}
		else
		{
			pat = szPattern.c_str();
			firstMatch = 0;
		}
	}

	return NULL;
}

DWORD Utils::FindPattern(const std::string & szModules, const std::string & szPattern, std::string szMask)
{
	HMODULE hmModule = GetModuleHandleSafe(szModules);
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hmModule;
	PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((DWORD)hmModule) + pDOSHeader->e_lfanew);

	DWORD dwAddress = ((DWORD)hmModule) + pNTHeaders->OptionalHeader.BaseOfCode;
	DWORD dwLength = ((DWORD)hmModule) + pNTHeaders->OptionalHeader.SizeOfCode;

	return FindPattern(dwAddress, dwLength, szPattern, szMask);
}

DWORD Utils::FindPattern(DWORD dwBegin, DWORD dwEnd, const std::string & szPattern, std::string szMask)
{
	BYTE First = szPattern[0];
	PBYTE BaseAddress = (PBYTE)(dwBegin);
	PBYTE Max = (PBYTE)(dwEnd - szPattern.length());

	const static auto CompareByteArray = [](PBYTE Data, const char* Signature, const char* Mask) -> bool
	{
		for (; *Signature; ++Signature, ++Data, ++Mask)
		{
			if (*Mask == '?' || *Signature == '\x00' || *Signature == '\x2A')
			{
				continue;
			}
			if (*Data != *Signature)
			{
				return false;
			}
		}
		return true;
	};

	if (szMask.length() < szPattern.length())
	{
		for (auto i = szPattern.begin() + (szPattern.length() - 1); i != szPattern.end(); ++i)
			szMask.push_back(*i == '\x2A' || *i == '\x00' ? '?' : 'x');
	}

	for (; BaseAddress < Max; ++BaseAddress)
	{
		if (*BaseAddress != First)
		{
			continue;
		}
		if (CompareByteArray(BaseAddress, szPattern.c_str(), szMask.c_str()))
		{
			return (DWORD)BaseAddress;
		}
	}

	return NULL;
}