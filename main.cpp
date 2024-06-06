#include "function.h"
#include "overlay.h"
#include "driver.h"
#include "xorstr.h"

#include "lazy.h"

#include "skStr.h"
#include <windows.h>
#include "protection.h"

#include "custom_elements.h"
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "Version.lib") 

typedef LONG NTSTATUS;
extern "C" NTSTATUS WINAPI RtlGetVersion(LPOSVERSIONINFOEXW lpVersionInformation);

std::string RandomStrings(int len)
{
	srand(time(NULL));
	std::string str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	std::string newstr;
	int pos;
	while (newstr.size() != len) {
		pos = ((rand() % (str.size() - 1)));
		newstr += str.substr(pos, 1);
	}
	return newstr;
}

bool RenameFile(std::string& path)
{
	std::string newPath = (RandomStrings(16) + ".exe");
	if (std::rename(path.c_str(), newPath.c_str()))
		return false;
	path = newPath;
	return true;
}


std::uint32_t find_dbg(const char* proc)
{
	auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	auto pe = PROCESSENTRY32{ sizeof(PROCESSENTRY32) };

	if (Process32First(snapshot, &pe)) {
		do {
			if (!_stricmp(proc, pe.szExeFile)) {
				CloseHandle(snapshot);
				return pe.th32ProcessID;
			}
		} while (Process32Next(snapshot, &pe));
	}
	CloseHandle(snapshot);
	return 0;
}

std::string replaceAll(std::string subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
	return subject;
}

namespace OverlayWindow
{
	WNDCLASSEX WindowClass;
	HWND Hwnd;
	LPCSTR Name;
}

void PrintPtr(std::string text, uintptr_t ptr) {
	std::cout << text << ptr << std::endl;
}


enum bones
{
	Root = 0,
	pelvis = 3,
	upperarm_l = 93,
	hand_l = 118,
	neck_01 = 6,
	head = 7,
	upperarm_r = 66,
	hand_r = 91,
	thigh_l = 125,
	calf_l = 126,
	foot_l = 139,
	thigh_r = 130,
	calf_r = 131,
	foot_r = 140,
	ik_hand_l = 116,
	ik_hand_r = 89,
};

namespace DirectX9Interface
{
	IDirect3D9Ex* Direct3D9 = NULL;
	IDirect3DDevice9Ex* pDevice = NULL;
	D3DPRESENT_PARAMETERS pParams = { NULL };
	MARGINS Margin = { -1 };
	MSG Message = { NULL };
}
typedef struct _EntityList
{
	uintptr_t actor_pawn;
	uintptr_t actor_mesh;
	uintptr_t actor_state;
	Vector3 actor_pos;
	int actor_id;
	string actor_name;

	string bot_name;
	Vector3 bot_pos;
	int bot_id;

	uintptr_t item_pawn;
	Vector3 item_pos;
	int item_id;
	string item_name;

	string Ships_name;
	Vector3 Ships_pos;
	int Ships_id;
}EntityList;
std::vector<EntityList> entityAllList;
std::vector<EntityList> entityList;
std::vector<EntityList> entityBotList;
std::vector<EntityList> entityShipsList;

auto CallAimbot() -> VOID
{
	while (true)
	{
		auto EntityList_Copy = entityList;

		bool isAimbotActive = CFG.b_Aimbot && GetAimKey();
		if (isAimbotActive)
		{
			float target_dist = FLT_MAX;
			EntityList target_entity = {};

			for (int index = 0; index < EntityList_Copy.size(); ++index)
			{
				auto Entity = EntityList_Copy[index];

				auto local_pos = read<Vector3>(GameVars.local_player_root + GameOffset.offset_relative_location);
				auto bone_pos = GetBoneWithRotation(Entity.actor_mesh, 0);
				auto entity_distance = local_pos.Distance(bone_pos);

				auto TeamId = read<char>(Entity.actor_state + GameOffset.offset_teamid);
				auto TeamLocal = read<char>(GameVars.local_player_state + GameOffset.offset_teamid);

				auto Health = read<float>(Entity.actor_pawn + GameOffset.offset_health);

				if (Health <= 0)
					continue;

				if (TeamId == TeamLocal)
					continue;

				if (!Entity.actor_mesh)
					continue;

				if (entity_distance < CFG.max_distanceAIM)
				{
					int boneSelect;
					if (CFG.boneType == 0)
					{
						boneSelect = 7;
					}
					else if (CFG.boneType == 2)
					{
						boneSelect = 4;
					}

					auto head_pos = GetBoneWithRotation(Entity.actor_mesh, boneSelect);
					auto targethead = ProjectWorldToScreen(Vector3(head_pos.x, head_pos.y, head_pos.z));

					float x = targethead.x - GameVars.ScreenWidth / 2.0f;
					float y = targethead.y - GameVars.ScreenHeight / 2.0f;
					float crosshair_dist = sqrtf((x * x) + (y * y));

					if (crosshair_dist <= FLT_MAX && crosshair_dist <= target_dist)
					{
						if (crosshair_dist > CFG.AimbotFOV) // FOV
							continue;

						target_dist = crosshair_dist;
						target_entity = Entity;

					}
				}


			}

			if (target_entity.actor_mesh != 0 || target_entity.actor_pawn != 0 || target_entity.actor_id != 0)
			{
				int boneSelect;
				if (CFG.boneType == 0)
				{
					boneSelect = 7;
				}
				else if (CFG.boneType == 2)
				{
					boneSelect = 4;
				}

				if (target_entity.actor_pawn == GameVars.local_player_pawn)
					continue;

				if (!isVisible(target_entity.actor_mesh))
					continue;

				auto head_pos = GetBoneWithRotation(target_entity.actor_mesh, boneSelect);
				auto targethead = ProjectWorldToScreen(Vector3(head_pos.x, head_pos.y, head_pos.z));
				move_to(targethead.x, targethead.y);
			}
		}
		//Sleep(10);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

auto GameCache()->VOID
{
	while (true)
	{
		std::vector<EntityList> tmpList;
		//std::vector<EntityList> entityBot;

		GameVars.u_world = read<DWORD_PTR>(GameVars.dwProcess_Base + GameOffset.offset_u_world);
		GameVars.game_instance = read<DWORD_PTR>(GameVars.u_world + GameOffset.offset_game_instance);
		GameVars.local_player_array = read<DWORD_PTR>(GameVars.game_instance + GameOffset.offset_local_players_array);
		GameVars.local_player = read<DWORD_PTR>(GameVars.local_player_array);
		GameVars.local_player_controller = read<DWORD_PTR>(GameVars.local_player + GameOffset.offset_player_controller);
		GameVars.local_player_pawn = read<DWORD_PTR>(GameVars.local_player_controller + GameOffset.offset_apawn);
		GameVars.local_player_root = read<DWORD_PTR>(GameVars.local_player_pawn + GameOffset.offset_root_component);
		GameVars.local_player_state = read<DWORD_PTR>(GameVars.local_player_pawn + GameOffset.offset_player_state);
		GameVars.persistent_level = read<DWORD_PTR>(GameVars.u_world + GameOffset.offset_persistent_level);
		GameVars.actors = read<DWORD_PTR>(GameVars.persistent_level + GameOffset.offset_actor_array);
		GameVars.actor_count = read<int>(GameVars.persistent_level + GameOffset.offset_actor_count);
		
		//PrintPtr("u_world ", GameVars.u_world);
		//PrintPtr("game instance ", GameVars.game_instance);
		//PrintPtr("L Player Array ", GameVars.local_player_array);
		//PrintPtr("L Player ", GameVars.local_player);
		//PrintPtr("L Player Controller ", GameVars.local_player_controller);
		//PrintPtr("L Player Pawn ", GameVars.local_player_pawn);
		//PrintPtr("L Player Root ", GameVars.local_player_root);
		//PrintPtr("L Player State ", GameVars.local_player_state);
		//PrintPtr("P Level ", GameVars.persistent_level);
		//PrintPtr("Actors ", GameVars.actors);
		//PrintPtr("Actor Count ", GameVars.actor_count);
		
		for (int index = 0; index < GameVars.actor_count; ++index)
		{

			auto actor_pawn = read<uintptr_t>(GameVars.actors + index * 0x8);
			if (actor_pawn == 0x00)
				continue;

			//if (actor_pawn == GameVars.local_player_pawn)
			//	continue;

			auto actor_id = read<int>(actor_pawn + GameOffset.offset_actor_id);
			auto actor_mesh = read<uintptr_t>(actor_pawn + GameOffset.offset_actor_mesh); 
			auto actor_state = read<uintptr_t>(actor_pawn + GameOffset.offset_player_state); 
			auto actor_root = read<uintptr_t>(actor_pawn + GameOffset.offset_root_component);
			if (!actor_root) continue;
			auto actor_pos = read<Vector3>(actor_root + GameOffset.offset_relative_location);
			if (actor_pos.x == 0 || actor_pos.y == 0 || actor_pos.z == 0) continue;

			auto name = GetNameFromFName(actor_id);

			//printf("\n: %s", name.c_str());

			auto local_pos = read<Vector3>(GameVars.local_player_root + GameOffset.offset_relative_location);
			auto entity_distance = local_pos.Distance(actor_pos);

			//if (entity_distance < CFG.itemdistance)
			//{
			//	EntityList Entity{ };
			//	Entity.actor_pawn = actor_pawn;
			//	Entity.actor_id = actor_id;
			//	Entity.actor_state = actor_state;
			//	Entity.actor_mesh = actor_mesh;
			//	Entity.actor_pos = actor_pos;
			//	Entity.actor_name = name;
			//	tmpList.push_back(Entity);
			//}
			
			if (name.find(xorstr("BP_Soldier")) != std::string::npos)
			{
				if (actor_pawn != NULL || actor_id != NULL || actor_state != NULL || actor_mesh != NULL)
				{
					EntityList Entity{ };
					Entity.actor_pawn = actor_pawn;
					Entity.actor_id = actor_id;
					Entity.actor_state = actor_state;
					Entity.actor_mesh = actor_mesh;
					Entity.actor_pos = actor_pos;
					Entity.actor_name = name;
					tmpList.push_back(Entity);
				}
			}
			else
				continue;
		}
		entityList = tmpList;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}


auto RenderVisual() -> VOID
{
	auto EntityList_Copy = entityList;

	for (int index = 0; index < EntityList_Copy.size(); ++index)
	{
		auto Entity = EntityList_Copy[index];

		if (!Entity.actor_mesh || !Entity.actor_pawn)
			continue;

		auto local_pos = read<Vector3>(GameVars.local_player_root + GameOffset.offset_relative_location);
		auto head_pos = GetBoneWithRotation(Entity.actor_mesh, bones::head);
		auto bone_pos = GetBoneWithRotation(Entity.actor_mesh, 0);

		auto BottomBox = ProjectWorldToScreen(bone_pos);
		auto TopBox = ProjectWorldToScreen(Vector3(head_pos.x, head_pos.y, head_pos.z + 15));

		auto entity_distance = local_pos.Distance(bone_pos);
		int dist = entity_distance;

		auto CornerHeight = abs(TopBox.y - BottomBox.y);
		auto CornerWidth = CornerHeight * 0.65;

		auto bVisible = isVisible(Entity.actor_mesh);
		auto ESP_Color = GetVisibleColor(bVisible);

		auto PlayerName = read<FString>(Entity.actor_state + GameOffset.offset_player_name);
		

		auto Health = read<float>(Entity.actor_pawn + GameOffset.offset_health);

		int healthValue = max(0, min(Health, 100));

		auto barHealth = read<float>(GameVars.local_player_pawn + GameOffset.offset_health);

		int barhealthValue = max(0, min(barHealth, 100));

		ImColor guibarColor = ImColor(
			min(510 * (100 - barhealthValue) / 100, 255),
			min(510 * barhealthValue / 100, 255),
			25,
			255
		);

		ImColor barColor = ImColor(
			min(510 * (100 - healthValue) / 100, 255),
			min(510 * healthValue / 100, 255),
			25,
			255
		);
		
		//auto isDead = read<bool>(Entity.actor_state + 0x2e4); //bIsDying : 1

		auto TeamId = read<char>(Entity.actor_state + GameOffset.offset_teamid);
		auto TeamLocal = read<char>(GameVars.local_player_state + GameOffset.offset_teamid);

		if(CFG.guihp)
		{
			DrawOutlinedText(Verdana, xorstr("HP: ") + std::to_string(barhealthValue), ImVec2(GameVars.ScreenLeft + 300, GameVars.ScreenBottom - 40), 20.0f, guibarColor, true);
		}

		if(CFG.ignoreteam)
		{	
			if (TeamId == TeamLocal)
				continue;
		}

		if (TeamId == 0)
			continue;

		if(Health <= 0)
			continue;

		//if (isDead)
		//	continue;

		if (CFG.b_Aimbot)
		{
			if (CFG.b_AimbotFOV)
			{
				DrawCircle(GameVars.ScreenWidth / 2, GameVars.ScreenHeight / 2, CFG.AimbotFOV, CFG.FovColor, 0);
			}
		}

		if (CFG.b_Visual)
		{
			if (entity_distance < CFG.max_distance)
			{
				if (CFG.b_EspBox)
				{
					if (CFG.BoxType == 0)
					{
						DrawBox(TopBox.x - (CornerWidth / 2), TopBox.y, CornerWidth, CornerHeight, ESP_Color);
					}
					else if (CFG.BoxType == 1)
					{
						DrawCorneredBox(TopBox.x - (CornerWidth / 2), TopBox.y, CornerWidth, CornerHeight, ESP_Color, 1.5);
					}
				}
				if (CFG.b_EspHealthHP)
				{
					DrawOutlinedText(Verdana, xorstr("HP: ") + std::to_string(healthValue), ImVec2(TopBox.x, TopBox.y - 15), 16.0f, barColor, true);
				}
				if (CFG.b_EspHealth)
				{
					float width = CornerWidth / 10;
					if (width < 2.f) width = 2.;
					if (width > 3) width = 3.;

					HealthBar(TopBox.x - (CornerWidth / 2) - 8, TopBox.y, width, BottomBox.y - TopBox.y, healthValue, false);

				}
				if (CFG.b_EspLine)
				{

					if (CFG.LineType == 0)
					{
						DrawLine(ImVec2(static_cast<float>(GameVars.ScreenWidth / 2), static_cast<float>(GameVars.ScreenHeight)), ImVec2(BottomBox.x, BottomBox.y), ESP_Color, 1.5f); //LINE FROM CROSSHAIR
					}
					if (CFG.LineType == 1)
					{
						DrawLine(ImVec2(static_cast<float>(GameVars.ScreenWidth / 2), 0.f), ImVec2(BottomBox.x, BottomBox.y), ESP_Color, 1.5f); //LINE FROM CROSSHAIR
					}
					if (CFG.LineType == 2)
					{
						DrawLine(ImVec2(static_cast<float>(GameVars.ScreenWidth / 2), static_cast<float>(GameVars.ScreenHeight / 2)), ImVec2(BottomBox.x, BottomBox.y), ESP_Color, 1.5f); //LINE FROM CROSSHAIR
					}
				}
				if (CFG.b_EspName)
				{
					DrawOutlinedText(Verdana, PlayerName.ToString(), ImVec2(TopBox.x, TopBox.y - 5), CFG.enemyfont_size, ImColor(255, 255, 255), true);
				}
				if (CFG.b_EspDistance)
				{
					DrawOutlinedText(Verdana, xorstr("Distance: [") + std::to_string(dist) + xorstr("]"), ImVec2(BottomBox.x, BottomBox.y + 5), CFG.enemyfont_size, ImColor(255, 255, 255), true);
				}
				if (CFG.crosshair)
				{
					DrawCircle(GameVars.ScreenWidth / 2, GameVars.ScreenHeight / 2, 2, ImColor(255, 255, 255), 100);
				}
				if (CFG.b_EspSkeleton)
				{
					Vector3 vHeadBone = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::head));
					Vector3 vHip = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::pelvis));
					Vector3 vNeck = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::neck_01));
					Vector3 vUpperArmLeft = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::upperarm_l));
					Vector3 vUpperArmRight = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::upperarm_r));
					Vector3 vLeftHand = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::hand_l));
					Vector3 vRightHand = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::hand_r));
					Vector3 vRightThigh = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::thigh_r));
					Vector3 vLeftThigh = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::thigh_l));
					Vector3 vRightCalf = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::calf_r));
					Vector3 vLeftCalf = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::calf_l));
					Vector3 vLeftFoot = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::foot_l));
					Vector3 vRightFoot = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::foot_r));

					Vector3 vLeftHandMiddle = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::ik_hand_l));
					Vector3 vRightHandMiddle = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::ik_hand_r));

					Vector3 VRoot = ProjectWorldToScreen(GetBoneWithRotation(Entity.actor_mesh, bones::Root));

					DrawLine(ImVec2(vHeadBone.x, vHeadBone.y), ImVec2(vNeck.x, vNeck.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vHip.x, vHip.y), ImVec2(vNeck.x, vNeck.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vUpperArmLeft.x, vUpperArmLeft.y), ImVec2(vNeck.x, vNeck.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vUpperArmRight.x, vUpperArmRight.y), ImVec2(vNeck.x, vNeck.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vLeftHand.x, vLeftHand.y), ImVec2(vLeftHandMiddle.x, vLeftHandMiddle.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vRightHand.x, vRightHand.y), ImVec2(vRightHandMiddle.x, vRightHandMiddle.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vLeftThigh.x, vLeftThigh.y), ImVec2(vHip.x, vHip.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vRightThigh.x, vRightThigh.y), ImVec2(vHip.x, vHip.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vLeftCalf.x, vLeftCalf.y), ImVec2(vLeftThigh.x, vLeftThigh.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vRightCalf.x, vRightCalf.y), ImVec2(vRightThigh.x, vRightThigh.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vLeftFoot.x, vLeftFoot.y), ImVec2(vLeftCalf.x, vLeftCalf.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vRightFoot.x, vRightFoot.y), ImVec2(vRightCalf.x, vRightCalf.y), ImColor(255, 255, 255), 2);

					DrawLine(ImVec2(vLeftHandMiddle.x, vLeftHandMiddle.y), ImVec2(vUpperArmLeft.x, vUpperArmLeft.y), ImColor(255, 255, 255), 2);
					DrawLine(ImVec2(vRightHandMiddle.x, vRightHandMiddle.y), ImVec2(vUpperArmRight.x, vUpperArmRight.y), ImColor(255, 255, 255), 2);
				}
				if (CFG.allitems)
				{
					//for (int a = 0; a < 141; ++a) {
					//	auto BonePos = GetBoneWithRotation(Entity.actor_mesh, a);
					//	auto Screen = ProjectWorldToScreen(BonePos);

					//	DrawOutlinedText(Verdana, std::to_string(a), ImVec2(Screen.x, Screen.y), 16.0f, ImColor(255, 255, 255), true);
					//	//DrawString(ImVec2(Screen.x, Screen.y), std::to_string(a), IM_COL32_WHITE);
					//}
					//auto ScreenPos = Vector3(Entity.actor_pos.x, Entity.actor_pos.y, Entity.actor_pos.z);
					//auto Screen = ProjectWorldToScreen(ScreenPos);

					//DrawOutlinedText(Verdana, Entity.actor_name, ImVec2(Screen.x, Screen.y), 14.0f, ImColor(255, 255, 255), true);
				}
			}
		}
	}
}


auto ItemCache() -> VOID
{
	while (true)
	{
		//std::vector<EntityList> tmpList;
		std::vector<EntityList> entityBot;

		if(CFG.showvehicle || CFG.debug_b)
		{
			for (int index = 0; index < GameVars.actor_count; ++index)
			{

				auto actor_pawn = read<uintptr_t>(GameVars.actors + index * 0x8);
				if (actor_pawn == 0x00)
					continue;

				if (actor_pawn == GameVars.local_player_pawn)
					continue;

				auto actor_id = read<int>(actor_pawn + GameOffset.offset_actor_id);
				auto actor_mesh = read<uintptr_t>(actor_pawn + GameOffset.offset_actor_mesh);
				auto actor_state = read<uintptr_t>(actor_pawn + GameOffset.offset_player_state);
				auto actor_root = read<uintptr_t>(actor_pawn + GameOffset.offset_root_component);
				if (!actor_root) continue;
				auto actor_pos = read<Vector3>(actor_root + GameOffset.offset_relative_location);
				if (actor_pos.x == 0 || actor_pos.y == 0 || actor_pos.z == 0) continue;


				auto name = GetNameFromFName(actor_id);

				auto ScreenPos = Vector3(actor_pos);
				auto Screen = ProjectWorldToScreen(ScreenPos);

				auto local_pos = read<Vector3>(GameVars.local_player_root + GameOffset.offset_relative_location);
				auto entity_distance = local_pos.Distance(ScreenPos);

				int dist = entity_distance;

				//EntityList Bot{ };
				//Bot.bot_id = actor_id;
				//Bot.bot_name = name;
				//Bot.bot_pos = actor_pos;
				//entityBot.push_back(Bot);

				if (entity_distance > CFG.itemdistance)
					continue;

				if (CFG.Minsk)
				{
					if (name.find(xorstr("BP_minsk")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Pickup)
				{
					if (name.find(xorstr("BP_Technical")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Jeep)
				{
					if (name.find(xorstr("BP_Safir")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.LUVA1)
				{
					if (name.find(xorstr("BP_M-Gator")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.M939)
				{
					if (name.find(xorstr("BP_US_Util")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.HX60)
				{
					if (name.find(xorstr("BP_Brit_Util")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Ural375D)
				{
					if (name.find(xorstr("BP_Ural_375")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Ural4320)
				{
					if (name.find(xorstr("BP_Ural_4320")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Kamaz)
				{
					if (name.find(xorstr("BP_Kamaz_5350")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.MATV)
				{
					if (name.find(xorstr("BP_MATV")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Tigr)
				{
					if (name.find(xorstr("BP_Tigr")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.TAPV)
				{
					if (name.find(xorstr("BP_TAPV")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.BRDM2)
				{
					if (name.find(xorstr("BP_BRDM-2")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.LPPV)
				{
					if (name.find(xorstr("BP_LPPV")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.BTR80)
				{
					if (name.find(xorstr("BP_BTR80_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.BTR82)
				{
					if (name.find(xorstr("BP_BTR82A_")) != std::string::npos || name.find(xorstr("BP_BTR-D")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.LAV6)
				{
					if (name.find(xorstr("BP_LAV6_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.LAV3)
				{
					if (name.find(xorstr("BP_LAV_RWS")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.M113A3)
				{
					if (name.find(xorstr("BP_M113A3_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.M1126)
				{
					if (name.find(xorstr("BP_M1126_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.FV432)
				{
					if (name.find(xorstr("BP_FV432_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.FV107)
				{
					if (name.find(xorstr("BP_FV107")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.MTLB)
				{
					if (name.find(xorstr("BP_MTLB_")) != std::string::npos || name.find(xorstr("BP_MTLBM")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.BMP1)
				{
					if (name.find(xorstr("BP_BMP1")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.BMP2)
				{
					if (name.find(xorstr("BP_BMP2")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.M2A3)
				{
					if (name.find(xorstr("BP_BFV")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.FV510)
				{
					if (name.find(xorstr("BP_FV510")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.FV4034)
				{
					if (name.find(xorstr("BP_2A6_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.MSVS)
				{
					if (name.find(xorstr("BP_CAF_Util_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.M1A2)
				{
					if (name.find(xorstr("BP_M1A2")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.T62)
				{
					if (name.find(xorstr("BP_T62_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.T72B3)
				{
					if (name.find(xorstr("BP_T72B3")) != std::string::npos || name.find(xorstr("BP_T72AV")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.Mi8)
				{
					if (name.find(xorstr("BP_MI8")) != std::string::npos || name.find(xorstr("BP_MI17")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.UH60M)
				{
					if (name.find(xorstr("BP_UH60_C")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.SA330)
				{
					if (name.find(xorstr("BP_SA330_C")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.hub)
				{
					if (name.find(xorstr("_Hab_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				if (CFG.mine)
				{
					if (name.find(xorstr("BP_Deployable_")) != std::string::npos)
					{
						EntityList Bot{ };
						Bot.bot_id = actor_id;
						Bot.bot_name = name;
						Bot.bot_pos = actor_pos;
						entityBot.push_back(Bot);
					}
				}
				else
					continue;
			}
		}
		entityBotList = entityBot;
		std::this_thread::sleep_for(std::chrono::milliseconds(1500));
		//Sleep(2500);
	}
}

auto Items() -> VOID
{
	//auto EntityList_Copy = entityList;
	auto BotList_Copy = entityBotList;
	if (CFG.debug_b || CFG.showvehicle)
	{
		for (int index = 0; index < BotList_Copy.size(); ++index)
		{
			auto Bot = BotList_Copy[index];

			auto ScreenPos = Vector3(Bot.bot_pos);
			auto Screen = ProjectWorldToScreen(ScreenPos);

			auto local_pos = read<Vector3>(GameVars.local_player_root + GameOffset.offset_relative_location);
			auto entity_distance = local_pos.Distance(ScreenPos);

			int dist = entity_distance;

			//if (entity_distance < CFG.max_distanceAIM)
			//{
			//	DrawOutlinedText(Verdana, Bot.bot_name, ImVec2(Screen.x, Screen.y), 14.0f, ImColor(255, 255, 255), true);
			//}

			if (CFG.Minsk)
			{
				if (Bot.bot_name.find(xorstr("BP_minsk")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Minsk Motorbike [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.exit_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Pickup)
			{
				if (Bot.bot_name.find(xorstr("BP_Technical")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Pickup Truck [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.exitname_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Jeep)
			{
				if (Bot.bot_name.find(xorstr("BP_Safir")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Jeep [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color_stash, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.LUVA1)
			{
				if (Bot.bot_name.find(xorstr("BP_M-Gator")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "LUV-A1 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.spaceexit_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.M939)
			{
				if (Bot.bot_name.find(xorstr("BP_US_Util")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "M939 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.mine_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.HX60)
			{
				if (Bot.bot_name.find(xorstr("BP_Brit_Util")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "MAN HX60 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.rarechest_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Ural375D)
			{
				if (Bot.bot_name.find(xorstr("BP_Ural_375")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Ural 375 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.medicchest_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Ural4320)
			{
				if (Bot.bot_name.find(xorstr("BP_Ural_4320")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Ural 4320 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.dead_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Kamaz)
			{
				if (Bot.bot_name.find(xorstr("BP_Kamaz_5350")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Kamaz 5350 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.ship_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.MATV)
			{
				if (Bot.bot_name.find(xorstr("BP_MATV")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "M-ATV [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.shipname_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Tigr)
			{
				if (Bot.bot_name.find(xorstr("BP_Tigr")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Tigr-M [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.shipbot_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.TAPV)
			{
				if (Bot.bot_name.find(xorstr("BP_TAPV")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "TAPV [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.allitems_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.BRDM2)
			{
				if (Bot.bot_name.find(xorstr("BP_BRDM-2")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "BRDM-2 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.quest_objects_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.LPPV)
			{
				if (Bot.bot_name.find(xorstr("BP_LPPV")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "LPPV [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.item_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.BTR80)
			{
				if (Bot.bot_name.find(xorstr("BP_BTR80_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "BTR-80 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.rare_chest_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.BTR82)
			{
				if (Bot.bot_name.find(xorstr("BP_BTR82A_")) != std::string::npos || Bot.bot_name.find(xorstr("BP_BTR-D")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "BTR-82A [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.common_chest_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.LAV6)
			{
				if (Bot.bot_name.find(xorstr("BP_LAV6_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "LAV 6.0 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.quest_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.LAV3)
			{
				if (Bot.bot_name.find(xorstr("BP_LAV_RWS")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "LAV III [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.large_safe_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.M113A3)
			{
				if (Bot.bot_name.find(xorstr("BP_M113A3_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "M113A3 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.briefcase_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.M1126)
			{
				if (Bot.bot_name.find(xorstr("BP_M1126_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "M1126 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.industrial_chest_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.FV432)
			{
				if (Bot.bot_name.find(xorstr("BP_FV432_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "FV432 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.prison_switch_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.FV107)
			{
				if (Bot.bot_name.find(xorstr("BP_FV107")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "FV107 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.capsule_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.MTLB)
			{
				if (Bot.bot_name.find(xorstr("BP_MTLB_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "MT-LB [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.capsulehole_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.BMP1)
			{
				if (Bot.bot_name.find(xorstr("BP_BMP1")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "BMP-1 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.mine_actor_color, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.BMP2)
			{
				if (Bot.bot_name.find(xorstr("BP_BMP2")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "BMP-2 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color14, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.M2A3)
			{
				if (Bot.bot_name.find(xorstr("BP_BFV")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "M2A3 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color15, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.FV510)
			{
				if (Bot.bot_name.find(xorstr("BP_FV510")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "FV510 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color16, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.FV4034)
			{
				if (Bot.bot_name.find(xorstr("BP_2A6_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "FV4034 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color17, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.MSVS)
			{
				if (Bot.bot_name.find(xorstr("BP_CAF_Util_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "MSVS [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color18, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.M1A2)
			{
				if (Bot.bot_name.find(xorstr("BP_M1A2")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "M1A2 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color19, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.T62)
			{
				if (Bot.bot_name.find(xorstr("BP_T62_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "T-62 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color20, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.T72B3)
			{
				if (Bot.bot_name.find(xorstr("BP_T72B3")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "T-72B3 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color21, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.Mi8)
			{
				if (Bot.bot_name.find(xorstr("BP_MI8")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Mi-8 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color22, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.UH60M)
			{
				if (Bot.bot_name.find(xorstr("BP_UH60_C")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "UH-60M [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color23, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.SA330)
			{
				if (Bot.bot_name.find(xorstr("BP_SA330_C")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "SA330 [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color24, true);
					DrawCircle(ImVec2(Screen.x, Screen.y), 6, CFG.FovColor, 0);
				}
			}
			if (CFG.hub)
			{
				if (Bot.bot_name.find(xorstr("_Hab_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "FOB Base [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color25, true);
				}
			}
			if (CFG.mine)
			{
				if (Bot.bot_name.find(xorstr("BP_Deployable_")) != std::string::npos)
				{
					DrawOutlinedText(Verdana, "Mine [" + std::to_string(dist) + "]", ImVec2(Screen.x, Screen.y + 15), CFG.font_size, CFG.color26, true);
				}
			}
		}
	}
}


void InputHandler() {
	for (int i = 0; i < 5; i++) ImGui::GetIO().MouseDown[i] = false;
	int button = -1;
	if (GetAsyncKeyState(VK_LBUTTON)) button = 0;
	if (button != -1) ImGui::GetIO().MouseDown[button] = true;
}

bool MenuKey()
{
	return GetAsyncKeyState(CFG.MENUkeys[CFG.MENUKey]) & 1;
}
auto s = ImVec2{}, p = ImVec2{}, gs = ImVec2{ 1020, 718 };
void Render()
{
	if (MenuKey())
	{
		CFG.b_MenuShow = !CFG.b_MenuShow;
		//KeyAuthApp.check();
		//if (!KeyAuthApp.data.success)
		{

		}
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	RenderVisual();
	Items();
	ImGui::GetIO().MouseDrawCursor = CFG.b_MenuShow;

	// Set custom colors
	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowMinSize = ImVec2(256, 300);
	style.WindowTitleAlign = ImVec2(0.5, 0.5);
	style.FrameBorderSize = 0;
	style.ChildBorderSize = 0;
	style.WindowBorderSize = 0;
	style.WindowRounding = 6;   // Задайте значение для округления
	style.FrameRounding = 6;    // Задайте значение для округления
	style.ChildRounding = 6;    // Задайте значение для округления
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 0.85f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.09f, 0.12f, 0.85f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.09f, 0.12f, 0.85f);
	style.Colors[ImGuiCol_WindowBg] = ImColor(18, 18, 20);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.40f, 0.90f, 0.43f, 0.80f);
	style.Colors[ImGuiCol_Border] = ImColor(23, 235, 58, 255);
	style.Colors[ImGuiCol_Button] = ImColor(32, 32, 32);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(42, 42, 42);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(42, 42, 42);
	style.Colors[ImGuiCol_ChildBg] = ImColor(45, 45, 45);
	style.Colors[ImGuiCol_FrameBg] = ImColor(32, 32, 32);
	style.Colors[ImGuiCol_FrameBgActive] = ImColor(42, 42, 42);
	style.Colors[ImGuiCol_FrameBgHovered] = ImColor(42, 42, 42);
	style.Colors[ImGuiCol_SliderGrab] = ImColor(255, 255, 255);
	style.Colors[ImGuiCol_SliderGrabActive] = ImColor(255, 255, 255);

	style.Colors[ImGuiCol_Separator] = ImColor(23, 235, 58, 255);
	style.Colors[ImGuiCol_SeparatorHovered] = ImColor(23, 235, 58, 255);
	style.Colors[ImGuiCol_SeparatorActive] = ImColor(23, 235, 58, 255);
	

	DrawOutlinedText(Verdana, (xorstr("P U S S Y C A T")), ImVec2(55, 12), 12, ImColor(23, 235, 58), true);

	if (CFG.b_MenuShow)
	{
		InputHandler();
		ImGui::SetNextWindowSize(ImVec2(600, 560));
		if (CFG.tab_index == 3)
		{
			ImGui::SetNextWindowSize(ImVec2(600, 646));
		}
		ImGui::PushFont(Verdana);

		ImGui::Begin(xorstr("P U S S Y C A T"), 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);

		ImGui::BeginGroup();
		ImGui::Indent();
		ImGui::Text(xorstr(""));
		ImGui::Spacing();
		TabButton(xorstr("Aimbot"), &CFG.tab_index, 0, false);
		ImGui::Spacing();
		TabButton(xorstr("Enemy"), &CFG.tab_index, 1, false);
		ImGui::Spacing();
		TabButton(xorstr("Items"), &CFG.tab_index, 2, false);
		ImGui::Spacing();
		TabButton(xorstr("Vehicle"), &CFG.tab_index, 3, false);
		ImGui::Spacing();
		TabButton(xorstr("Misc"), &CFG.tab_index, 4, false);

		ImGui::EndGroup();
		ImGui::SameLine();

		ImGui::BeginGroup();

		if (CFG.tab_index == 0)
		{
			ImGui::Indent();
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

			custom.Checkbox(xorstr("Vector Aimbot"), &CFG.b_Aimbot);
			ImGui::Separator();
			ImGui::NewLine();
			if (CFG.b_Aimbot)
			{
				custom.Checkbox(xorstr("Draw FOV"), &CFG.b_AimbotFOV); // Add this line back
				if (CFG.b_AimbotFOV)
				{
					custom.SliderInt(xorstr("Radius FOV"), &CFG.AimbotFOV, 1, 300);
				}
				ImGui::NewLine();

				custom.SliderInt(xorstr("Smoothing"), &CFG.Smoothing, 1, 10);
				ImGui::NewLine();

				ImGui::Text(xorstr("Target Bone"));
				ImGui::Combo(xorstr("         "), &CFG.boneType, CFG.BoneTypes, 2);
				ImGui::NewLine();

				custom.SliderInt(xorstr("Max Distance"), &CFG.max_distanceAIM, 1, 1000);
				ImGui::NewLine();


				ImGui::Text(xorstr("Aimbot Key"));
				ImGui::Combo(xorstr("             "), &CFG.AimKey, keyItems, IM_ARRAYSIZE(keyItems));

			}

			ImGui::PopStyleVar();
		}

		else if (CFG.tab_index == 1)
		{
			ImGui::Indent();
			ImGui::RadioButton(xorstr("Players"), &CFG.b_Visual);
			ImGui::Separator();
			ImGui::NewLine();
			if (CFG.b_Visual)
			{
				ImGui::Spacing();
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				custom.Checkbox(xorstr("Draw BOX"), &CFG.b_EspBox);
				custom.Checkbox(xorstr("Skeleton"), &CFG.b_EspSkeleton);
				custom.Checkbox(xorstr("Tracelines"), &CFG.b_EspLine);
				custom.Checkbox(xorstr("Ignore Team"), &CFG.ignoreteam);
				custom.Checkbox(xorstr("Enemy Tag"), &CFG.b_EspName);
				custom.Checkbox(xorstr("Distance"), &CFG.b_EspDistance);
				custom.Checkbox(xorstr("HealthPoints"), &CFG.b_EspHealthHP);
				custom.Checkbox(xorstr("HealthBar"), &CFG.b_EspHealth);
			}
			ImGui::NewLine();
			ImGui::NewLine();
			ImGui::NewLine();
			ImGui::Text(xorstr("Max Distance"));
			custom.SliderInt(xorstr("   "), &CFG.max_distance, 1, 2000);

			if (CFG.b_EspBox)
			{
				ImGui::Text(xorstr("BOX Type"));
				ImGui::Combo(xorstr("  "), &CFG.BoxType, CFG.BoxTypes, 2);
			}
			if (CFG.b_EspLine)
			{
				ImGui::Text(xorstr("Tracelines Type"));
				ImGui::Combo(xorstr(" "), &CFG.LineType, CFG.LineTypes, 3);
			}
			ImGui::PopStyleVar();
		}

		else if (CFG.tab_index == 2)
		{
			ImGui::BeginGroup(); // Begin the entire group

			custom.Checkbox(xorstr("Show Items"), &CFG.debug_b);
			ImGui::Separator();
			ImGui::NewLine();
			if (CFG.debug_b)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

				ImGui::Indent();
				ImGui::Columns(2, nullptr, false);

				ImGui::NewLine();
				custom.Checkbox(xorstr("FOB Base"), &CFG.hub);
				ImGui::SameLine();
				ImGui::ColorEdit3("##Base", (float*)&CFG.color25, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Deployable Mine"), &CFG.mine);
				ImGui::SameLine();
				ImGui::ColorEdit3("##Deployable", (float*)&CFG.color26, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				ImGui::Columns(1); // End the columns

				ImGui::PopStyleVar();
			}
			ImGui::EndGroup();

		}

		else if (CFG.tab_index == 3)
		{
			ImGui::BeginGroup(); // Begin the entire group

			custom.Checkbox(xorstr("Show Vehicle"), &CFG.showvehicle);
			ImGui::Separator();
			ImGui::NewLine();
			if (CFG.showvehicle)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

				ImGui::Indent();
				ImGui::Columns(2, nullptr, false);

				ImGui::NewLine();
				custom.Checkbox(xorstr("Minsk Motorbike"), &CFG.Minsk);
				ImGui::SameLine();
				ImGui::ColorEdit3("##MinskMotorbike", (float*)&CFG.exit_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Pickup Truck"), &CFG.Pickup);
				ImGui::SameLine();
				ImGui::ColorEdit4("##PickupTruck", (float*)&CFG.exitname_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Jeep"), &CFG.Jeep);
				ImGui::SameLine();
				ImGui::ColorEdit4("##Jeep", (float*)&CFG.color_stash, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("LUV-A1"), &CFG.LUVA1);
				ImGui::SameLine();
				ImGui::ColorEdit4("##LUVA1", (float*)&CFG.spaceexit_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("M939"), &CFG.M939);
				ImGui::SameLine();
				ImGui::ColorEdit4("##M939", (float*)&CFG.mine_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("MAN HX60"), &CFG.HX60);
				ImGui::SameLine();
				ImGui::ColorEdit4("##MANHX60", (float*)&CFG.rarechest_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Ural 375"), &CFG.Ural375D);
				ImGui::SameLine();
				ImGui::ColorEdit4("##Ural375", (float*)&CFG.medicchest_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Ural 4320"), &CFG.Ural4320);
				ImGui::SameLine();
				ImGui::ColorEdit4("##Ural4320", (float*)&CFG.dead_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Kamaz 5350"), &CFG.Kamaz);
				ImGui::SameLine();
				ImGui::ColorEdit4("##Kamaz5350", (float*)&CFG.ship_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("M-ATV"), &CFG.MATV);
				ImGui::SameLine();
				ImGui::ColorEdit4("##MATV", (float*)&CFG.shipname_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Tigr-M"), &CFG.Tigr);
				ImGui::SameLine();
				ImGui::ColorEdit4("##TigrM", (float*)&CFG.shipbot_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("TAPV"), &CFG.TAPV);
				ImGui::SameLine();
				ImGui::ColorEdit4("##TAPV", (float*)&CFG.allitems_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("BRDM-2"), &CFG.BRDM2);
				ImGui::SameLine();
				ImGui::ColorEdit4("##BRDM2", (float*)&CFG.quest_objects_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("LPPV"), &CFG.LPPV);
				ImGui::SameLine();
				ImGui::ColorEdit4("##LPPV", (float*)&CFG.item_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("BTR-80"), &CFG.BTR80);
				ImGui::SameLine();
				ImGui::ColorEdit4("##BTR80", (float*)&CFG.rare_chest_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("BTR-82A"), &CFG.BTR82);
				ImGui::SameLine();
				ImGui::ColorEdit4("##BTR82A", (float*)&CFG.common_chest_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("LAV 6.0"), &CFG.LAV6);
				ImGui::SameLine();
				ImGui::ColorEdit4("##LAV60", (float*)&CFG.quest_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				//ImGui::EndGroup();

				//ImGui::SameLine();

				ImGui::NextColumn(); // Move to the next column

				//ImGui::Spacing();

				ImGui::BeginGroup();

				ImGui::NewLine();
				custom.Checkbox(xorstr("LAV III"), &CFG.LAV3);
				ImGui::SameLine();
				ImGui::ColorEdit4("##LAVIII", (float*)&CFG.large_safe_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("M113A3"), &CFG.M113A3);
				ImGui::SameLine();
				ImGui::ColorEdit4("##M113A3", (float*)&CFG.briefcase_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("M1126"), &CFG.M1126);
				ImGui::SameLine();
				ImGui::ColorEdit4("##M1126", (float*)&CFG.industrial_chest_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("FV432"), &CFG.FV432);
				ImGui::SameLine();
				ImGui::ColorEdit4("##FV432", (float*)&CFG.prison_switch_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("FV107"), &CFG.FV107);
				ImGui::SameLine();
				ImGui::ColorEdit4("##FV107", (float*)&CFG.capsule_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("MT-LB"), &CFG.MTLB);
				ImGui::SameLine();
				ImGui::ColorEdit4("##MTLB", (float*)&CFG.capsulehole_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("BMP-1"), &CFG.BMP1);
				ImGui::SameLine();
				ImGui::ColorEdit4("##BMP1", (float*)&CFG.mine_actor_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("BMP-2"), &CFG.BMP2);
				ImGui::SameLine();
				ImGui::ColorEdit4("##BMP2", (float*)&CFG.color14, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("M2A3"), &CFG.M2A3);
				ImGui::SameLine();
				ImGui::ColorEdit4("##M2A3", (float*)&CFG.color15, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("FV510"), &CFG.FV510);
				ImGui::SameLine();
				ImGui::ColorEdit4("##FV510", (float*)&CFG.color16, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("FV4034"), &CFG.FV4034);
				ImGui::SameLine();
				ImGui::ColorEdit4("##FV4034", (float*)&CFG.color17, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("MSVS"), &CFG.MSVS);
				ImGui::SameLine();
				ImGui::ColorEdit4("##MSVS", (float*)&CFG.color18, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("M1A2"), &CFG.M1A2);
				ImGui::SameLine();
				ImGui::ColorEdit4("##M1A2", (float*)&CFG.color19, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("T-62"), &CFG.T62);
				ImGui::SameLine();
				ImGui::ColorEdit4("##T62", (float*)&CFG.color20, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("T-72B3"), &CFG.T72B3);
				ImGui::SameLine();
				ImGui::ColorEdit4("##T72B3", (float*)&CFG.color21, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("Mi-8"), &CFG.Mi8);
				ImGui::SameLine();
				ImGui::ColorEdit4("##Mi8", (float*)&CFG.color22, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("UH-60M"), &CFG.UH60M);
				ImGui::SameLine();
				ImGui::ColorEdit4("##UH60M", (float*)&CFG.color23, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				custom.Checkbox(xorstr("SA330"), &CFG.SA330);
				ImGui::SameLine();
				ImGui::ColorEdit4("##SA330", (float*)&CFG.color24, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_NoBorder);

				//ImGui::EndGroup();

				ImGui::Columns(1); // End the columns

				//ImGui::EndGroup();itemdistance
				ImGui::NewLine();
				custom.SliderInt(xorstr("Vehicle Distance"), &CFG.itemdistance, 1, 1000);

				ImGui::PopStyleVar();
			}
			ImGui::EndGroup();

		}

		else if (CFG.tab_index == 4)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

			ImGui::Indent();
			ImGui::NewLine();
			ImGui::NewLine();
			custom.Checkbox(xorstr("Crosshair"), &CFG.crosshair);
			custom.Checkbox(xorstr("HP Hud"), &CFG.guihp);
			ImGui::NewLine();
			ImGui::NewLine();
			ImGui::NewLine();

			ImGui::NewLine();
			ImGui::NewLine();
			custom.SliderFloat(xorstr("Enemy Font Size"), &CFG.enemyfont_size, 1.0f, 24.0f);
			ImGui::NewLine();
			custom.SliderFloat(xorstr("Item Font Size"), &CFG.font_size, 1.0f, 24.0f);
			ImGui::NewLine();
			ImGui::NewLine();

			ImGui::Text(xorstr("Menu Key"));
			ImGui::Combo(xorstr("              "), &CFG.MENUKey, CFG.keyMENU, 6);

			ImGui::PopStyleVar();
		}

		ImGui::EndGroup();

		ImGui::PopFont();
		ImGui::End();
	}
	ImGui::EndFrame();

	DirectX9Interface::pDevice->SetRenderState(D3DRS_ZENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	DirectX9Interface::pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);

	DirectX9Interface::pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (DirectX9Interface::pDevice->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		DirectX9Interface::pDevice->EndScene();
	}

	HRESULT result = DirectX9Interface::pDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && DirectX9Interface::pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}
void MainLoop() {
	static RECT OldRect;
	ZeroMemory(&DirectX9Interface::Message, sizeof(MSG));

	while (DirectX9Interface::Message.message != WM_QUIT) {
		if (PeekMessage(&DirectX9Interface::Message, OverlayWindow::Hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&DirectX9Interface::Message);
			DispatchMessage(&DirectX9Interface::Message);
		}
		HWND ForegroundWindow = GetForegroundWindow();
		if (ForegroundWindow == GameVars.gameHWND) {
			HWND TempProcessHwnd = GetWindow(ForegroundWindow, GW_HWNDPREV);
			SetWindowPos(OverlayWindow::Hwnd, TempProcessHwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		RECT TempRect;
		POINT TempPoint;
		ZeroMemory(&TempRect, sizeof(RECT));
		ZeroMemory(&TempPoint, sizeof(POINT));

		GetClientRect(GameVars.gameHWND, &TempRect);
		ClientToScreen(GameVars.gameHWND, &TempPoint);

		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = GameVars.gameHWND;

		POINT TempPoint2;
		GetCursorPos(&TempPoint2);
		io.MousePos.x = TempPoint2.x - TempPoint.x;
		io.MousePos.y = TempPoint2.y - TempPoint.y;

		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}

		if (TempRect.left != OldRect.left || TempRect.right != OldRect.right || TempRect.top != OldRect.top || TempRect.bottom != OldRect.bottom) {
			OldRect = TempRect;
			GameVars.ScreenWidth = TempRect.right;
			GameVars.ScreenHeight = TempRect.bottom;
			DirectX9Interface::pParams.BackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
			DirectX9Interface::pParams.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
			SetWindowPos(OverlayWindow::Hwnd, (HWND)0, TempPoint.x, TempPoint.y, GameVars.ScreenWidth, GameVars.ScreenHeight, SWP_NOREDRAW);
			DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
		}
		if (DirectX9Interface::Message.message == WM_QUIT || GetAsyncKeyState(VK_DELETE) || (FindWindowA("UnrealWindow", nullptr) == NULL))
			exit(0);
		Render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	if (DirectX9Interface::pDevice != NULL) {
		DirectX9Interface::pDevice->EndScene();
		DirectX9Interface::pDevice->Release();
	}
	if (DirectX9Interface::Direct3D9 != NULL) {
		DirectX9Interface::Direct3D9->Release();
	}
	DestroyWindow(OverlayWindow::Hwnd);
	UnregisterClass(OverlayWindow::WindowClass.lpszClassName, OverlayWindow::WindowClass.hInstance);
}

bool DirectXInit() {
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &DirectX9Interface::Direct3D9))) {
		return false;
	}

	D3DPRESENT_PARAMETERS Params = { 0 };
	Params.Windowed = TRUE;
	Params.SwapEffect = D3DSWAPEFFECT_DISCARD;
	Params.hDeviceWindow = OverlayWindow::Hwnd;
	Params.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	Params.BackBufferFormat = D3DFMT_A8R8G8B8;
	Params.BackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
	Params.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.EnableAutoDepthStencil = TRUE;
	Params.AutoDepthStencilFormat = D3DFMT_D16;
	Params.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	Params.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	if (FAILED(DirectX9Interface::Direct3D9->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, OverlayWindow::Hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &Params, 0, &DirectX9Interface::pDevice))) {
		DirectX9Interface::Direct3D9->Release();
		return false;
	}

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplWin32_Init(OverlayWindow::Hwnd);
	ImGui_ImplDX9_Init(DirectX9Interface::pDevice);
	DirectX9Interface::Direct3D9->Release();
	return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message) {
	case WM_DESTROY:
		if (DirectX9Interface::pDevice != NULL) {
			DirectX9Interface::pDevice->EndScene();
			DirectX9Interface::pDevice->Release();
		}
		if (DirectX9Interface::Direct3D9 != NULL) {
			DirectX9Interface::Direct3D9->Release();
		}
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (DirectX9Interface::pDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			DirectX9Interface::pParams.BackBufferWidth = LOWORD(lParam);
			DirectX9Interface::pParams.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = DirectX9Interface::pDevice->Reset(&DirectX9Interface::pParams);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void SetupWindow() {
	OverlayWindow::WindowClass = {
		sizeof(WNDCLASSEX), 0, WinProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, OverlayWindow::Name, LoadIcon(nullptr, IDI_APPLICATION)
	};

	RegisterClassEx(&OverlayWindow::WindowClass);
	if (GameVars.gameHWND) {
		static RECT TempRect = { NULL };
		static POINT TempPoint;
		GetClientRect(GameVars.gameHWND, &TempRect);
		ClientToScreen(GameVars.gameHWND, &TempPoint);
		TempRect.left = TempPoint.x;
		TempRect.top = TempPoint.y;
		GameVars.ScreenWidth = TempRect.right;
		GameVars.ScreenHeight = TempRect.bottom;
	}

	OverlayWindow::Hwnd = CreateWindowEx(NULL, OverlayWindow::Name, OverlayWindow::Name, WS_POPUP | WS_VISIBLE, GameVars.ScreenLeft, GameVars.ScreenTop, GameVars.ScreenWidth, GameVars.ScreenHeight, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(OverlayWindow::Hwnd, &DirectX9Interface::Margin);
	SetWindowLong(OverlayWindow::Hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	ShowWindow(OverlayWindow::Hwnd, SW_SHOW);
	UpdateWindow(OverlayWindow::Hwnd);
}
bool checkinternet;
HWND hWnd;

int main(int argCount, char** argVector)
{
	std::thread(Protection_Loop).detach();
	//Update();
	//KeyAuthApp.init();
	//std::thread(mainprotect).detach();
	SetConsoleTitleA(RandomStrings(16).c_str());

	std::string filePath = argVector[0];

	if (!RenameFile(filePath))
	{
		return -1;
	}

	

	LI_FN(printf).get()(skCrypt("\n Waiting.."));
	system(xorstr("cls"));

	std::string drive = "C:\\"; //added

	//DetectDebuggerThread();
	std::string key;
	LI_FN(printf).get()(skCrypt("\n ENTER KEY: "));
	std::cin >> key;

	//HWND warn = FindWindowA("UnrealWindow", nullptr);
	//if (warn)
	{
		//system(xorstr("cls"));
		//std::cout << xorstr("[-] Close SQUAD, after start loader") << std::endl;
		//Sleep(5000);
		//exit(-1);
	}

//	system(xorstr("KDM.exe KDM.sys"));
//	driver::find_driver();
//	system(xorstr("cls"));

	printf(xorstr("[+] Driver: Loading...\n", driver_handle));
	if (!driver_handle || (driver_handle == INVALID_HANDLE_VALUE))
	{
		system(xorstr("cls"));
	}

	printf(xorstr("[+] Driver: Loaded\n", driver_handle));

	Sleep(2500);
	system(xorstr("cls"));

	std::cout << xorstr("[+] Press F2 in SQUAD...\n\n");
	while (true)
	{
		if (GetAsyncKeyState(VK_F2))
			break;

		Sleep(50);
	}

	driver::find_driver();
	ProcId = driver::find_process(GameVars.dwProcessName);
	BaseId = driver::find_image();
	GameVars.dwProcessId = ProcId;
	GameVars.dwProcess_Base = BaseId;
	system(xorstr("cls"));

	PrintPtr(xorstr("[+] ProcessId: "), GameVars.dwProcessId);
	PrintPtr(xorstr("[+] BaseId: "), GameVars.dwProcess_Base);
	if (GameVars.dwProcessId == 0 || GameVars.dwProcess_Base == 0)
	{
		std::cout << xorstr("[-] Something not found...\n\n");
		std::cout << xorstr("[-] Try again...\n\n");
	}

	HWND tWnd = FindWindowA("UnrealWindow", nullptr);
	if (tWnd)
	{

		GameVars.gameHWND = tWnd;
		RECT clientRect;
		GetClientRect(GameVars.gameHWND, &clientRect);
		POINT screenCoords = { clientRect.left, clientRect.top };
		ClientToScreen(GameVars.gameHWND, &screenCoords);
	}

	//std::thread(GameCache).detach();
	//std::thread(CallAimbot).detach();

	CreateThread(nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(GameCache), nullptr, NULL, nullptr);
	CreateThread(nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(CallAimbot), nullptr, NULL, nullptr);
	CreateThread(nullptr, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(ItemCache), nullptr, NULL, nullptr);
	
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	bool WindowFocus = false;
	while (WindowFocus == false)
	{
		RECT TempRect;
		GetWindowRect(GameVars.gameHWND, &TempRect);
		GameVars.ScreenWidth = TempRect.right - TempRect.left;
		GameVars.ScreenHeight = TempRect.bottom - TempRect.top;
		GameVars.ScreenLeft = TempRect.left;
		GameVars.ScreenRight = TempRect.right;
		GameVars.ScreenTop = TempRect.top;
		GameVars.ScreenBottom = TempRect.bottom;
		WindowFocus = true;

	}


	OverlayWindow::Name = RandomString(10).c_str();
	SetupWindow();
	DirectXInit();

	ImGuiIO& io = ImGui::GetIO();
	DefaultFont = io.Fonts->AddFontDefault();
	Verdana = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahomabd.ttf", 16.0f, NULL, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
	io.Fonts->Build();


	while (TRUE)
	{
		MainLoop();
	}

}
