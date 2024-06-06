#include "custom_elements.h"
#include "imgui/imgui_internal.h"


using namespace ImGui;

bool custom_elements::Checkbox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float w = GetWindowWidth() - 400;
    const float square_sz = 17;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect frame_bb(pos + ImVec2(w - square_sz - 13, 0), window->DC.CursorPos + ImVec2(w, square_sz - 1));
    const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
    {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
        return false;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(frame_bb, id, &hovered, &held);
    if (pressed)
    {
        *v = !(*v);
        MarkItemEdited(id);
    }

    static std::unordered_map <ImGuiID, float> animate;
    auto it_anim = animate.find(id);
    if (it_anim == animate.end())
    {
        animate.insert({ id, { 0.0f } });
        it_anim = animate.find(id);
    }

    it_anim->second = ImLerp(it_anim->second, *v ? 1.0f : 0.0f, 0.12f * (1.0f - ImGui::GetIO().DeltaTime));

    RenderNavHighlight(total_bb, id);

    RenderFrame(frame_bb.Min, frame_bb.Max, ImColor(141, 143, 140), false, 9.0f);
    RenderFrame(frame_bb.Min, frame_bb.Max, ImColor(0.0902f, 0.9215f, 0.2274f, it_anim->second), false, 9.0f);


    window->DrawList->AddCircleFilled(ImVec2(frame_bb.Min.x + 8 + 14 * it_anim->second, frame_bb.Min.y + 8), 5.0f, ImColor(1.0f, 1.0f, 1.0f), 30);

    if (label_size.x > 0.0f)
        RenderText(total_bb.Min, label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
    return pressed;
}

bool custom_elements::SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = GetWindowWidth() - 250;

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + 16));
    const ImRect frame_bb(total_bb.Min + ImVec2(0, label_size.y + 10), total_bb.Max);

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    // Tabbing or CTRL-clicking on Slider turns it into an input box
    const bool hovered = ItemHoverable(frame_bb, id);
    const bool clicked = (hovered && g.IO.MouseClicked[0]);
    if (clicked)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

    window->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, ImColor(141, 143, 140), 5.0f);
    window->DrawList->AddRectFilled(frame_bb.Min, ImVec2(grab_bb.Min.x + 3, frame_bb.Max.y), ImColor(23, 235, 58), 5.0f);
    window->DrawList->AddCircleFilled(ImVec2(grab_bb.Min.x + 7, grab_bb.Min.y + 1), 6.0f, ImColor(1.0f, 1.0f, 1.0f), 30);

    RenderText(total_bb.Min, label);

    RenderTextClipped(total_bb.Min, total_bb.Max, value_buf, value_buf_end, NULL, ImVec2(1.f, 0.f));

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
    return value_changed;
}

bool custom_elements::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags);
}

bool custom_elements::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags);
}

const char* keys[] = {
    "None",
    "Mouse 1",
    "Mouse 2",
    "CN",
    "Mouse 3",
    "Mouse 4",
    "Mouse 5",
    "-",
    "Back",
    "Tab",
    "-",
    "-",
    "CLR",
    "Enter",
    "-",
    "-",
    "Shift",
    "CTL",
    "Menu",
    "Pause",
    "Caps",
    "KAN",
    "-",
    "JUN",
    "FIN",
    "KAN",
    "-",
    "ESC",
    "CON",
    "NCO",
    "ACC",
    "MAD",
    "Space",
    "PGU",
    "PGD",
    "End",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "SEL",
    "PRI",
    "EXE",
    "PRI",
    "INS",
    "DEL",
    "HEL",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "WIN",
    "WIN",
    "APP",
    "-",
    "SLE",
    "Numpad 0",
    "Numpad 1",
    "Numpad 2",
    "Numpad 3",
    "Numpad 4",
    "Numpad 5",
    "Numpad 6",
    "Numpad 7",
    "Numpad 8",
    "Numpad 9",
    "MUL",
    "ADD",
    "SEP",
    "MIN",
    "Delete",
    "DIV",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "NUM",
    "SCR",
    "EQU",
    "MAS",
    "TOY",
    "OYA",
    "OYA",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "Shift",
    "Shift",
    "Ctrl",
    "Ctrl",
    "Alt",
    "Alt"
};

bool custom_elements::Keybind(const char* label, int* key, bool widget_label) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    ImVec2 mode_size;
    ImVec2 key_size;

    const ImGuiID id = window->GetID(label);
    const float size = GetWindowWidth() - 390;
    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(size, 30));
    ImRect mode_rect(window->DC.CursorPos + ImVec2(size - 0, 0), window->DC.CursorPos + ImVec2(size, 40));
    ImRect key_rect;

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect total_bb(pos, pos + ImVec2(270, 35));

    ImGui::ItemSize(rect, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    bool popup_open = ImGui::IsPopupOpen(id, ImGuiPopupFlags_None);
    bool hovered_rect = ItemHoverable(rect, id);

    bool value_changed = false;
    int k = *key;

    mode_rect.Min.x -= mode_size.x;
    key_rect = ImRect(mode_rect.Min - ImVec2(CalcItemWidth(), 0), ImVec2(mode_rect.Min.x - 1, mode_rect.Max.y));

    bool hovered_mode = ItemHoverable(mode_rect, id);

    char buf_display1[64] = "None";

    std::string active_key = "";
    active_key += keys[*key];

    if (*key != 0 && g.ActiveId != id) {
        strcpy_s(buf_display1, active_key.c_str());
    }
    else if (g.ActiveId == id) {
        strcpy_s(buf_display1, "-");
    }
    key_size = CalcTextSize(buf_display1, NULL, true);
    key_rect.Min.x -= key_size.x;

    bool hovered_key = ItemHoverable(key_rect, id);

    std::string nametwo = "keybind_state";
    nametwo += label;


    if (hovered_key && io.MouseClicked[0])
    {
        if (g.ActiveId != id) {
            // Start edition
            memset(io.MouseDown, 0, sizeof(io.MouseDown));
            memset(io.KeysDown, 0, sizeof(io.KeysDown));
            *key = 0;
        }
        ImGui::SetActiveID(id, window);
        ImGui::FocusWindow(window);
    }
    else if (io.MouseClicked[0]) {
        // Release focus when we click outside
        if (g.ActiveId == id)
            ImGui::ClearActiveID();
    }

    if (g.ActiveId == id) {
        for (auto i = 0; i < 5; i++) {
            if (io.MouseDown[i]) {
                switch (i) {
                case 0:
                    k = 0x01;
                    break;
                case 1:
                    k = 0x02;
                    break;
                case 2:
                    k = 0x04;
                    break;
                case 3:
                    k = 0x05;
                    break;
                case 4:
                    k = 0x06;
                    break;
                }
                value_changed = true;
                ImGui::ClearActiveID();
            }
        }
        if (!value_changed) {
            for (auto i = 0x08; i <= 0xA5; i++) {
                if (io.KeysDown[i]) {
                    k = i;
                    value_changed = true;
                    ImGui::ClearActiveID();
                }
            }
        }

        if (IsKeyPressedMap(ImGuiKey_Escape)) {
            *key = 0;
            ImGui::ClearActiveID();
        }
        else {
            *key = k;
        }
    }

    static std::map<ImGuiID, float> alpha_animation;
    auto it_alpha = alpha_animation.find(id);
    if (it_alpha == alpha_animation.end())
    {
        alpha_animation.insert({ id, 0.f });
        it_alpha = alpha_animation.find(id);
    }

    it_alpha->second = ImLerp(it_alpha->second, hovered_rect ? 0.2f : 0.f, g.IO.DeltaTime * 6.f);

    it_alpha->second *= style.Alpha;

    if (widget_label) window->DrawList->AddText(rect.Min + ImVec2(0, 4), ImColor(255, 255, 255), label);

    window->DrawList->AddRectFilled(key_rect.Min + ImVec2(CalcItemWidth() - 21, -1), key_rect.Max - ImVec2(2, 12), ImColor(0, 162, 250), style.FrameRounding);

    RenderTextClipped(key_rect.Min - ImVec2(60, 31), key_rect.Max + ImVec2(CalcItemWidth() + 38, 16), buf_display1, NULL, NULL, ImVec2(0.5f, 0.5f));

    SetCursorPos(GetCursorPos() + ImVec2(0, 8));

    return value_changed;
}