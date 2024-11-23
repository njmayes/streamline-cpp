#pragma once

#include "Utils.h"

#include "Widgets/Combobox.h"
#include "Widgets/MenuBar.h"
#include "Widgets/PopUp.h"

namespace slc {

	template<typename T> requires VecSized<ImVec2, T>
	using GridFunction = std::function<void(uint32_t x, uint32_t y, const T&)>;

	class Widgets
	{
	public:
		static void NewLine();
		static void SameLine(float xOffset = 0.0f, float spacing = -1.0f);
		static void Separator();
		static void Spacing();

		static void Disable(bool disable = true);
		static void EndDisable();

		static void SetXPosition(float pos);
		static void SetYPosition(float pos);

		static void BeginDockspace();
		static void EndDockspace();

		static bool BeginWindow(std::string_view heading, bool* open = nullptr, ImGuiWindowFlags flags = 0);
		static void EndWindow();

		template<typename T> requires VecSized<ImVec2, T>&& requires (T a) { T(0, 0); }
		static void BeginChild(std::string_view strID, const T& size = T(0, 0), bool border = false) { BeginChildInternal(strID, Utils::ToImVec<ImVec2>(size), border); }
		static void BeginChild(std::string_view strID) { BeginChild<ImVec2>(strID); }
		static void EndChild();

		static void BeginGroup();
		static void EndGroup();

		template<typename T> requires VecSized<ImVec2, T>
		static void GridControl(const T& pos, const T& size, size_t width, size_t height, GridFunction<T> func)
		{
			ImGui::SetCursorPos(Utils::ToImVec<ImVec2>(pos));
			Widgets::GridControl<T>(size, width, height, func);
		}
		template<typename T> requires VecSized<ImVec2, T>
		static void GridControl(const T& size, size_t width, size_t height, GridFunction<T> func)
		{
			ImVec2 pos = ImGui::GetCursorPos();
			ImVec2 tileSize = { size.x / width, size.y / height };

			for (uint32_t y = 0; y < height; y++)
			{
				for (uint32_t x = 0; x < width; x++)
				{
					ImGui::SetCursorPosX(pos.x + ((float)x * tileSize.x));
					ImGui::SetCursorPosY(pos.y + ((float)y * tileSize.y));

					func(x, y, Utils::FromImVec<T>(tileSize));
				}
			}
		}

		static void BeginColumns(int count, bool border = false);
		static void NextColumn();
		static void EndColumns();

		static void TreeNode(void* id, std::string_view text, bool selected, IsAction auto&& whileOpen)
		{
			if (TreeNodeExInternal(id, text, selected))
			{
				whileOpen();
				TreePopInternal();
			}
		}
		static bool TreeNodeEx(void* id, std::string_view text, ImGuiTreeNodeFlags flags);

		static void Selectable(std::string_view label, bool selected, IsAction auto&& action)
		{
			if (SelectableInternal(label, selected))
			{
				action();
			}
		}

		static void BeginMenuBar();
		static void AddMenuBarHeading(std::string_view heading);
		static void AddMenuBarItem(std::string_view heading, Action<>&& action);
		static void AddMenuBarItem(std::string_view heading, std::string_view shortcut, Action<>&& action);
		static void AddMenuBarItem(std::string_view heading, bool& displayed);
		static void AddMenuBarSeparator();
		static void EndMenuBar();

		static void OpenPopup(std::string_view popupName);
		static void BeginPopup(std::string_view popupName);
		static void AddPopupItem(std::string_view heading, Action<>&& action);
		static void EndPopup();

		static void BeginContextPopup();
		static void AddContextItem(std::string_view heading, Action<>&& action);
		static void EndContextPopup();

		static void Label(std::string_view fmt);
		static void LabelWrapped(std::string_view fmt);

		static void StringEdit(std::string_view label, std::string& field);
		static void PathEdit(std::string_view label, std::filesystem::path& field);

		template<std::signed_integral T>
		static void IntEdit(std::string_view label, T& field)
		{
			int64_t result = ScalarEdit(label, field);
			if (result < Limits<T>::Min)
				result = Limits<T>::Min;
			else if (result > Limits<T>::Max)
				result = Limits<T>::Max;

			field = (T)result;
		}
		template<std::signed_integral T, typename Func> requires IsAction<T>
		static void IntEdit(std::string_view label, T field, Func&& onEdit)
		{
			int64_t result = ScalarEdit(label, field);
			if (result < Limits<T>::Min)
				result = Limits<T>::Min;
			else if (result > Limits<T>::Max)
				result = Limits<T>::Max;

			T resultVal = (T)result;
			if (field != resultVal)
				onEdit(resultVal);
		}

		template<std::unsigned_integral T>
		static void UIntEdit(std::string_view label, T& field)
		{
			uint64_t result = UScalarEdit(label, field);
			if (result < Limits<T>::Min)
				result = Limits<T>::Min;
			else if (result > Limits<T>::Max)
				result = Limits<T>::Max;

			field = (T)result;
		}
		template<std::unsigned_integral T, typename Func> requires IsAction<T>
		static void UIntEdit(std::string_view label, T field, Func&& onEdit)
		{
			uint64_t result = UScalarEdit(label, field);
			if (result < Limits<T>::Min)
				result = Limits<T>::Min;
			else if (result > Limits<T>::Max)
				result = Limits<T>::Max;

			T resultVal = (T)result;
			if (field != resultVal)
				onEdit(resultVal);
		}

		static void FloatEdit(std::string_view label, float& field, float speed = 1.0f, float min = 0.0f, float max = 0.0f)
		{
			field = FloatEditInternal(label, field, speed, min, max);
		}
		template<typename Func> requires IsAction<float>
		static void FloatEdit(std::string_view label, float field, Func&& onEdit, float speed = 1.0f, float min = 0.0f, float max = 0.0f)
		{
			float result = FloatEditInternal(label, field, speed, min, max);

			if (field != result)
				onEdit(result);
		}

		template<VecSized<ImVec2> T>
		static void Vector2Edit(std::string_view label, T& values, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			ImVec2 imValues = Utils::ToImVec<ImVec2>(values);
			Vector2EditInternalRef(label, imValues, resetVal, colWidth);
			values = Utils::FromImVec<T>(imValues);
		}
		template<VecSized<ImVec3> T>
		static void Vector3Edit(std::string_view label, T& values, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			ImVec3 imValues = Utils::ToImVec<ImVec3>(values);
			Vector3EditInternalRef(label, imValues, resetVal, colWidth);
			values = Utils::FromImVec<T>(imValues);
		}
		template<VecSized<ImVec4> T>
		static void Vector4Edit(std::string_view label, T& values, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			ImVec4 imValues = Utils::ToImVec<ImVec4>(values);
			Vector4EditInternalRef(label, imValues, resetVal, colWidth);
			values = Utils::FromImVec<T>(imValues);
		}
		template<VecSized<ImVec2> T, IsAction<const ImVec2&> Func>
		static void Vector2Edit(std::string_view label, T values, Func&& onEdit, float resetVal = 0.0f, float colWidth = 100.0f)
		{ 
			auto cmpVal = Utils::ToImVec<ImVec2>(values);
			auto newValues = Vector2EditInternal(label, cmpVal, resetVal, colWidth);
			if (newValues.x != cmpVal.x or newValues.y != cmpVal.y)
			{
				onEdit(Utils::FromImVec<T>(newValues));
			}
		}
		template<VecSized<ImVec3> T, IsAction<const ImVec3&> Func>
		static void Vector3Edit(std::string_view label, T values, Func&& onEdit, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			auto cmpVal = Utils::ToImVec<ImVec3>(values);
			auto newValues = Vector3EditInternal(label, cmpVal, resetVal, colWidth);
			if (newValues.x != cmpVal.x or newValues.y != cmpVal.y or newValues.z != cmpVal.z)
			{
				onEdit(Utils::FromImVec<T>(newValues));
			}
		}
		template<VecSized<ImVec4> T, IsAction<const ImVec4&> Func>
		static void Vector4Edit(std::string_view label, T values, Func&& onEdit, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			auto cmpVal = Utils::ToImVec<ImVec4>(values);
			auto newValues = Vector4EditInternal(label, cmpVal, resetVal, colWidth);
			if (newValues.x != cmpVal.x or newValues.y != cmpVal.y or newValues.z != cmpVal.z or newValues.w != cmpVal.w)
			{
				onEdit(Utils::FromImVec<T>(newValues));
			}
		}

		template<VecSized<ImVec4> T>
		static void ColourEdit(std::string_view label, T& colour)
		{
			ImVec4 imColour = Utils::ToImVec<ImVec4>(colour);
			ColourEditInternal(label, imColour);
			colour = Utils::FromImVec<T>(imColour);
		}

		template<VecSized<ImVec2> T>
		static void Image(uintptr_t image, const T& size, float rotation = 0.0f) { ImageInternal((ImTextureID)image, Utils::ToImVec<ImVec2>(size), rotation); }
		template<VecSized<ImVec2> T>
		static void Image(uintptr_t image, const T& size, float rotation, const T& uv0, const T& uv1)
		{
			ImageInternal((ImTextureID)image, Utils::ToImVec<ImVec2>(size), rotation, Utils::ToImVec<ImVec2>(uv0), Utils::ToImVec<ImVec2>(uv1));
		}

		template<VecSized<ImVec2> T, IsAction Func>
		static void ImageButton(uintptr_t image, const T& size, Func&& action = {}) 
		{ 
			if (ImageButtonInternal((ImTextureID)image, Utils::ToImVec<ImVec2>(size)) && action)
			{
				action();
			}

		}
		template<VecSized<ImVec2> T, IsAction Func>
		static void ImageButton(uintptr_t image, const T& size, Func&& action, const T& uv0, const T& uv1, int padding = -1)
		{
			if (ImageButtonInternal((ImTextureID)image, Utils::ToImVec<ImVec2>(size), Utils::ToImVec<ImVec2>(uv0), Utils::ToImVec<ImVec2>(uv1), padding))
			{
				action();
			}
		}

		template<typename T>
		static void AddDragDropSource(std::string_view strID, const T& data)
		{
			if (!BeginDragDropSourceInternal())
				return;

			EndDragDropSourceInternal(strID, &data, sizeof(T));
		}
		template<typename T, IsAction<const T&> Func>
		static void AddDragDropTarget(std::string_view strID, Func&& response)
		{
			void* imguiPayload = DragDropTargetInternal(strID);
			if (!imguiPayload)
				return;

			const T& payload = *(T*)imguiPayload;
			response(payload);
		}

		static void OnWidgetSelected(IsAction auto&& action)
		{
			if (Utils::ItemHovered() and Utils::IsLeftMouseDoubleClicked())
			{
				action();
			}
		}
		static void OnWidgetHovered(IsAction auto&& action, IsAction auto&& elseAction = {})
		{
			if (Utils::ItemHovered())
				action();
			else if (elseAction)
				elseAction();
		}

		static void Checkbox(std::string_view label, bool& value, IsAction auto&& action = {})
		{
			if (CheckboxInternal(label, value) and action)
			{
				action();
			}
		}

		template<IsAction Func>
		static void Button(std::string_view label, Func&& action = {})
		{
			if (ButtonInternal(label) && action)
			{
				action();
			}
		}
		template<typename T, IsAction Func> requires VecSized<ImVec2, T>
		static void Button(const T& size, std::string_view label, Func&& action = {}) 
		{ 
			if (ButtonInternal(label, Utils::ToImVec<ImVec2>(size)) && action)
			{
				action();
			}
		}

		template<typename T> requires std::copy_constructible<T>
		static void Combobox(std::string_view label, std::string_view preview, T& value, std::span<const ComboEntry<T>> table)
		{
			if (!BeginCombo(label, preview))
				return;

			// Convert std::span of entries to view of base classes
			auto baseTable = table | std::views::transform([](const ComboEntry<T>& entry) { return dynamic_cast<const IComboEntry*>(&entry); });

			const IComboEntry* comboEntry = nullptr;
			for (const IComboEntry* entry : baseTable)
			{
				if (Widgets::ComboboxEntry(preview, entry))
					comboEntry = entry;
			}

			EndCombo();

			if (!comboEntry)
				return;

			value = T(*(const T*)comboEntry->getVal());
		}

		template<typename T, typename Func> requires IsAction<Func, std::string_view, const T&>
		static void Combobox(std::string_view label, std::string_view preview, T value, std::span<const ComboEntry<T>> table, Func&& onSelection)
		{
			if (!BeginCombo(label, preview))
				return;

			// Convert std::span of entries to view of base classes
			auto baseTable = table | std::views::transform([](const ComboEntry<T>& entry) { return dynamic_cast<const IComboEntry*>(&entry); });

			const IComboEntry* comboEntry = nullptr;
			for (const IComboEntry* entry : baseTable)
			{
				if (Widgets::ComboboxEntry(preview, entry))
					comboEntry = entry;
			}

			EndCombo();

			if (!comboEntry)
				return;

			onSelection(comboEntry->key, *(const T*)comboEntry->getVal());
		}

	private:
		static bool TreeNodeExInternal(void* id, std::string_view text, bool selected);
		static void TreePopInternal();

		static bool SelectableInternal(std::string_view label, bool selected);

		static void BeginChildInternal(std::string_view strID, const ImVec2& size = { 0.0f, 0.0f }, bool border = true);

		static bool CheckboxInternal(std::string_view label, bool& value);
		static bool ButtonInternal(std::string_view label);
		static bool ButtonInternal(std::string_view label, const ImVec2& size);

		static int64_t ScalarEdit(std::string_view label, int64_t field);
		static uint64_t UScalarEdit(std::string_view label, uint64_t field);

		static float FloatEditInternal(std::string_view label, float field, float speed, float min, float max);

		static void Vector2EditInternalRef(std::string_view label, ImVec2& values, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector3EditInternalRef(std::string_view label, ImVec3& values, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector4EditInternalRef(std::string_view label, ImVec4& values, float resetVal = 0.0f, float colWidth = 100.0f);
		static ImVec2 Vector2EditInternal(std::string_view label, ImVec2 values, float resetVal = 0.0f, float colWidth = 100.0f);
		static ImVec3 Vector3EditInternal(std::string_view label, ImVec3 values, float resetVal = 0.0f, float colWidth = 100.0f);
		static ImVec4 Vector4EditInternal(std::string_view label, ImVec4 values, float resetVal = 0.0f, float colWidth = 100.0f);

		static void ColourEditInternal(std::string_view label, ImVec4& colour);

		static void ImageInternal(ImTextureID image, const ImVec2& size, float rotation = 0.0f, const ImVec2& uv0 = { 0.0f, 0.0f }, const ImVec2& uv1 = { 1.0f, 1.0f });
		static bool ImageButtonInternal(ImTextureID image, const ImVec2& size, const ImVec2& uv0 = { 0.0f, 0.0f }, const ImVec2& uv1 = { 1.0f, 1.0f }, int padding = -1);

		static bool BeginDragDropSourceInternal();
		static void EndDragDropSourceInternal(std::string_view strID, const void* data, size_t size);
		static void* DragDropTargetInternal(std::string_view strID);

		static bool BeginCombo(std::string_view label, std::string_view preview);
		static bool ComboboxEntry(std::string_view preview, const IComboEntry* entry);
		static void EndCombo();

	private:
		inline static UI::MenuBar		sCurrentMenuBar;
		inline static UI::PopUp			sCurrentPopup;
		inline static UI::PopUpContext	sCurrentPopupCtx;
	};
}