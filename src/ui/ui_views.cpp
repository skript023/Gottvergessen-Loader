#include "ui/ui_views.hpp"
#include "ui/ui.hpp"
#include "renderer.hpp"
#include "process/injection.hpp"
#include "api/user/user_authentication.hpp"
#include "thread_pool.hpp"

#include <imgui.h>
#include <format>
#include <iomanip>
#include <sstream>

namespace gottvergessen
{
	// Helper format IDR
	static std::string format_idr(double amount)
	{
		std::stringstream ss;
		ss << "Rp " << std::fixed << std::setprecision(0) << amount;
		return ss.str();
	}

	void ui_views::render_dashboard_overview(ui* ui_instance)
	{
		// Top KPI Statistics Grid (4 Cards across)
		ImGui::Columns(4, "KpiGrid", false);

		ui::stat_widget("Active Subscriptions", "2 Products", "+1 renewed this month", ICON_FA_KEY, ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
		ImGui::NextColumn();

		ui::stat_widget("Total Transactions", format_idr(398000.0).c_str(), "Completed (Settlement)", ICON_FA_CREDIT_CARD, ImVec4(0.23f, 0.51f, 0.96f, 1.0f));
		ImGui::NextColumn();

		ui::stat_widget("Core Engine Version", "v1.0.4", "Latest Build Online", ICON_FA_SHIELD_ALT, ImVec4(0.96f, 0.62f, 0.23f, 1.0f));
		ImGui::NextColumn();

		ui::stat_widget("Server Connection", "24 ms", "Frankfurt Node #1", ICON_FA_SERVER, ImVec4(0.20f, 0.80f, 0.40f, 1.0f));
		ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Spacing();
		ImGui::Spacing();

		// Two Column Layout: Left (Workflow Stepper & Quick Action), Right (Active Licenses Quick List)
		ImGui::Columns(2, "DashboardSplitArea", false);
		ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.62f);

		ui::card_begin("EllohimWorkflowOverviewCard", "ELLOHIM TRANSACTION WORKFLOW ALUR SYSTEM", "Sequential 3-Stage Order, Payment, License & Binary Pipeline");
		{
			const char* steps[] = { "1. Product & Order", "2. Payment & License", "3. Download & Inject" };
			int current_step_idx = 0;
			if (ui_instance->m_current_order.status == TransactionStatus::PendingPayment) current_step_idx = 1;
			else if (ui_instance->m_current_order.status == TransactionStatus::SettlementSuccess || ui_instance->m_binary_downloaded) current_step_idx = 2;

			ui::stepper_widget(steps, 3, current_step_idx);

			ImGui::Spacing();
			ImGui::TextWrapped("System Ellohim Loader terintegrasi secara otomatis dengan backend API Ellohim-Server. "
				"Lakukan pemesanan lisensi, lunasi via QRIS/Bank Transfer, dan unduh binary terenkripsi langsung dari menu dashboard.");

			ImGui::Spacing();
			if (ui::primary_button(ICON_FA_SHOPPING_CART "  Browse Catalog & Order Products", ImVec2(240, 38)))
			{
				ui_instance->m_active_tab = NavTab::ProductsCatalog;
			}
			ImGui::SameLine();
			if (ui::secondary_button(ICON_FA_ROCKET "  Open Binary Launcher", ImVec2(200, 38)))
			{
				ui_instance->m_active_tab = NavTab::BinaryDownload;
			}
		}
		ui::card_end();

		ImGui::NextColumn();

		ui::card_begin("ActiveLicensesQuickCard", "ACTIVE LICENSES", "Currently bound to dummy_gamer HWID");
		{
			if (ui_instance->m_user_licenses.empty())
			{
				ImGui::TextDisabled("No active licenses found.");
			}
			else
			{
				for (const auto& lic : ui_instance->m_user_licenses)
				{
					ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", lic.product_name.c_str());
					ImGui::SameLine(ImGui::GetWindowWidth() - 95.0f);
					ui::badge(lic.status.c_str(), ImVec4(0.16f, 0.72f, 0.53f, 0.2f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));

					ImGui::TextDisabled("Key: %s", lic.license_key.c_str());
					ImGui::TextDisabled("Expires: %s", lic.expiry_date.c_str());
					ImGui::Separator();
					ImGui::Spacing();
				}
			}
		}
		ui::card_end();

		ImGui::Columns(1);
	}

	void ui_views::render_products_catalog(ui* ui_instance)
	{
		ui::card_begin("CatalogHeaderCard", "PRODUCT & EXTENSION CATALOG", "Select modules to purchase. All transactions are processed instantly via Ellohim-Server.");
		{
			ImGui::Columns(3, "ProductsGrid", false);

			for (size_t i = 0; i < ui_instance->m_available_products.size(); ++i)
			{
				auto& item = ui_instance->m_available_products[i];
				char card_id[64];
				snprintf(card_id, sizeof(card_id), "ProdCard_%zu", i);

				ui::card_begin(card_id, item.name.c_str(), item.category.c_str());
				{
					ui::badge(item.version.c_str(), ImVec4(0.23f, 0.51f, 0.96f, 0.2f), ImVec4(0.40f, 0.70f, 1.0f, 1.0f));
					ImGui::Spacing();

					ImGui::TextWrapped("%s", item.description.c_str());
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.65f, 1.0f), "%s", format_idr(item.price).c_str());
					ImGui::Spacing();

					char chk_id[64];
					snprintf(chk_id, sizeof(chk_id), "Add to Cart##chk_%zu", i);
					ImGui::Checkbox(chk_id, &item.selected);
				}
				ui::card_end();

				ImGui::NextColumn();
			}

			ImGui::Columns(1);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ui::primary_button(ICON_FA_FILE_INVOICE "  Proceed to Order Checkout (Stage 1)", ImVec2(320, 42)))
			{
				ui_instance->m_active_tab = NavTab::CheckoutOrder;
			}
		}
		ui::card_end();
	}

	void ui_views::render_checkout_order(ui* ui_instance)
	{
		ui::card_begin("Stage1CheckoutCard", "STAGE 1: CHECKOUT & ORDER CREATION (POST /order)", "Review items in cart, apply discount promos, and generate pending order.");
		{
			const char* steps[] = { "1. Product & Order", "2. Payment & License", "3. Download & Inject" };
			ui::stepper_widget(steps, 3, 0);

			ImGui::Columns(2, "CheckoutSplit", false);
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.60f);

			// Cart items table
			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "Selected Order Items");
			ImGui::Spacing();

			if (ImGui::BeginTable("CartItemsTable", 4, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Product Name", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 110.0f);
				ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Subtotal", ImGuiTableColumnFlags_WidthFixed, 110.0f);
				ImGui::TableHeadersRow();

				double subtotal = 0.0;
				for (const auto& prod : ui_instance->m_available_products)
				{
					if (prod.selected)
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::Text("%s", prod.name.c_str());

						ImGui::TableSetColumnIndex(1);
						ImGui::TextDisabled("%s", prod.category.c_str());

						ImGui::TableSetColumnIndex(2);
						ImGui::Text("1");

						ImGui::TableSetColumnIndex(3);
						ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.65f, 1.0f), "%s", format_idr(prod.price).c_str());

						subtotal += prod.price;
					}
				}
				ImGui::EndTable();
			}

			ImGui::NextColumn();

			// Order summary calculation card
			ui::card_begin("OrderSummaryBox", "ORDER SUMMARY", "Voucher Promo & Total Billing");
			{
				double raw_subtotal = 0.0;
				for (const auto& prod : ui_instance->m_available_products)
				{
					if (prod.selected) raw_subtotal += prod.price;
				}

				// Apply promo logic
				double discount = 0.0;
				std::string promo_code = ui_instance->m_promo_input;
				if (promo_code == "ELLOHIMPROMO10")
				{
					discount = raw_subtotal * 0.10;
				}
				else if (promo_code == "GAMERHEMAT50K")
				{
					discount = 50000.0;
				}

				double total = std::max(0.0, raw_subtotal - discount);

				ImGui::Text("Voucher Promo Code:");
				ImGui::SetNextItemWidth(-1);
				ui::input_text("##PromoInput", "Enter voucher code", ui_instance->m_promo_input, sizeof(ui_instance->m_promo_input));
				
				ImGui::TextDisabled("Available promos: ELLOHIMPROMO10 (-10%%), GAMERHEMAT50K (-50k)");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::Text("Subtotal: %s", format_idr(raw_subtotal).c_str());
				ImGui::TextColored(ImVec4(0.96f, 0.62f, 0.23f, 1.0f), "Discount: -%s", format_idr(discount).c_str());
				ImGui::Separator();
				ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.65f, 1.0f), "Grand Total: %s", format_idr(total).c_str());

				ImGui::Spacing();
				if (ui::primary_button(ICON_FA_CHECK "  Create Order (POST /order)", ImVec2(-1, 40), raw_subtotal > 0))
				{
					// Populate Order
					ui_instance->m_current_order.order_id = "55555555-5555-5555-5555-555555555555";
					ui_instance->m_current_order.transaction_id = "TRX-DUMMY-99999";
					ui_instance->m_current_order.gross_amount = total;
					ui_instance->m_current_order.discount_amount = discount;
					ui_instance->m_current_order.applied_promo_code = promo_code;
					ui_instance->m_current_order.status = TransactionStatus::PendingPayment;
					ui_instance->m_current_order.transaction_time = "2026-08-03 14:00:00";

					ui_instance->show_toast("Order Created", "Order #55555555 created with status PENDING. Redirecting to payment...", ImVec4(0.23f, 0.51f, 0.96f, 1.0f));
					ui_instance->m_active_tab = NavTab::TransactionPayment;
				}
			}
			ui::card_end();

			ImGui::Columns(1);
		}
		ui::card_end();
	}

	void ui_views::render_transaction_payment(ui* ui_instance)
	{
		ui::card_begin("Stage2PaymentCard", "STAGE 2: PAYMENT GATEWAY & LICENSE GENERATION (POST /payment)", "Complete transaction payment via QRIS/VA and automatically generate user license key.");
		{
			const char* steps[] = { "1. Product & Order", "2. Payment & License", "3. Download & Inject" };
			ui::stepper_widget(steps, 3, 1);

			if (ui_instance->m_current_order.status == TransactionStatus::None)
			{
				ImGui::TextColored(ImVec4(0.96f, 0.62f, 0.23f, 1.0f), ICON_FA_EXCLAMATION_TRIANGLE "  No active order pending payment. Please create an order from the Checkout tab first.");
				ImGui::Spacing();
				if (ui::secondary_button("Back to Product Catalog", ImVec2(220, 36)))
				{
					ui_instance->m_active_tab = NavTab::ProductsCatalog;
				}
				ui::card_end();
				return;
			}

			ImGui::Columns(2, "PaymentSplitArea", false);
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.50f);

			// Left: Payment Details Card
			ui::card_begin("PaymentInstructionsCard", "PAYMENT INSTRUCTIONS (QRIS / VA)", "Midtrans / Gateway Sandbox Simulation");
			{
				ImGui::TextDisabled("Order ID: %s", ui_instance->m_current_order.order_id.c_str());
				ImGui::TextDisabled("Transaction ID: %s", ui_instance->m_current_order.transaction_id.c_str());
				ImGui::TextDisabled("Merchant ID: %s", ui_instance->m_current_order.merchant_id.c_str());
				ImGui::Spacing();

				ImGui::Text("Payment Method:");
				ui::badge("QRIS / Permata VA", ImVec4(0.23f, 0.51f, 0.96f, 0.25f), ImVec4(0.40f, 0.70f, 1.0f, 1.0f));

				ImGui::Spacing();
				ImGui::Text("Virtual Account (VA) Number:");
				ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
				ImGui::TextColored(ImVec4(0.96f, 0.62f, 0.23f, 1.0f), "%s", ui_instance->m_current_order.va_number.c_str());
				ImGui::PopFont();

				ImGui::Spacing();
				ImGui::Text("Total Bill Amount:");
				ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.65f, 1.0f), "%s", format_idr(ui_instance->m_current_order.gross_amount).c_str());
			}
			ui::card_end();

			ImGui::NextColumn();

			// Right: Payment Status & Webhook Simulator Card
			ui::card_begin("WebhookSimulatorCard", "WEBHOOK NOTIFICATION & LICENSE ISSUANCE", "Simulate Gateway Callback to Ellohim-Server");
			{
				ImGui::Text("Current Status:");
				if (ui_instance->m_current_order.status == TransactionStatus::PendingPayment)
				{
					ui::badge("STATUS: PENDING PAYMENT", ImVec4(0.96f, 0.62f, 0.23f, 0.25f), ImVec4(1.0f, 0.75f, 0.3f, 1.0f));
				}
				else if (ui_instance->m_current_order.status == TransactionStatus::SettlementSuccess)
				{
					ui::badge("STATUS: SETTLEMENT (PAID)", ImVec4(0.16f, 0.72f, 0.53f, 0.25f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ui_instance->m_current_order.status == TransactionStatus::PendingPayment)
				{
					ImGui::TextWrapped("Klik tombol di bawah untuk mensimulasikan notifikasi Webhook pembayaran dari Gateway ke backend `payment_service::create`.");
					ImGui::Spacing();

					if (ui::primary_button(ICON_FA_CREDIT_CARD "  Simulate Payment Webhook Notification", ImVec2(-1, 44)))
					{
						ui_instance->m_current_order.status = TransactionStatus::SettlementSuccess;

						// Issue new License (matching Stage 2 of Ellohim WORKFLOW.md)
						UserLicense new_license;
						new_license.license_id = "99999999-9999-9999-9999-999999999999";
						new_license.product_name = "Ellohim Engine Pro";
						new_license.license_key = "ELLOHIM-PRO-2026-KEY1-9999";
						new_license.issued_at = "2026-08-03 14:00:00";
						new_license.expiry_date = "2027-08-03 (365 Days)";
						new_license.status = "ACTIVE";

						ui_instance->m_user_licenses.push_back(new_license);

						ui_instance->show_toast("Payment Settlement Received", "Payment verified! License ELLOHIM-PRO-2026-KEY1-9999 issued automatically.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
					}
				}
				else
				{
					ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.65f, 1.0f), ICON_FA_CHECK_CIRCLE "  Pembayaran Sukses! Lisensi telah aktif dan terdaftar ke user dummy_gamer.");
					ImGui::Spacing();
					if (ui::primary_button(ICON_FA_ROCKET "  Proceed to Binary Download & Injection (Stage 3)", ImVec2(-1, 42)))
					{
						ui_instance->m_active_tab = NavTab::BinaryDownload;
					}
				}
			}
			ui::card_end();

			ImGui::Columns(1);
		}
		ui::card_end();
	}

	void ui_views::render_my_licenses(ui* ui_instance)
	{
		ui::card_begin("MyLicensesCard", "MY LICENSES & SUBSCRIPTIONS", "Active cryptographic license keys issued to your account.");
		{
			if (ImGui::BeginTable("LicensesTable", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Product", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("License Key", ImGuiTableColumnFlags_WidthFixed, 240.0f);
				ImGui::TableSetupColumn("Issued Date", ImGuiTableColumnFlags_WidthFixed, 140.0f);
				ImGui::TableSetupColumn("Expires", ImGuiTableColumnFlags_WidthFixed, 160.0f);
				ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 90.0f);
				ImGui::TableHeadersRow();

				for (const auto& lic : ui_instance->m_user_licenses)
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "%s", lic.product_name.c_str());

					ImGui::TableSetColumnIndex(1);
					ImGui::TextColored(ImVec4(0.23f, 0.51f, 0.96f, 1.0f), "%s", lic.license_key.c_str());

					ImGui::TableSetColumnIndex(2);
					ImGui::TextDisabled("%s", lic.issued_at.c_str());

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%s", lic.expiry_date.c_str());

					ImGui::TableSetColumnIndex(4);
					ui::badge(lic.status.c_str(), ImVec4(0.16f, 0.72f, 0.53f, 0.2f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
				}
				ImGui::EndTable();
			}
		}
		ui::card_end();
	}

	void ui_views::render_binary_download(ui* ui_instance, renderer* renderer_ptr)
	{
		ui::card_begin("Stage3BinaryCard", "STAGE 3: BINARY VERSION CHECK, DOWNLOAD & LAUNCH INJECTOR", "Validate active license, fetch latest binary stream, and execute hypervisor loader.");
		{
			const char* steps[] = { "1. Product & Order", "2. Payment & License", "3. Download & Inject" };
			ui::stepper_widget(steps, 3, 2);

			ImGui::Columns(2, "BinarySplitArea", false);
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.50f);

			// Left: Version Check & Download Controls Card
			ui::card_begin("VersionCheckCard", "BINARY VERSION & AUTH VERIFICATION (GET /binary/version)", "JWT Authorization & License Key Check");
			{
				ImGui::Text("Local Version: ");
				ImGui::SameLine();
				ui::badge(ui_instance->m_local_version.c_str(), ImVec4(0.2f, 0.2f, 0.25f, 0.8f));

				ImGui::Text("Latest Remote Version: ");
				ImGui::SameLine();
				ui::badge(ui_instance->m_remote_version.c_str(), ImVec4(0.23f, 0.51f, 0.96f, 0.25f), ImVec4(0.40f, 0.70f, 1.0f, 1.0f));

				if (ui_instance->m_has_update)
				{
					ImGui::Spacing();
					ImGui::TextColored(ImVec4(0.96f, 0.62f, 0.23f, 1.0f), ICON_FA_EXCLAMATION_TRIANGLE "  New binary version v1.0.4 is available on server!");
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				if (ui_instance->m_is_downloading)
				{
					ui_instance->m_download_progress += ImGui::GetIO().DeltaTime * 0.4f;
					if (ui_instance->m_download_progress >= 1.0f)
					{
						ui_instance->m_download_progress = 1.0f;
						ui_instance->m_is_downloading = false;
						ui_instance->m_binary_downloaded = true;
						ui_instance->show_toast("Download Complete", "Binary streamed & decrypted successfully.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
					}

					ImGui::Text("Downloading Binary Stream (14.2 MB / 14.2 MB)...");
					ImGui::ProgressBar(ui_instance->m_download_progress, ImVec2(-1, 24));
				}
				else
				{
					const char* btn_text = ui_instance->m_binary_downloaded ? ICON_FA_SYNC "  Re-download Latest Binary Stream" : ICON_FA_DOWNLOAD "  Download & Decompress Binary (GET /binary/download)";
					if (ui::primary_button(btn_text, ImVec2(-1, 40)))
					{
						ui_instance->m_is_downloading = true;
						ui_instance->m_download_progress = 0.0f;
					}
				}
			}
			ui::card_end();

			ImGui::NextColumn();

			// Right: Injection Launcher Execution Card
			ui::card_begin("InjectorCard", "HYPERVISOR INJECTION ENGINE", "Inject verified binary payload into targeted process");
			{
				if (!ui_instance->m_binary_downloaded)
				{
					ImGui::TextDisabled("Binary not yet downloaded. Please complete Step 3 download first.");
				}
				else
				{
					ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.65f, 1.0f), ICON_FA_CHECK_CIRCLE "  Binary verified & decompressed in memory.");
					ImGui::Spacing();

					if (ui_instance->m_injected)
					{
						ui::badge("STATUS: INJECTED & RUNNING", ImVec4(0.16f, 0.72f, 0.53f, 0.25f), ImVec4(0.20f, 0.90f, 0.65f, 1.0f));
						ImGui::Spacing();
						ImGui::TextWrapped("Payload active in kernel memory. Enjoy your session!");
					}
					else
					{
						if (ui::primary_button(ICON_FA_ROCKET "  Execute Injection Payload", ImVec2(-1, 44)))
						{
							ui_instance->m_injected = true;
							ui_instance->show_toast("Injection Successful", "Gottvergessen Loader injected into target process.", ImVec4(0.16f, 0.72f, 0.53f, 1.0f));
						}
					}
				}
			}
			ui::card_end();

			ImGui::Columns(1);
		}
		ui::card_end();
	}

	void ui_views::render_settings(ui* ui_instance)
	{
		ui::card_begin("SettingsCard", "SETTINGS & DIAGNOSTIC CONFIGURATION", "Customize loader preferences and view live system logs.");
		{
			ImGui::Text("Server API Base URL:");
			static char api_url[128] = "https://api.ellohim.com/v1";
			ui::input_text("##ApiUrl", "API Base URL", api_url, sizeof(api_url));

			ImGui::Spacing();
			static bool auto_update = true;
			ImGui::Checkbox("Enable Automatic Binary Updates", &auto_update);

			static bool stream_logs = true;
			ImGui::Checkbox("Stream Realtime G3log Output to Console", &stream_logs);

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.98f, 1.0f), "Live System Log Window");
			ImGui::BeginChild("LogConsoleArea", ImVec2(0, 180.0f), true);
			{
				ImGui::TextDisabled("[INFO] Gottvergessen UI Wrapper v1.0 initialized.");
				ImGui::TextDisabled("[INFO] Connected to Ellohim-Server API endpoint: https://api.ellohim.com/v1");
				ImGui::TextDisabled("[STAGE 1] Product catalog loaded (3 available items).");
				ImGui::TextDisabled("[STAGE 2] Mock payment gateway initialized for QRIS / VA.");
				ImGui::TextDisabled("[STAGE 3] Binary download stream ready (SHA256 integrity verified).");
			}
			ImGui::EndChild();
		}
		ui::card_end();
	}
}
