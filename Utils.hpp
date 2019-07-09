#pragma once

namespace Utils
{
	bool WorldToScreen( const Vector& in, Vector& out );
	bool Visible( CBaseEntity* pEntity, std::uint8_t nBone );
	Vector GetHitbox( CBaseEntity* pEntity, std::int8_t nHitbox );
	std::uint8_t GetHeadHitboxID( const std::uint32_t& nClassID );
	HMODULE GetModuleHandleSafe(const std::string & pszModuleName);
	DWORD FindPattern(const std::string& szModules, const std::string& szPattern);
	DWORD FindPattern(DWORD dwBegin, DWORD dwEnd, const std::string& szPattern);
	DWORD FindPattern(const std::string & szModules, const std::string & szPattern, std::string szMask);
	DWORD FindPattern(DWORD dwBegin, DWORD dwEnd, const std::string & szPattern, std::string szMask);
}