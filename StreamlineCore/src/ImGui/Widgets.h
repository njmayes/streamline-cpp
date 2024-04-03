#pragma once

#include "Utils.h"

#include "Widgets/Combobox.h"
#include "Widgets/MenuBar.h"
#include "Widgets/PopUp.h"
#include "Widgets/Payload.h"

#include "Containers/Span.h"

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

		static void TreeNode(void* id, std::string_view text, bool selected, Action<> whileOpen);
		static bool TreeNodeEx(void* id, std::string_view text, ImGuiTreeNodeFlags flags);

		static void Selectable(std::string_view label, bool selected, Action<> action);

		static void BeginMenuBar();
		static void AddMenuBarHeading(std::string_view heading);
		static void AddMenuBarItem(std::string_view heading, Action<> action);
		static void AddMenuBarItem(std::string_view heading, std::string_view shortcut, Action<> action);
		static void AddMenuBarItem(std::string_view heading, bool& displayed);
		static void AddMenuBarSeparator();
		static void EndMenuBar();

		static void OpenPopup(std::string_view popupName);
		static void BeginPopup(std::string_view popupName);
		static void AddPopupItem(std::string_view heading, Action<> action);
		static void EndPopup();

		static void BeginContextPopup();
		static void AddContextItem(std::string_view heading, Action<> action);
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
		template<std::signed_integral T>
		static void IntEdit(std::string_view label, T field, Action<T> onEdit)
		{
			int64_t result = ScalarEdit(label, field);
			if (result < Limits<T>::Min)
				result = Limits<T>::Min;
			else if (result > Limits<T>::Max)
				result = Limits<T>::Max;

			field = (T)result;
			onEdit(field);
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
		template<std::unsigned_integral T>
		static void UIntEdit(std::string_view label, T field, Action<T> onEdit)
		{
			uint64_t result = UScalarEdit(label, field);
			if (result < Limits<T>::Min)
				result = Limits<T>::Min;
			else if (result > Limits<T>::Max)
				result = Limits<T>::Max;

			field = (T)result;
			onEdit(field);
		}

		static void FloatEdit(std::string_view label, float& field, float speed = 1.0f, float mix = 0.0f, float max = 0.0f);
		static void FloatEdit(std::string_view label, float field, Action<float> onEdit, float speed = 1.0f, float mix = 0.0f, float max = 0.0f);
		static void DoubleEdit(std::string_view label, double& field, float speed = 1.0f, float mix = 0.0f, float max = 0.0f);
		static void DoubleEdit(std::string_view label, double field, Action<double> onEdit, float speed = 1.0f, float mix = 0.0f, float max = 0.0f);

		template<VecSized<ImVec2> T>
		static void Vector2Edit(std::string_view label, T& values, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			ImVec2 imValues = Utils::ToImVec<ImVec2>(values);
			Vector2EditInternal(label, imValues, resetVal, colWidth);
			values = Utils::FromImVec<T>(imValues);
		}
		template<VecSized<ImVec3> T>
		static void Vector3Edit(std::string_view label, T& values, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			ImVec3 imValues = Utils::ToImVec<ImVec3>(values);
			Vector3EditInternal(label, imValues, resetVal, colWidth);
			values = Utils::FromImVec<T>(imValues);
		}
		template<VecSized<ImVec4> T>
		static void Vector4Edit(std::string_view label, T& values, float resetVal = 0.0f, float colWidth = 100.0f)
		{
			ImVec4 imValues = Utils::ToImVec<ImVec4>(values);
			Vector4EditInternal(label, imValues, resetVal, colWidth);
			values = Utils::FromImVec<T>(imValues);
		}
		template<VecSized<ImVec2> T>
		static void Vector2Edit(std::string_view label, T values, Action<const ImVec2&> onEdit, float resetVal = 0.0f, float colWidth = 100.0f) { Vector2EditInternal(label, Utils::ToImVec<ImVec2>(values), onEdit, resetVal, colWidth); }
		template<VecSized<ImVec3> T>
		static void Vector3Edit(std::string_view label, T values, Action<const ImVec3&> onEdit, float resetVal = 0.0f, float colWidth = 100.0f) { Vector3EditInternal(label, Utils::ToImVec<ImVec3>(values), onEdit, resetVal, colWidth); }
		template<VecSized<ImVec4> T>
		static void Vector4Edit(std::string_view label, T values, Action<const ImVec4&> onEdit, float resetVal = 0.0f, float colWidth = 100.0f) { Vector4EditInternal(label, Utils::ToImVec<ImVec4>(values), onEdit, resetVal, colWidth); }

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

		template<VecSized<ImVec2> T>
		static void ImageButton(uintptr_t image, const T& size, Action<> action = {}) { ImageButtonInternal((ImTextureID)image, Utils::ToImVec<ImVec2>(size), action); }
		template<VecSized<ImVec2> T>
		static void ImageButton(uintptr_t image, const T& size, Action<> action, const T& uv0, const T& uv1, int padding = -1)
		{
			ImageButtonInternal((ImTextureID)image, Utils::ToImVec<ImVec2>(size), action, Utils::ToImVec<ImVec2>(uv0), Utils::ToImVec<ImVec2>(uv1), padding);
		}

		template<typename T>
		static void AddDragDropSource(std::string_view strID, const T& data)
		{
			DragDropSourceInternal(strID, [&]() { sCurrentPayload = std::make_unique<Payload<T>>(data); });
		}
		template<typename T>
		static void AddDragDropTarget(std::string_view strID, Action<const T&> response)
		{
			void* imguiPayload = DragDropTargetInternal(strID);
			if (!imguiPayload)
				return;

			const T& payload = *(T*)imguiPayload;
			response(payload);
		}

		static void OnWidgetSelected(Action<> action);
		static void OnWidgetHovered(Action<> action, Action<> elseAction = {});

		static void Checkbox(std::string_view label, bool& value, Action<> action = {});

		static void Button(std::string_view label, Action<> action = {});
		template<typename T> requires VecSized<ImVec2, T>
		static void Button(const T& size, std::string_view label, Action<> action = {}) { ButtonInternal(label, Utils::ToImVec<ImVec2>(size), action); }

		template<typename T> requires std::copy_constructible<T>
		static void Combobox(std::string_view label, std::string_view preview, T& value, Span<const ComboEntry<T>> table)
		{
			if (!BeginCombo(label, preview))
				return;

			// Convert span of entries to view of base classes
			auto baseTable = table.Select<const IComboEntry*>([](const ComboEntry<T>& entry) { return dynamic_cast<const IComboEntry*>(&entry); });

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

		template<typename T>
		static void Combobox(std::string_view label, std::string_view preview, T value, Span<const ComboEntry<T>> table, Action<std::string_view, const T&> onSelection)
		{
			if (!BeginCombo(label, preview))
				return;

			// Convert span of entries to view of base classes
			auto baseTable = table.Select<const IComboEntry*>([](const ComboEntry<T>& entry) { return dynamic_cast<const IComboEntry*>(&entry); });

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
		static void BeginChildInternal(std::string_view strID, const ImVec2& size = { 0.0f, 0.0f }, bool border = true);

		static void ButtonInternal(std::string_view label, const ImVec2& size, Action<> action = {});

		static int64_t ScalarEdit(std::string_view label, int64_t field);
		static void ScalarEdit(std::string_view label, int64_t field, Action<int64_t> onEdit);
		static uint64_t UScalarEdit(std::string_view label, uint64_t field);
		static void UScalarEdit(std::string_view label, uint64_t field, Action<uint64_t> onEdit);

		static void Vector2EditInternal(std::string_view label, ImVec2& values, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector3EditInternal(std::string_view label, ImVec3& values, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector4EditInternal(std::string_view label, ImVec4& values, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector2EditInternal(std::string_view label, ImVec2 values, Action<const ImVec2&> onEdit, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector3EditInternal(std::string_view label, ImVec3 values, Action<const ImVec3&> onEdit, float resetVal = 0.0f, float colWidth = 100.0f);
		static void Vector4EditInternal(std::string_view label, ImVec4 values, Action<const ImVec4&> onEdit, float resetVal = 0.0f, float colWidth = 100.0f);

		static void ColourEditInternal(std::string_view label, ImVec4& colour);

		static void TreeNodeInternal(void* id, std::string_view text, bool selected, ImGuiTreeNodeFlags flags, Action<> whileOpen);

		static void ImageInternal(ImTextureID image, const ImVec2& size, float rotation = 0.0f, const ImVec2& uv0 = { 0.0f, 0.0f }, const ImVec2& uv1 = { 1.0f, 1.0f });
		static void ImageButtonInternal(ImTextureID image, const ImVec2& size, Action<> action = {}, const ImVec2& uv0 = { 0.0f, 0.0f }, const ImVec2& uv1 = { 1.0f, 1.0f }, int padding = -1);

		static void DragDropSourceInternal(std::string_view strID, Action<> createPayload);
		static void* DragDropTargetInternal(std::string_view strID);

		static bool BeginCombo(std::string_view label, std::string_view preview);
		static bool ComboboxEntry(std::string_view preview, const IComboEntry* entry);
		static void EndCombo();

	private:
		inline static Impl<IPayload>	sCurrentPayload  = nullptr;
		inline static UI::MenuBar		sCurrentMenuBar;
		inline static UI::PopUp			sCurrentPopup;
		inline static UI::PopUpContext	sCurrentPopupCtx;
	};
}