#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "singleton.h"
#include "imgui/imgui.h"

DWORD keys[] = { VK_LMENU, VK_SHIFT, VK_CONTROL, VK_LBUTTON, VK_RBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
const char* keyItems[] = { "LAlt", "LShift", "LControl", "LMouse", "RMouse", "Mouse4", "Mouse5" };

inline namespace Configuration
{
	class Settings : public Singleton<Settings>
	{
	public:

		const char* BoxTypes[2] = { "Full Box","Cornered Box" };
		const char* LineTypes[3] = { "Bottom To Enemy","Top To Enemy","Crosshair To Enemy" };

		DWORD MENUkeys[6] = { VK_INSERT, VK_HOME, VK_PRIOR, VK_NEXT, VK_END, VK_RSHIFT };
		const char* keyMENU[6] = { "INSERT", "HOME", "PageUP", "PageDown", "END", "RShift" };
		int MENUKey = 0;

		const char* BoneTypes[2] = { "Head", "Body" };
		int boneType = 0;

		bool b_MenuShow = true;


		bool b_Visual = true;
		bool b_EspBox = false;
		bool b_EspSkeleton = true;
		bool b_EspLine = true;
		bool b_EspDistance = true;
		bool b_EspHealth  = true;
		bool b_EspName = false;

		bool crosshair = true;

		bool b_Aimbot = true;
		bool b_AimbotFOV = true;
		bool b_AimbotSmooth = false;

		bool debug_b = false;

		bool b_EspHealthHP = false;

		bool ignoreteam = true;

		bool allitems = false;

		bool Minsk = false;
		bool Pickup= false;
		bool Jeep = false;
		bool LUVA1 = false;
		bool M939 = false;
		bool HX60 = false;
		bool Ural375D = false;
		bool Ural4320 = false;
		bool Kamaz = false;
		bool MATV = false;
		bool Tigr = false;
		bool TAPV = false;
		bool BRDM2 = false;
		bool LPPV = false;
		bool BTR80 = false;
		bool BTR82 = false;
		bool LAV6 = false;
		bool LAV3 = false;
		bool M113A3 = false;
		bool M1126 = false;
		bool FV432 = false;
		bool FV107 = false;
		bool MTLB = false;
		bool BMP1 = false;
		bool BMP2 = false;
		bool M2A3 = false;
		bool FV510 = false;
		bool FV4034 = false;
		bool MSVS = false;
		bool M1A2 = false;
		bool T62 = false;
		bool T72B3 = false;
		bool Mi8 = false;
		bool UH60M = false;
		bool SA330 = false;

		bool hub = false;
		bool mine = false;

		ImColor exit_color = ImColor(128, 255, 128); 
		ImColor exitname_color = ImColor(128, 255, 128);
		ImColor color_stash = ImColor(128, 0, 128); // Blue color by default
		ImColor spaceexit_color = ImColor(128, 128, 255); // Blue color by default
		ImColor mine_color = ImColor(255, 255, 0); // Yellow color by default
		ImColor rarechest_color = ImColor(255, 128, 0); // Orange color by default
		ImColor medicchest_color = ImColor(128, 0, 128); // Purple color by default
		ImColor dead_color = ImColor(0, 255, 255); // Cyan color by default
		ImColor ship_color = ImColor(255, 128, 0); 
		ImColor shipname_color = ImColor(255, 255, 255); // White color by default
		ImColor shipbot_color = ImColor(128, 255, 255); 
		ImColor allitems_color = ImColor(255, 255, 255); // Black color by default

		ImColor quest_objects_color = ImColor(255, 100, 50);  // Пример цвета (оранжевый)
		ImColor item_color = ImColor(50, 150, 255);  // Пример цвета (голубой)

		ImColor rare_chest_color = ImColor(255, 128, 0); // Orange color by default
		ImColor common_chest_color = ImColor(255, 0, 255); // Magenta color by default
		ImColor quest_color = ImColor(0, 255, 0); // Green color by default
		ImColor large_safe_color = ImColor(0, 128, 0); // Dark green color by default
		ImColor briefcase_color = ImColor(0, 0, 255); // Blue color by default
		ImColor industrial_chest_color = ImColor(128, 0, 128); // Purple color by default
		ImColor prison_switch_color = ImColor(255, 0, 0); // Red color by default
		ImColor capsule_color = ImColor(255, 128, 128); // Black color by default
		ImColor capsulehole_color = ImColor(255, 255, 255); // White color by default
		ImColor mine_actor_color = ImColor(128, 128, 128); // Gray color by default

		ImColor color14 = ImColor(255, 0, 0);        // Red
		ImColor color15 = ImColor(0, 255, 0);        // Green
		ImColor color16 = ImColor(0, 0, 255);        // Blue
		ImColor color17 = ImColor(255, 255, 0);      // Yellow
		ImColor color18 = ImColor(255, 0, 255);      // Magenta
		ImColor color19 = ImColor(0, 255, 255);      // Cyan
		ImColor color20 = ImColor(128, 128, 0);     // Olive
		ImColor color21 = ImColor(128, 0, 128);     // Purple
		ImColor color22 = ImColor(0, 128, 128);     // Teal
		ImColor color23 = ImColor(128, 128, 128);   // Gray
		ImColor color24 = ImColor(192, 192, 192);   // Silver
		ImColor color25 = ImColor(255, 140, 0);     // Dark Orange
		ImColor color26 = ImColor(0, 128, 0);       // Dark Green


		ImColor VisibleColor = ImColor(255.f / 255, 0.f, 0.f);
		float fl_VisibleColor[3] = { 0.f,255.f / 255,0.f };  //

		ImColor BotColor = ImColor(0.f / 255, 255.f, 0.f);
		float fl_BotColor[3] = { 0.f,255.f / 255,0.f };  //

		ImColor InvisibleColor = ImColor(0.f, 255.f / 255, 0.f);
		float fl_InvisibleColor[3] = { 255.f / 255,0.f,0.f };  //

		ImColor FovColor = ImColor(255.f / 255, 255.f, 255.f);
		float fl_FovColor[3] = { 255.f / 255,255.f,255.f };  //

		int BoxType = 1;
		int LineType = 0;
		int tab_index = 0;
		int AimKey = 0;

		float font_size = 14.0f;

		float enemyfont_size = 14.0f;

		bool guihp = true;

		bool showvehicle = false;

		int Smoothing = 5.0f;
		int AimbotFOV = 230;
		int max_distance = 1500;
		int max_distanceAIM = 1000;
		int itemdistance = 1000;
	
	};
#define CFG Configuration::Settings::Get()
}
bool GetAimKey()
{
	return GetAsyncKeyState(keys[CFG.AimKey]);
}


