#include "stdafx.h"
#include "checksum.hpp"

std::unique_ptr<VMTHook> g_RenderViewHook;
std::unique_ptr<VMTHook> g_ClientModeHook;
std::unique_ptr<VMTHook> g_PanelHook;

void Hooks::Initialize()
{
	g_RenderViewHook = std::make_unique<VMTHook>( g_Interfaces->RenderView );
	if (g_RenderViewHook->HookFunction( Hooks::SceneEnd, 9 ))
		printf( "Hooked SceneEnd.\n" );

	auto GetClientMode = (void*(__cdecl*)())(reinterpret_cast<std::uintptr_t>(GetModuleHandleA( "client.dll" )) + 0x223670);
	void* ClientMode = GetClientMode();
	g_ClientModeHook = std::make_unique<VMTHook>( ClientMode );
	if (g_ClientModeHook->HookFunction( Hooks::CreateMove, 27 ))
		printf( "Hooked ClientMode::CreateMove.\n" );

	g_PanelHook = std::make_unique<VMTHook>( g_Interfaces->Panel );
	if (g_PanelHook->HookFunction( Hooks::PaintTraverse, 41 ))
		printf( "Hooked PaintTraverse.\n" );

}

void Hooks::Unhook()
{
	g_RenderViewHook->UnhookFunction( 9 );
	g_ClientModeHook->UnhookFunction( 27 );
	g_PanelHook->UnhookFunction( 41 );
	printf( "Uninjected.\n" );
}

void __fastcall Hooks::SceneEnd( void * thisptr, void * edx )
{
	static auto oSceneEnd = g_RenderViewHook->GetOriginalFunction<FnSceneEnd>( 9 );
	oSceneEnd( edx );

	if (!g_Config.m_bChams)
		return;

	auto pLocal = g_Interfaces->EntityList->GetClientEntity( g_Interfaces->EngineClient->GetLocalPlayer() );
	if (!pLocal)
		return;

	static IMaterial* pMaterial = nullptr;
	if (!pMaterial)
	{
		pMaterial = g_Interfaces->MaterialSystem->FindMaterial( "debug/debugambientcube", "Model textures" );
		pMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, true );
		pMaterial->ColorModuleate( 1.f, 1.f, 1.f );
	}

	for (std::uint32_t nIndex = 1; nIndex < g_Interfaces->EntityList->GetHighestEntityIndex(); nIndex++)
	{
		auto pEntity = g_Interfaces->EntityList->GetClientEntity( nIndex );
		if (!pEntity || pEntity == pLocal || !pEntity->ValidEntity())
			continue;

		g_Interfaces->ModelRenderer->ForcedMaterialOverride( pMaterial );
		pEntity->DrawModel( 0x1 );
		g_Interfaces->ModelRenderer->ForcedMaterialOverride( nullptr );

	}
}

void SinCos(float a, float* s, float*c)
{
	*s = sin(a);
	*c = cos(a);
}
void AngleVectors(const Vector &angles, Vector& forward, Vector& right, Vector& up)
{
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles[1]), &sy, &cy);
	SinCos(DEG2RAD(angles[0]), &sp, &cp);
	SinCos(DEG2RAD(angles[2]), &sr, &cr);

	forward.x = (cp * cy);
	forward.y = (cp * sy);
	forward.z = (-sp);
	right.x = (-1 * sr * sp * cy + -1 * cr * -sy);
	right.y = (-1 * sr * sp * sy + -1 * cr *  cy);
	right.z = (-1 * sr * cp);
	up.x = (cr * sp * cy + -sr * -sy);
	up.y = (cr * sp * sy + -sr * cy);
	up.z = (cr * cp);
}
void FixMovement(CUserCmd *usercmd, Vector& wish_angle)
{
	Vector view_fwd, view_right, view_up, cmd_fwd, cmd_right, cmd_up;
	Vector viewangles = usercmd->viewangles;
	auto Norm = [&](Vector in)
	{
		auto x_rev = in.x / 360.f;
		if (in.x > 180.f || in.x < -180.f)
		{
			x_rev = abs(x_rev);
			x_rev = round(x_rev);

			if (in.x < 0.f)
				in.x = (in.x + 360.f * x_rev);

			else
				in.x = (in.x - 360.f * x_rev);
		}

		auto y_rev = in.y / 360.f;
		if (in.y > 180.f || in.y < -180.f)
		{
			y_rev = abs(y_rev);
			y_rev = round(y_rev);

			if (in.y < 0.f)
				in.y = (in.y + 360.f * y_rev);

			else
				in.y = (in.y - 360.f * y_rev);
		}

		auto z_rev = in.z / 360.f;
		if (in.z > 180.f || in.z < -180.f)
		{
			z_rev = abs(z_rev);
			z_rev = round(z_rev);

			if (in.z < 0.f)
				in.z = (in.z + 360.f * z_rev);

			else
				in.z = (in.z - 360.f * z_rev);
		}

		return Vector(in.x, in.y, in.z);
	};

	Norm(viewangles);

	AngleVectors(wish_angle, view_fwd, view_right, view_up);
	AngleVectors(usercmd->viewangles, cmd_fwd, cmd_right, cmd_up);

	const float v8 = sqrtf((view_fwd.x * view_fwd.x) + (view_fwd.y * view_fwd.y));
	const float v10 = sqrtf((view_right.x * view_right.x) + (view_right.y * view_right.y));
	const float v12 = sqrtf(view_up.z * view_up.z);

	const Vector norm_view_fwd((1.f / v8) * view_fwd.x, (1.f / v8) * view_fwd.y, 0.f);
	const Vector norm_view_right((1.f / v10) * view_right.x, (1.f / v10) * view_right.y, 0.f);
	const Vector norm_view_up(0.f, 0.f, (1.f / v12) * view_up.z);

	const float v14 = sqrtf((cmd_fwd.x * cmd_fwd.x) + (cmd_fwd.y * cmd_fwd.y));
	const float v16 = sqrtf((cmd_right.x * cmd_right.x) + (cmd_right.y * cmd_right.y));
	const float v18 = sqrtf(cmd_up.z * cmd_up.z);

	const Vector norm_cmd_fwd((1.f / v14) * cmd_fwd.x, (1.f / v14) * cmd_fwd.y, 0.f);
	const Vector norm_cmd_right((1.f / v16) * cmd_right.x, (1.f / v16) * cmd_right.y, 0.f);
	const Vector norm_cmd_up(0.f, 0.f, (1.f / v18) * cmd_up.z);

	const float v22 = norm_view_fwd.x * usercmd->forwardmove;
	const float v26 = norm_view_fwd.y * usercmd->forwardmove;
	const float v28 = norm_view_fwd.z * usercmd->forwardmove;
	const float v24 = norm_view_right.x * usercmd->sidemove;
	const float v23 = norm_view_right.y * usercmd->sidemove;
	const float v25 = norm_view_right.z * usercmd->sidemove;
	const float v30 = norm_view_up.x * usercmd->upmove;
	const float v27 = norm_view_up.z * usercmd->upmove;
	const float v29 = norm_view_up.y * usercmd->upmove;

	usercmd->forwardmove = ((((norm_cmd_fwd.x * v24) + (norm_cmd_fwd.y * v23)) + (norm_cmd_fwd.z * v25))
		+ (((norm_cmd_fwd.x * v22) + (norm_cmd_fwd.y * v26)) + (norm_cmd_fwd.z * v28)))
		+ (((norm_cmd_fwd.y * v30) + (norm_cmd_fwd.x * v29)) + (norm_cmd_fwd.z * v27));
	usercmd->sidemove = ((((norm_cmd_right.x * v24) + (norm_cmd_right.y * v23)) + (norm_cmd_right.z * v25))
		+ (((norm_cmd_right.x * v22) + (norm_cmd_right.y * v26)) + (norm_cmd_right.z * v28)))
		+ (((norm_cmd_right.x * v29) + (norm_cmd_right.y * v30)) + (norm_cmd_right.z * v27));
	usercmd->upmove = ((((norm_cmd_up.x * v23) + (norm_cmd_up.y * v24)) + (norm_cmd_up.z * v25))
		+ (((norm_cmd_up.x * v26) + (norm_cmd_up.y * v22)) + (norm_cmd_up.z * v28)))
		+ (((norm_cmd_up.x * v30) + (norm_cmd_up.y * v29)) + (norm_cmd_up.z * v27));
}
bool __stdcall Hooks::CreateMove( float flInputSampleTime, CUserCmd * pCmd )
{
	static auto g_Aimbot = std::make_unique<Aimbot::CAimbot>();
	static auto g_Misc = std::make_unique<MiscFeatures::CMiscFeatures>();
	Vector angle = pCmd->viewangles;
	if (!pCmd->command_number)
		return false;

	auto pLocal = g_Interfaces->EntityList->GetClientEntity( g_Interfaces->EngineClient->GetLocalPlayer() );
	if (!pLocal)
		return false;

	pCmd->random_seed = MD5_PseudoRandom( pCmd->command_number ) & 0x7fffffff;

	g_Misc->Execute( pCmd );

	EnginePrediction::CEnginePrediction EnginePred = EnginePrediction::CEnginePrediction( pCmd, pLocal );
	g_Aimbot->Execute( pCmd );
	FixMovement(pCmd, angle);
	return !(g_Config.m_nAimMode == Config::AIMBOT_MODE::AIMBOT_SILENT);
}

void __stdcall Hooks::PaintTraverse( std::uint32_t nPanel, bool bForceRepaint, bool bAllowForce )
{
	static FnPaintTraverse oPaintTraverse = g_PanelHook->GetOriginalFunction<FnPaintTraverse>( 41 );
	oPaintTraverse( g_Interfaces->Panel, nPanel, bForceRepaint, bAllowForce );

	static auto g_ESP = std::make_unique<ESP::CESP>();
	static auto g_Menu = std::make_unique<Menu::CMenu>();

	static std::uint32_t nDrawPanel = 0;
	if (!nDrawPanel)
	{
		if (!strcmp( g_Interfaces->Panel->GetName( nPanel ), "FocusOverlayPanel" ))
			nDrawPanel = nPanel;
		else
			return;
	}

	if (nPanel != nDrawPanel)
		return;

	if (!g_Interfaces->EngineClient->IsInGame() || !g_Interfaces->EngineClient->IsConnected())
		return;

	g_ESP->Execute();
	g_Menu->Execute();
}
