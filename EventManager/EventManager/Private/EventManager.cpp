#include "..\Public\EventManager.h"
#include "EventMan.h"
#pragma comment(lib, "ArkApi.lib")
DECLARE_HOOK(APrimalStructure_IsAllowedToBuild, int, APrimalStructure*, APlayerController*, FVector, FRotator, FPlacementData*, bool, FRotator, bool);
DECLARE_HOOK(APrimalCharacter_TakeDamage, float, APrimalCharacter*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(AShooterCharacter_Die, bool, AShooterCharacter*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);

int SecCounter = 0;
void EventManagerUpdate()
{
	if (SecCounter++ == 2)
	{
		if (!ArkApi::GetApiUtils().GetWorld())
		{
			SecCounter = 0;
			return;
		}
		EventMan::Get().Update();
		SecCounter = 0;
	}
}

int _cdecl Hook_APrimalStructure_IsAllowedToBuild(APrimalStructure* _this, APlayerController* PC, FVector AtLocation, FRotator AtRotation, FPlacementData * OutPlacementData, bool bDontAdjustForMaxRange, FRotator PlayerViewRotation, bool bFinalPlacement)
{
	return APrimalStructure_IsAllowedToBuild_original(_this, PC, AtLocation, AtRotation, OutPlacementData, bDontAdjustForMaxRange, PlayerViewRotation, bFinalPlacement);
}

const long long GetPlayerID(APrimalCharacter* _this)
{
	AShooterCharacter* shooterCharacter = static_cast<AShooterCharacter*>(_this);
	return (shooterCharacter && shooterCharacter->GetPlayerData()) ? shooterCharacter->GetPlayerData()->MyDataField()()->PlayerDataIDField()() : -1;
}

const long long GetPlayerID(AController* _this)
{
	AShooterPlayerController* Player = static_cast<AShooterPlayerController*>(_this);
	return Player ? Player->LinkedPlayerIDField()() : 0;
}

float _cdecl Hook_APrimalCharacter_TakeDamage(APrimalCharacter* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (EventMan::Get().IsEventRunning() && EventInstigator && _this && !EventInstigator->IsLocalController() && _this->IsA(AShooterCharacter::GetPrivateStaticClass()))
		return EventMan::Get().CanTakeDamage(GetPlayerID(EventInstigator), GetPlayerID(_this)) ? APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser) : 0;
	return APrimalCharacter_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
}

bool _cdecl Hook_AShooterCharacter_Die(AShooterCharacter* _this, float KillingDamage, FDamageEvent* DamageEvent, AController* Killer, AActor* DamageCauser)
{
	if (EventMan::Get().IsEventRunning() && Killer && _this && !Killer->IsLocalController() && Killer->IsA(AShooterCharacter::GetPrivateStaticClass())) EventMan::Get().OnPlayerDied(GetPlayerID(Killer), GetPlayerID(_this));
	return AShooterCharacter_Die_original(_this, KillingDamage, DamageEvent, Killer, DamageCauser);
}

void _cdecl Hook_AShooterGameMode_Logout(AShooterGameMode* _this, AController* Exiting)
{
	if (EventMan::Get().IsEventRunning() && Exiting && Exiting->IsA(AShooterPlayerController::StaticClass()))
	{
		AShooterPlayerController* Player = static_cast<AShooterPlayerController*>(Exiting);
		if (Player)	EventMan::Get().OnPlayerLogg(Player);
	}
	AShooterGameMode_Logout_original(_this, Exiting);
}

float _cdecl Hook_APrimalStructure_TakeDamage(APrimalStructure* _this, float Damage, FDamageEvent* DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	return EventMan::Get().IsEventProtectedStructure(_this->RootComponentField()() ? _this->RootComponentField()()->RelativeLocationField()() : FVector(0, 0, 0)) ? 0 : APrimalStructure_TakeDamage_original(_this, Damage, DamageEvent, EventInstigator, DamageCauser);
}

void InitEventManager()
{
	EventMan::Get().Init();
	ArkApi::GetHooks().SetHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild, reinterpret_cast<LPVOID*>(&APrimalStructure_IsAllowedToBuild_original));
	ArkApi::GetHooks().SetHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage, reinterpret_cast<LPVOID*>(&APrimalCharacter_TakeDamage_original));
	ArkApi::GetHooks().SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage, reinterpret_cast<LPVOID*>(&APrimalStructure_TakeDamage_original));
	ArkApi::GetHooks().SetHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die, reinterpret_cast<LPVOID*>(&AShooterCharacter_Die_original));
	ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, reinterpret_cast<LPVOID*>(&AShooterGameMode_Logout_original));
	ArkApi::GetCommands().AddOnTimerCallback("EventManagerUpdate", &EventManagerUpdate);
}

void DestroyEventManager()
{
	ArkApi::GetHooks().DisableHook("APrimalStructure.IsAllowedToBuild", &Hook_APrimalStructure_IsAllowedToBuild);
	ArkApi::GetHooks().DisableHook("APrimalCharacter.TakeDamage", &Hook_APrimalCharacter_TakeDamage);
	ArkApi::GetHooks().DisableHook("APrimalTargetableActor.TakeDamage", &Hook_APrimalStructure_TakeDamage);
	ArkApi::GetHooks().DisableHook("AShooterCharacter.Die", &Hook_AShooterCharacter_Die);
	ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
	ArkApi::GetCommands().RemoveOnTimerCallback("EventManagerUpdate");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitEventManager();
		break;
	case DLL_PROCESS_DETACH:
		DestroyEventManager();
		break;
	}
	return TRUE;
}

namespace EventManager
{
	EventMan& GetEventManager()
	{ 
		return EventMan::Get(); 
	}

	void AddEvent(Event event)
	{
		EventMan::Get().AddEvent(event);
	}

	void RemoveEvent(Event event)
	{
		EventMan::Get().RemoveEvent(event);
	}
	
	bool StartEvent(const int EventID)
	{
		return EventMan::Get().StartEvent(EventID);
	}
	
	void TeleportEventPlayers(const bool TeamBased, const bool WipeInventory, const bool PreventDinos, const TArray<FVector> TeamA, const TArray<FVector> TeamB)
	{
		EventMan::Get().TeleportEventPlayers(TeamBased, WipeInventory, PreventDinos, TeamA, TeamB);
	}
};