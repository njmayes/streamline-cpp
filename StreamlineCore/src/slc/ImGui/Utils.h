#pragma once

#include "imgui.h"

#include "slc/Common/Base.h"
#include "slc/IO/KeyCodes.h"
#include "slc/IO/MouseCodes.h"

namespace slc {

	template<typename T>
	concept ImVecType = std::same_as<T, ImVec2> || std::same_as<T, ImVec3> || std::same_as<T, ImVec4>;

	template<typename Vec, typename T>
	concept VecSized = std::is_trivially_copyable_v<T> && sizeof(Vec) == sizeof(T);


	class Utils
	{
	public:
		template<ImVecType Vec, typename T> requires VecSized<Vec, T>
		static Vec ToImVec(const T& var) { return std::bit_cast<Vec>(var); }

		template<typename T, ImVecType Vec> requires VecSized<Vec, T>
		static T FromImVec(const Vec& var) { return std::bit_cast<T>(var); }

		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseDown(MouseCode button);
		static bool IsMouseReleased(MouseCode button);

		template<typename T> requires VecSized<ImVec2, T>
		static T MousePos() { return FromImVec<T>(MousePos()); }
		static ImVec2 MousePos();

		template<typename T> requires VecSized<ImVec2, T>
		static T CursorPos() { return FromImVec<T>(CursorPosInternal()); }
		static void SetCursorPos(float x, float y);
		static void SetCursorPosX(float pos);
		static void SetCursorPosY(float pos);

		template<typename T> requires VecSized<ImVec2, T>
		static T WindowPos() { return FromImVec<T>(WindowPos()); }
		static ImVec2 WindowPos();

		template<typename T> requires VecSized<ImVec2, T>
		static void SetNextWindowSize(const T& size, ImGuiCond cond = ImGuiCond_FirstUseEver) { SetNextWindowSizeInternal(ToImVec<ImVec2>(size), cond); }
		template<typename T> requires VecSized<ImVec2, T>
		static void SetNextWindowPos(const T& size, const T& pivot = T(0, 0), ImGuiCond cond = ImGuiCond_FirstUseEver) { SetNextWindowPosInternal(ToImVec<ImVec2>(size), ToImVec<ImVec2>(pivot), cond); }

		template<typename T> requires VecSized<ImVec4, T>
		static void SetButtonColour(const T& colour) { SetButtonColourInternal(ToImVec<ImVec4>(colour)); }
		static void SetButtonDefaults();

		template<typename T> requires VecSized<ImVec2, T>
		static T GetMainWindowCentre() { return FromImVec<T>(GetMainWindowCentreInternal()); }

		static void SetWindowMoveFromTitleBar(bool titleBarOnly = true);

		template<typename T> requires VecSized<ImVec2, T>
		static T AvailableRegion() { return FromImVec<T>(AvailableRegion()); }
		static ImVec2 AvailableRegion();
		template<typename T> requires VecSized<ImVec2, T>
		static T AvailableRegionMax() { return FromImVec<T>(AvailableRegionMax()); }
		static ImVec2 AvailableRegionMax();
		template<typename T> requires VecSized<ImVec2, T>
		static T AvailableRegionMin() { return FromImVec<T>(AvailableRegionMin()); }
		static ImVec2 AvailableRegionMin();

		static float FontSize();
		template<typename T> requires VecSized<ImVec2, T>
		static T FramePadding() { return FromImVec<T>(FramePadding()); }
		static ImVec2 FramePadding();
		static float FrameHeightWithSpacing();

		static float LineHeight();

		static float WindowWidth();
		static float WindowHeight();

		static bool ItemFocused();
		static bool ItemHovered();

		static bool WindowFocused();
		static bool WindowHovered();

		static void PushItemWidth(float width);
		static void PopItemWidth();

		static void PushID(std::string_view strID);
		static void PopID();

		static void PushStyle(ImGuiStyleVar flags, float var);
		static void PushStyle(ImGuiStyleVar flags, const ImVec2& var);
		static void PopStyle(int count = 1);

		static void PushStyleColour(ImGuiCol flags, const ImVec4& var);
		static void PopStyleColour(int count = 1);

		static bool IsLeftMouseClicked();
		static bool IsLeftMouseDoubleClicked();

	private:
		static ImVec2 CursorPosInternal();
		static ImVec2 GetMainWindowCentreInternal();

		static void SetNextWindowSizeInternal(const ImVec2& size, ImGuiCond cond = ImGuiCond_FirstUseEver);
		static void SetNextWindowPosInternal(const ImVec2& size, const ImVec2& pivot = ImVec2(0, 0), ImGuiCond cond = ImGuiCond_FirstUseEver);

		static void SetButtonColourInternal(const ImVec4& colour);
	};
}