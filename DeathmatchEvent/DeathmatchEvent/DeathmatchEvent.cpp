#pragma once
#include "DeathmatchEvent.h"
#include "../../EventManager/EventManager/Public/EventManager.h"
#pragma comment(lib, "AEventManager.lib")
#pragma comment(lib, "ArkApi.lib")
#include <fstream>
#include "json.hpp"

class DeathMatch : Event
{
public:
	virtual void Init(const FString& Map)
	{
		if (!GetSpawnsSet())
		{
			InitDefaults(L"DeathMatch", L"Epidemic", 2, true, FVector(0, 0, 0), 60000);
			/*const std::string config_path = ArkApi::Tools::GetCurrentDir() + "/ArkApi/Plugins/DeathMatchEvent/" + Map.ToString() + ".json";
			std::ifstream file { config_path };
			if (!file.is_open()) throw std::runtime_error(fmt::format("Can't open {}.json", Map.ToString().c_str()).c_str());
			nlohmann::json config;
			file >> config;
			auto DMSpawns = config["Deathmatch"]["Spawns"];
			for (const auto& PermColours : DMSpawns)
			{
				config = PermColours["Spawn"];
				AddSpawn(FVector(config[0], config[1], config[2]));
			}
			file.close();*/
		}
		Reset();
		AddTime(60);
	}

	virtual void Update()
	{
		switch (GetState())
		{
		case EventState::WaitingForPlayers:
			if (TimePassed())
			{
				if (UpCount() == 4)
				{
					ResetCount();
					if (EventManager::GetEventManager().GetEventPlayerCount() < 2)
					{
						EventManager::GetEventManager().SendChatMessageToAllEventPlayers(GetServerName(), L"[Event] {} Failed to start needed 2 Players", *GetName());
						SetState(EventState::Finnished);
					}
					else SetState(EventState::TeleportingPlayers);
				}
				AddTime(60);
			}
			break;
		case EventState::TeleportingPlayers:
			EventManager::TeleportEventPlayers(false, true, true, GetSpawns(), GetSpawns(EventTeam::Blue));
			EventManager::GetEventManager().SendChatMessageToAllEventPlayers(GetServerName(), L"[Event] {} Starting in 30 Seconds", *GetName());
			AddTime(30);
			SetState(EventState::WaitForFight);
			break;
		case EventState::WaitForFight:
			if (TimePassed())
			{
				EventManager::GetEventManager().SendChatMessageToAllEventPlayers(GetServerName(), L"[Event] {} Started Kill or Be Killed!", *GetName());
				SetState(EventState::Fighting);
			}
			break;
		case EventState::Fighting:
			break;
		case EventState::Rewarding:
			break;
		}
	}
} DMEvent;

void RemoveEvent()
{
	EventManager::AddEvent((Event&)DMEvent);
}

void InitEvent()
{
	EventManager::RemoveEvent((Event&)DMEvent);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitEvent();
		break;
	case DLL_PROCESS_DETACH:
		RemoveEvent();
		break;
	}
	return TRUE;
}