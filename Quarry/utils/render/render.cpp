#include "render.h"
#include "..\..\menu\menu.h"
#include "..\..\cheats\visuals\visuals_init.h"
#include "..\..\Bytesa.h"
#include "..\..\menu\ImGui\imgui_freetype.h"
#include <mutex>

ImDrawListSharedData _data;

std::mutex render_mutex;

void g_render::create_objects(IDirect3DDevice9* pDevice)
{
	ImGui_ImplDX9_CreateDeviceObjects();
}

void g_render::invalidate_objects()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();;
}

void g_render::gui_init(IDirect3DDevice9* pDevice)
{
	if (d3d_init)
		return;

	ImGui::CreateContext();

	ImGui_ImplWin32_Init(INIT::Window);
	ImGui_ImplDX9_Init(pDevice);

	m_draw_list = std::make_shared<ImDrawList>(ImGui::GetDrawListSharedData());
	m_data_draw_list = std::make_shared<ImDrawList>(ImGui::GetDrawListSharedData());
	m_replace_draw_list = std::make_shared<ImDrawList>(ImGui::GetDrawListSharedData());

	auto& io = ImGui::GetIO();
	auto& style = ImGui::GetStyle();

	style.WindowMinSize = ImVec2(10, 10);

	ImFontConfig m_config;
	m_config.OversampleH = m_config.OversampleV = 3;
	m_config.RasterizerFlags = ImGuiFreeType::Monochrome || ImGuiFreeType::MonoHinting;
	m_config.PixelSnapH = false;

	ImFontConfig esp_config_aa;
	ImFontConfig esp_config;

	esp_config_aa.RasterizerFlags = ImGuiFreeType::Monochrome || ImGuiFreeType::MonoHinting;
	m_config.PixelSnapH = true;
	m_config.OversampleH = m_config.OversampleV = 1;

	static const ImWchar rangescyr[] =
	{
		0x0020, 0x00FF,
		0x0400, 0x044F,
		0
	};

	// Menu fonts
	c_menu::get().futura_small = io.Fonts->AddFontFromMemoryTTF((void*)futuramediumc, sizeof(futuramediumc), 15.f, &m_config, rangescyr);
	c_menu::get().futura = io.Fonts->AddFontFromMemoryTTF((void*)futuramediumc, sizeof(futuramediumc), 17.f, &m_config, rangescyr);
	c_menu::get().futura_large = io.Fonts->AddFontFromMemoryTTF((void*)futuramediumc, sizeof(futuramediumc), 20.f, &m_config, rangescyr);
	c_menu::get().gotham = io.Fonts->AddFontFromMemoryTTF((void*)GothamPro, sizeof(GothamPro), 13.f, &m_config, rangescyr);

	// Icon fonts
	c_menu::get().ico_menu = io.Fonts->AddFontFromMemoryTTF((void*)icomenu, sizeof(icomenu), 20.f, &m_config, io.Fonts->GetGlyphRangesCyrillic());
	c_menu::get().ico_bottom = io.Fonts->AddFontFromMemoryTTF((void*)iconbot, sizeof(iconbot), 20.f, &m_config, io.Fonts->GetGlyphRangesCyrillic());

	// Esp fonts
	g_render::get().logs = io.Fonts->AddFontFromMemoryTTF((void*)lucida_console, sizeof(lucida_console), 11.f, &esp_config, rangescyr);
	g_render::get().name = io.Fonts->AddFontFromMemoryTTF((void*)opensans, sizeof(opensans), 13.f, &esp_config_aa, rangescyr);
	g_render::get().esp = io.Fonts->AddFontFromMemoryTTF((void*)opensans, sizeof(opensans), 12.f, &esp_config_aa, rangescyr);
	g_render::get().world = io.Fonts->AddFontFromMemoryTTF((void*)opensans, sizeof(opensans), 12.f, &esp_config_aa, rangescyr);
	g_render::get().damage = io.Fonts->AddFontFromMemoryTTF((void*)verdana, sizeof(verdana), 15.f, &esp_config_aa, rangescyr);
	g_render::get().indicators = io.Fonts->AddFontFromMemoryTTF((void*)verdana_bold, sizeof(verdana_bold), 25.f, &esp_config_aa, rangescyr);
	g_render::get().grenades = io.Fonts->AddFontFromMemoryTTF((void*)undefeated_font, sizeof(undefeated_font), 20.f, &esp_config_aa, io.Fonts->GetGlyphRangesCyrillic());
	g_render::get().weapon_icons = io.Fonts->AddFontFromMemoryTTF((void*)undefeated_font, sizeof(undefeated_font), 13.f, &esp_config_aa, io.Fonts->GetGlyphRangesCyrillic());

	ImGuiFreeType::BuildFontAtlas(io.Fonts, 0x00);

	ImGui_ImplDX9_CreateDeviceObjects();

	d3d_init = true;
}

void g_render::scene_add()
{
	const auto lock = std::unique_lock<std::mutex>(m_mutex, std::try_to_lock);

	if (lock.owns_lock()) {
		*m_replace_draw_list = *m_data_draw_list;
	}

	*ImGui::GetBackgroundDrawList() = *m_replace_draw_list;
}

void g_render::scene_render()
{
	if (!d3d_init)
		return;

	m_draw_list->Clear();
	m_draw_list->PushClipRectFullScreen();

	m_screen_size = *reinterpret_cast<Vector2D*>(&ImGui::GetIO().DisplaySize);

	visuals_init::get().init();
	{
		const auto lock = std::unique_lock<std::mutex>(m_mutex);
		*m_data_draw_list = *m_draw_list;
	}
}

float g_render::text(ImFont* font, float x, float y, Color color, bool center_x, bool center_y, bool outline, bool shadow, const char* text, ...)
{
	auto font_size = font->FontSize;

	va_list va_alist;
	char buffer[256];
	va_start(va_alist, text);
	_vsnprintf(buffer, sizeof(buffer), text, va_alist);
	va_end(va_alist);

	ImVec2 m_text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, buffer);

	if (!font)
		return 0.f;

	if (!font->ContainerAtlas)
		return 0.f;

	m_draw_list->PushTextureID(font->ContainerAtlas->TexID);

	if (center_x)
		x -= m_text_size.x / 2.0f;

	if (center_y)
		y -= m_text_size.y / 2.0f;

	font->FontSize;

	if (outline)
	{
		m_draw_list->AddText(font, font_size, ImVec2(x + 1, y + 1), GetU32(Color(0, 0, 0, color.a())), buffer);
		m_draw_list->AddText(font, font_size, ImVec2(x - 1, y - 1), GetU32(Color(0, 0, 0, color.a())), buffer);
		m_draw_list->AddText(font, font_size, ImVec2(x + 1, y - 1), GetU32(Color(0, 0, 0, color.a())), buffer);
		m_draw_list->AddText(font, font_size, ImVec2(x - 1, y + 1), GetU32(Color(0, 0, 0, color.a())), buffer);
	}

	if (shadow)
	{
		const auto col_bg = ImColor(0, 0, 0, ((GetU32(color) >> IM_COL32_A_SHIFT) & 0xFF) / 2);
		m_draw_list->AddText(font, font_size, ImVec2(x + 1, y + 1), col_bg, buffer);
	}

	m_draw_list->AddText(font, font_size, ImVec2(x, y), GetU32(color), buffer);
	m_draw_list->PopTextureID();

	return y + m_text_size.y;
}

void g_render::line(float x1, float y1, float x2, float y2, Color color, float thickness) {
	m_draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), GetU32(color), thickness);
}

void g_render::ring3d(int16_t x, int16_t y, int16_t z, int16_t radius, uint16_t points, Color color1, Color color2, float thickness, float prog, bool fill_prog)
{
	static float Step = (M_PI * 2.0f) / (points);
	std::vector<ImVec2> point;

	for (float lat = 0.f; lat <= M_PI * 2.0f; lat += Step)
	{
		const auto& point3d = Vector(sin(lat), cos(lat), 0.f) * radius;
		Vector point2d;

		if (math::world_to_screen(Vector(x, y, z) + point3d, point2d))
			point.push_back(ImVec2(point2d.x, point2d.y));
	}

	m_draw_list->AddConvexPolyFilled(point.data(), point.size(), GetU32(color2));
	m_draw_list->AddPolyline(point.data(), max(0, point.size() * prog), GetU32(color1), prog == 1.f, thickness);
}

void g_render::sided_arc(float x, float y, float radius, float scale, Color col, float thickness)
{
	arc(x, y, radius, -270, -90 - 90 * (scale * 2), col, thickness);
	arc(x, y, radius, -90 + 90 * (scale * 2), 90, col, thickness);
}

void g_render::line_gradient(float x1, float y1, float x2, float y2, Color color1, Color color2, float thickness)
{
	m_draw_list->AddRectFilledMultiColor(ImVec2(x1, y2), ImVec2(x2 + thickness, y2 + thickness),
		GetU32(color1), GetU32(color2), GetU32(color2), GetU32(color1));
}

void g_render::rect(float x, float y, float w, float h, Color color, float rounding)
{
	m_draw_list->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), GetU32(color), rounding);
}

void g_render::rect_filled(float x, float y, float w, float h, Color color, float rounding)
{
	m_draw_list->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), GetU32(color), rounding);
}

void g_render::rect_filled_gradient(float x, float y, float w, float h, Color first, Color second, int type)
{
	if (type == GradientType::GRADIENT_HORIZONTAL)
		m_draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + w, y + h), GetU32(first), GetU32(second), GetU32(second), GetU32(first));
	else
		m_draw_list->AddRectFilledMultiColor(ImVec2(x, y), ImVec2(x + w, y + h), GetU32(first), GetU32(first), GetU32(second), GetU32(second));
}

void g_render::circle(float x, float y, float radius, int points, Color color, float thickness) {
	m_draw_list->AddCircle(ImVec2(x, y), radius, GetU32(color), points, thickness);
}

void g_render::circle_filled(float x, float y, float radius, int points, Color color) {
	m_draw_list->AddCircleFilled(ImVec2(x, y), radius, GetU32(color), points);
}

void g_render::triangle(Vector2D first, Vector2D second, Vector2D third, Color color, float thickness)
{
	m_draw_list->AddTriangle(ImVec2(first.x, first.y), ImVec2(second.x, second.y), ImVec2(third.x, third.y), GetU32(color), thickness);
}

void g_render::filled_triangle(Vector2D first, Vector2D second, Vector2D third, Color color)
{
	m_draw_list->AddTriangleFilled(ImVec2(first.x, first.y), ImVec2(second.x, second.y), ImVec2(third.x, third.y), GetU32(color));
}

void g_render::image(ImTextureID user_texture_id, const ImVec2& a, const ImVec2& b, const ImVec2& uv_a, const ImVec2& uv_b, ImU32 color) {
	m_draw_list->AddImage(user_texture_id, a, b, uv_a, uv_b, color);
}

void g_render::arc(float x, float y, float radius, float min_angle, float max_angle, Color color, float thickness) {
	m_draw_list->PathArcTo(ImVec2(x, y), radius, DEG2RAD(min_angle), DEG2RAD(max_angle), 32);
	m_draw_list->PathStroke(GetU32(color), false, thickness);
}

void g_render::Draw3DRainbowCircle(const Vector& origin, float radius)
{
	static auto hue_offset = 0.0f;
	auto prevScreenPos = ZERO; 
	static auto step = M_PI * 2.0f / 72.0f;

	auto screenPos = ZERO;

	for (auto rotation = 0.0f; rotation <= M_PI * 2.0f; rotation += step) 
	{
		Vector pos(radius * cos(rotation) + origin.x, radius * sin(rotation) + origin.y, origin.z);

		Ray_t ray;
		trace_t trace;
		CTraceFilterWorldOnly filter;

		ray.Init(origin, pos);
		m_trace()->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace);

		if (math::world_to_screen(trace.endpos, screenPos))
		{
			if (!prevScreenPos.IsZero())
			{
				auto hue = RAD2DEG(rotation) + hue_offset;

				float r, g, b;
				ImGui::ColorConvertHSVtoRGB(hue / 360.0f, 1.0f, 1.0f, r, g, b);

				m_draw_list->AddLine(ImVec2(prevScreenPos.x, prevScreenPos.y), ImVec2(screenPos.x, screenPos.y), GetU32(Color(r, g, b)));
			}

			prevScreenPos = screenPos;
		}
	}

	hue_offset += m_globals()->m_frametime * 200.0f;
}

void g_render::Draw3DCircle(const Vector& origin, float radius, Color color)
{
	auto prevScreenPos = ZERO;
	static auto step = M_PI * 2.0f / 72.0f;

	auto screenPos = ZERO;

	for (auto rotation = 0.0f; rotation <= M_PI * 2.0f; rotation += step) 
	{
		Vector pos(radius * cos(rotation) + origin.x, radius * sin(rotation) + origin.y, origin.z);

		Ray_t ray;
		trace_t trace;
		CTraceFilterWorldOnly filter;

		ray.Init(origin, pos);
		m_trace()->TraceRay(ray, MASK_SHOT_BRUSHONLY, &filter, &trace);

		if (math::world_to_screen(trace.endpos, screenPos))
		{
			if (!prevScreenPos.IsZero())
				line(prevScreenPos.x, prevScreenPos.y, screenPos.x, screenPos.y, color);

			prevScreenPos = screenPos;
		}
	}
}

void g_render::Draw3DFilledCircle(const Vector& origin, float radius, Color color)
{
	auto prevScreenPos = ZERO; 
	static auto step = M_PI * 2.0f / 72.0f;

	auto screenPos = ZERO;
	auto screen = ZERO;

	if (!math::world_to_screen(origin, screen))
		return;

	for (auto rotation = 0.0f; rotation <= M_PI * 2.0f; rotation += step)
	{
		Vector pos(radius * cos(rotation) + origin.x, radius * sin(rotation) + origin.y, origin.z);

		if (math::world_to_screen(pos, screenPos))
		{
			if (!prevScreenPos.IsZero() && prevScreenPos.IsValid() && screenPos.IsValid() && prevScreenPos != screenPos)
			{
				line(prevScreenPos.x, prevScreenPos.y, screenPos.x, screenPos.y, color);
				filled_triangle(Vector2D(screen.x, screen.y), Vector2D(screenPos.x, screenPos.y), Vector2D(prevScreenPos.x, prevScreenPos.y), Color(color.r(), color.g(), color.b(), color.a() / 2));
			}

			prevScreenPos = screenPos;
		}
	}
}

void g_render::dual_circle(float x, float y, float radius, int resolution, DWORD color, DWORD color2, LPDIRECT3DDEVICE9 device)
{
	LPDIRECT3DVERTEXBUFFER9 g_pVB2 = nullptr;
	std::vector <CUSTOMVERTEX2> circle(resolution + 2);

	circle[0].x = x;
	circle[0].y = y;
	circle[0].z = 0.0f;

	circle[0].rhw = 1.0f;
	circle[0].color = color2;

	for (auto i = 1; i < resolution + 2; i++)
	{
		circle[i].x = (float)(x - radius * cos(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].y = (float)(y - radius * sin(D3DX_PI * ((i - 1) / (resolution / 2.0f))));
		circle[i].z = 0.0f;

		circle[i].rhw = 1.0f;
		circle[i].color = color;
	}

	device->CreateVertexBuffer((resolution + 2) * sizeof(CUSTOMVERTEX2), D3DUSAGE_WRITEONLY, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &g_pVB2, nullptr); 

	if (!g_pVB2)
		return;

	void* pVertices;

	g_pVB2->Lock(0, (resolution + 2) * sizeof(CUSTOMVERTEX2), (void**)&pVertices, 0); 
	memcpy(pVertices, &circle[0], (resolution + 2) * sizeof(CUSTOMVERTEX2));
	g_pVB2->Unlock();

	device->SetTexture(0, nullptr);
	device->SetPixelShader(nullptr);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetStreamSource(0, g_pVB2, 0, sizeof(CUSTOMVERTEX2));
	device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	device->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, resolution);

	g_pVB2->Release();
}