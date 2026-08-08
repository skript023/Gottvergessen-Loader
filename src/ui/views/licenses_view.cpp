#include "ui/views/licenses_view.hpp"
#include "ui/ui.hpp"

#include <imgui.h>

namespace gottvergessen
{
	void licenses_view::render_impl(ui* ui_instance)
	{
		ui::card_begin("MyLicensesCard", "MY LICENSES & SUBSCRIPTIONS", "Active cryptographic license keys issued to your account.");
		{
			if (ui::secondary_button(ICON_FA_SYNC "  Refresh Licenses", ImVec2(160, 32)))
			{
				ui::fetch_data_from_server();
			}

			ImGui::Spacing();

			if (ui_instance->m_user_licenses.empty())
			{
				ImGui::TextDisabled("No active licenses found for your account.");
			}
			else
			{
				size_t total_lics = ui_instance->m_user_licenses.size();
				float row_h = 36.0f;
				float table_h = row_h * (total_lics > 0 ? (total_lics + 1.2f) : 2.5f);
				if (table_h > 320.0f) table_h = 320.0f;

				ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(12.0f, 8.0f));
				if (ImGui::BeginTable("LicensesTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY, ImVec2(0.0f, table_h)))
				{
					ImGui::TableSetupColumn("Product / Module", ImGuiTableColumnFlags_WidthStretch, 0.28f);
					ImGui::TableSetupColumn("License Key", ImGuiTableColumnFlags_WidthFixed, 220.0f);
					ImGui::TableSetupColumn("Issued Date", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Expires", ImGuiTableColumnFlags_WidthFixed, 150.0f);
					ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
					ImGui::TableHeadersRow();

					int lic_idx = 0;
					for (const auto& lic : ui_instance->m_user_licenses)
					{
						ImGui::PushID(lic_idx++);
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", lic.product_name.c_str());

						ImGui::TableSetColumnIndex(1);
						ImGui::TextColored(ImVec4(0.23f, 0.51f, 0.96f, 1.0f), "%s", lic.license_key.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::TextDisabled("%s", lic.issued_at.empty() ? "-" : lic.issued_at.c_str());

						ImGui::TableSetColumnIndex(3);
						ImGui::Text("%s", lic.expiry_date.empty() ? "Lifetime" : lic.expiry_date.c_str());

						ImGui::TableSetColumnIndex(4);
						ui::badge(lic.status.c_str(), ImVec4(0.16f, 0.72f, 0.53f, 0.2f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
						ImGui::PopID();
					}
					ImGui::EndTable();
				}
				ImGui::PopStyleVar();
			}
		}
		ui::card_end();
	}
}
