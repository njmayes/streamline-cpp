#include "pch.h"
#include "Widgets.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Types/StaticString.h"

namespace slc {

	void Widgets::NewLine()
	{
		ImGui::NewLine();
	}

	void Widgets::SameLine(float xOffset, float spacing)
	{
		ImGui::SameLine(xOffset, spacing);
	}

	void Widgets::Separator()
	{
		ImGui::Separator();
	}

	void Widgets::Spacing()
	{
		ImGui::Spacing();
	}

	void Widgets::Disable(bool disable)
	{
		ImGui::BeginDisabled(disable);
	}

	void Widgets::EndDisable()
	{
		ImGui::EndDisabled();
	}

	void Widgets::SetXPosition(float pos)
	{
		ImGui::SetCursorPosX(pos);
	}

	void Widgets::SetYPosition(float pos)
	{
		ImGui::SetCursorPosY(pos);
	}

	void Widgets::BeginDockspace()
	{
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = minWinSizeX;
	}

	void Widgets::EndDockspace()
	{
		ImGui::End();
	}

	bool Widgets::BeginWindow(std::string_view heading, bool* open, ImGuiWindowFlags flags)
	{
		return ImGui::Begin(heading.data(), open, flags);
	}

	void Widgets::EndWindow()
	{
		ImGui::End();
	}

	void Widgets::BeginColumns(int count, bool border)
	{
		ImGui::Columns(count, 0, border);
	}

	void Widgets::NextColumn()
	{
		ImGui::NextColumn();
	}

	void Widgets::EndColumns()
	{
		ImGui::Columns(1);
	}

	void Widgets::BeginChildInternal(std::string_view strID, const ImVec2& size, bool border)
	{
		ImGui::BeginChild(strID.data(), size, border);
	}

	void Widgets::EndChild()
	{
		ImGui::EndChild();
	}

	void Widgets::BeginGroup()
	{
		ImGui::BeginGroup();
	}

	void Widgets::EndGroup()
	{
		ImGui::EndGroup();
	}

	void Widgets::TreeNode(void* id, std::string_view text, bool selected, Action<> whileOpen)
	{
		TreeNodeInternal(id, text, selected, ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth, whileOpen);
	}

	bool Widgets::TreeNodeEx(void* id, std::string_view text, ImGuiTreeNodeFlags flags)
	{
		return ImGui::TreeNodeEx(id, flags, "%s", text.data());
	}

	void Widgets::Selectable(std::string_view label, bool selected, Action<> action)
	{
		if (ImGui::Selectable(label.data(), selected))
			action();
	}

	void Widgets::TreeNodeInternal(void* id, std::string_view text, bool selected, ImGuiTreeNodeFlags flags, Action<> whileOpen)
	{
		flags |= selected ? ImGuiTreeNodeFlags_Selected : 0;
		if (ImGui::TreeNodeEx((void*)&id, flags, "%s", text.data()))
		{
			whileOpen();
			ImGui::TreePop();
		}
	}

	void Widgets::BeginMenuBar()
	{
		sCurrentMenuBar = std::make_unique<UI::MenuBar>();
	}

	void Widgets::AddMenuBarHeading(std::string_view heading)
	{
		sCurrentMenuBar->addHeading(heading);
	}

	void Widgets::AddMenuBarItem(std::string_view heading, Action<> action)
	{
		sCurrentMenuBar->addMenuItemAction(heading, "", action);
	}

	void Widgets::AddMenuBarItem(std::string_view heading, std::string_view shortcut, Action<> action)
	{
		sCurrentMenuBar->addMenuItemAction(heading, shortcut, action);
	}

	void Widgets::AddMenuBarItem(std::string_view heading, bool& displayed)
	{
		sCurrentMenuBar->addMenuItemSwitch(heading, "", displayed);
	}

	void Widgets::AddMenuBarSeparator()
	{
		sCurrentMenuBar->addSeparator();
	}

	void Widgets::EndMenuBar()
	{
		sCurrentMenuBar.reset();
	}

	void Widgets::OpenPopup(std::string_view popupName)
	{
		ImGui::OpenPopup(popupName.data());
	}

	void Widgets::BeginPopup(std::string_view popupName)
	{
		sCurrentPopup = std::make_unique<UI::PopUp>(popupName);
	}

	void Widgets::AddPopupItem(std::string_view heading, Action<> action)
	{
		sCurrentPopup->addPopUpItem(heading, action);
	}

	void Widgets::EndPopup()
	{
		sCurrentPopup.reset();
	}

	void Widgets::BeginContextPopup()
	{
		sCurrentPopupCtx = std::make_unique<UI::PopUpContext>();
	}

	void Widgets::AddContextItem(std::string_view heading, Action<> action)
	{
		sCurrentPopupCtx->addPopUpItem(heading, action);
	}

	void Widgets::EndContextPopup()
	{
		sCurrentPopupCtx.reset();
	}

	void Widgets::Label(std::string_view text)
	{
		if (text.empty())
		{
			ImGui::Text("...");
			return;
		}

		ImGui::TextUnformatted(text.data());
	}

	void Widgets::LabelWrapped(std::string_view text)
	{
		if (text.empty())
		{
			ImGui::TextWrapped("...");
			return;
		}
		ImGui::TextWrapped("%s", text.data());
	}

	void Widgets::StringEdit(std::string_view label, std::string& field)
	{
		StaticString<256> stringEditBuffer(field);
		if (ImGui::InputText(label.data(), stringEditBuffer, stringEditBuffer.length()))
			field = stringEditBuffer.toString();
	}

	void Widgets::PathEdit(std::string_view label, fs::path& field)
	{
		StaticString<256> stringEditBuffer(field.string());
		if (ImGui::InputText(label.data(), stringEditBuffer, stringEditBuffer.length()))
			field = stringEditBuffer.toString();
	}

	void Widgets::DragDropSourceInternal(std::string_view strID, Action<> createPayload)
	{
		if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			return;

		createPayload();
		ImGui::SetDragDropPayload(strID.data(), sCurrentPayload->data(), sCurrentPayload->size());
		ImGui::EndDragDropSource();
	}

	void* Widgets::DragDropTargetInternal(std::string_view strID)
	{
		if (!ImGui::BeginDragDropTarget())
			return nullptr;

		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(strID.data());
		ImGui::EndDragDropTarget();

		if (!payload)
			return nullptr;
		return payload->Data;
	}

	void Widgets::OnWidgetSelected(Action<> action)
	{
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			action();
	}

	void Widgets::OnWidgetHovered(Action<> action, Action<> elseAction)
	{
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
			action();
		else if (elseAction)
			elseAction();
	}

	void Widgets::Checkbox(std::string_view label, bool& value, Action<> action)
	{
		if (ImGui::Checkbox(label.data(), &value) && action)
			action();
	}

	void Widgets::Button(std::string_view label, Action<> action)
	{
		if (ImGui::Button(label.data()) && action)
			action();
	}

	void Widgets::ButtonInternal(std::string_view label, const ImVec2& size, Action<> action)
	{
		if (ImGui::Button(label.data(), size) && action)
			action();
	}

	void Widgets::FloatEdit(std::string_view label, float& field, float speed, float mix, float max)
	{
		ImGui::DragFloat(label.data(), &field);
	}

	void Widgets::FloatEdit(std::string_view label, float field, Action<float> onEdit, float speed, float mix, float max)
	{
		if (ImGui::DragFloat(label.data(), &field))
			onEdit(field);
	}

	void Widgets::DoubleEdit(std::string_view label, double field, Action<double> onEdit, float speed, float mix, float max)
	{
		float tmp = (float)field;
		if (ImGui::DragFloat(label.data(), &tmp))
		{
			field = (double)tmp;
			onEdit(field);
		}
	}

	void Widgets::DoubleEdit(std::string_view label, double& field, float speed, float mix, float max)
	{
		float tmp = (float)field;
		ImGui::DragFloat(label.data(), &tmp);
		field = (double)tmp;
	}

	void Widgets::Vector2EditInternal(std::string_view label, ImVec2& values, float resetVal, float colWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, colWidth);
		ImGui::TextUnformatted(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void Widgets::Vector3EditInternal(std::string_view label, ImVec3& values, float resetVal, float colWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, colWidth);
		ImGui::TextUnformatted(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void Widgets::Vector4EditInternal(std::string_view label, ImVec4& values, float resetVal, float colWidth)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, colWidth);
		ImGui::TextUnformatted(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("W", buttonSize))
			values.w = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void Widgets::Vector2EditInternal(std::string_view label, ImVec2 values, Action<const ImVec2&> onEdit, float resetVal, float colWidth)
	{
		ImVec2 tmp = values;

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, colWidth);
		ImGui::TextUnformatted(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(2, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		if (tmp.x != values.x || tmp.y != values.y)
			onEdit(values);
	}

	void Widgets::Vector3EditInternal(std::string_view label, ImVec3 values, Action<const ImVec3&> onEdit, float resetVal, float colWidth)
	{
		ImVec3 tmp = values;

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, colWidth);
		ImGui::TextUnformatted(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		if (tmp.x != values.x || tmp.y != values.y || tmp.z != values.z)
			onEdit(values);
	}

	void Widgets::Vector4EditInternal(std::string_view label, ImVec4 values, Action<const ImVec4&> onEdit, float resetVal, float colWidth)
	{
		ImVec4 tmp = values;

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, colWidth);
		ImGui::TextUnformatted(label.data());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("W", buttonSize))
			values.w = resetVal;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##W", &values.w, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		if (tmp.x != values.x || tmp.y != values.y || tmp.z != values.z || tmp.w != values.w)
			onEdit(values);
	}

	void Widgets::ColourEditInternal(std::string_view label, ImVec4& colour)
	{
		ImGui::ColorEdit4(label.data(), &colour.x);
	}

	void Widgets::ImageInternal(ImTextureID image, const ImVec2& size, float rotation, const ImVec2& uv0, const ImVec2& uv1)
	{
		ImGui::Image(image, size, uv0, uv1);
	}

	void Widgets::ImageButtonInternal(ImTextureID image, const ImVec2& size, Action<> action, const ImVec2& uv0, const ImVec2& uv1, int padding)
	{
		if (ImGui::ImageButton(image, size, uv0, uv1, padding) && action)
			action();
	}

	int64_t Widgets::ScalarEdit(std::string_view label, int64_t field)
	{
		int width = (int)field;
		ImGui::InputInt(label.data(), &width);
		return (int64_t)width;
	}

	void Widgets::ScalarEdit(std::string_view label, int64_t field, Action<int64_t> onEdit)
	{
		int val = (int)field;
		ImGui::InputInt(label.data(), &val);
		onEdit((int64_t)val);
	}

	uint64_t Widgets::UScalarEdit(std::string_view label, uint64_t field)
	{
		int val = (int)field;
		ImGui::InputInt(label.data(), &val);
		if (val < 0)
			val = 0;
		return (uint64_t)val;
	}

	void Widgets::UScalarEdit(std::string_view label, uint64_t field, Action<uint64_t> onEdit)
	{
		int val = (int)field;
		ImGui::InputInt(label.data(), &val);
		if (val < 0)
			val = 0;
		onEdit((uint64_t)val);
	}

	bool Widgets::BeginCombo(std::string_view label, std::string_view preview)
	{
		return ImGui::BeginCombo(label.data(), preview.data());
	}

	bool Widgets::ComboboxEntry(std::string_view preview, const IComboEntry* entry)
	{
		bool selectionChange = false;

		bool isSelected = entry->key == preview;
		if (ImGui::Selectable(entry->key.data(), isSelected))
			selectionChange = true;

		if (isSelected)
			ImGui::SetItemDefaultFocus();

		return selectionChange;
	}

	void Widgets::EndCombo()
	{
		ImGui::EndCombo();
	}
}