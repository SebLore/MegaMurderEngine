#include <GUI.h>

GUI::GUI(Engine2D& engine2D)
	: m_Engine2D(&engine2D)
{

}

void GUI::Initialize(Engine2D& engine2D)
{
	m_Engine2D = &engine2D;
}
/*
void GUI::InitializeCrosshair(const std::string& textureName)
{

	float widthHalf =
		(static_cast<float>(m_Engine2D->GetViewportWidth())) / 2.0f;
	float heightHalf =
		(static_cast<float>(m_Engine2D->GetViewportHeight())) / 2.0f;

	Sprite::Transform sf = { .Position = { widthHalf, heightHalf },
						   .Scale = { 16.0f, 16.0f },
						   .Anchor = Sprite::Anchor::Center,
						   .Rotation = 0.0f };

	m_Elements.m_Crosshair.Initialize(textureName, m_Engine2D->GetResourceManager(), sf, 32.0f, 32.0f);
}


void GUI::InitializeHealthBar(const std::string& background,
	const std::string& health,
	unsigned int startingHealth,
	unsigned int maxHealth)
{


	Sprite::Transform sf = { .Position = { 150.0f, m_Engine2D->GetViewportHeight() - 50.0f },
					    .Scale = { 256.0f, 32.0f },
	                                    .Anchor = Sprite::Anchor::TopLeft,
					    .Rotation = 0.0f, };

	m_Elements.m_HealthBar.m_Background.Initialize(background, m_Engine2D->GetResourceManager(), sf, 128.0f, 32.0f);
	m_Elements.m_HealthBar.m_Display.Initialize(health, m_Engine2D->GetResourceManager(), sf, 128.0f, 32.0f);
	
	m_Elements.m_HealthBar.m_ValueText.Initialize(std::to_wstring(startingHealth) + L"/" + std::to_wstring(maxHealth), 
		DirectX::XMFLOAT2(200.0f, 
			m_Engine2D->GetViewportHeight() - 100.0f));
	m_Elements.m_HealthBar.m_Value = static_cast<float>(startingHealth);
	m_Elements.m_HealthBar.m_MaxValue = static_cast<float>(maxHealth);
	m_Elements.m_HealthBar.m_MaxWidth = sf.Scale.x;
        
	m_Elements.m_HealthBar.m_StaticText = L"/" + std::to_wstring(maxHealth); ///< set the "/1000" to max health when that is added
}

void GUI::InitializeComboBar(const std::string& background,
	const std::string& combo,
	const std::wstring& comboValue)
{
	Sprite::Transform sf = { .Position = { m_Engine2D->GetViewportWidth() - 150.0f, 50.0f  },
	    .Scale = { 128.0f, 32.0f },
	    .Anchor = Sprite::Anchor::Center,
	    .Rotation = 0.0f };


	m_Elements.m_ComboBar.m_Background.Initialize(background, m_Engine2D->GetResourceManager(), sf, 128.0f, 32.0f);
	m_Elements.m_ComboBar.m_Display.Initialize(combo, m_Engine2D->GetResourceManager(), sf, 128.0f, 32.0f);
	m_Elements.m_ComboBar.m_ValueText.Initialize(comboValue, DirectX::XMFLOAT2( sf.Position.x, sf.Position.y+60.0f));
	m_Elements.m_ComboBar.m_Value = 0.0f;
	m_Elements.m_ComboBar.m_MaxValue = 10000.0f;
	m_Elements.m_ComboBar.m_MaxWidth = sf.Scale.x;
	m_Elements.m_ComboBar.m_StaticText = L" Combo!";
}

void GUI::InitializeTimer( int posX, int posY)
{
	m_Elements.m_Timer.Initialize(L"00:00:00", DirectX::XMFLOAT2(posX, posY));
}

void GUI::InitializeSpeed( int posX, int posY)
{
	m_Elements.m_Speed.Initialize(L"0.00 m/s", DirectX::XMFLOAT2(posX, posY));
}


void GUI::DrawCrosshair()
{
    m_Elements.m_Crosshair.Draw(m_Engine2D->GetSpriteBatch());
}

void GUI::DrawHealth()
{
	m_Elements.m_HealthBar.Draw(m_Engine2D);
}

void GUI::DrawCombo()
{
	m_Elements.m_ComboBar.Draw(m_Engine2D);
}

void GUI::DrawTimer()
{
	m_Elements.m_Timer.Draw(m_Engine2D->GetSpriteBatch(), m_Engine2D->GetSpriteFont());
}

void GUI::DrawSpeed()
{
	m_Elements.m_Speed.Draw(m_Engine2D->GetSpriteBatch(), m_Engine2D->GetSpriteFont());
}

void GUI::SetHealthValue(float value)
{
	m_Elements.m_HealthBar.SetValue(value);
	m_Elements.m_HealthBar.m_Depletion = m_Elements.m_HealthBar.m_Value / m_Elements.m_HealthBar.m_MaxValue; ///< used to lower the value for health only
	m_Elements.m_HealthBar.UpdateDisplay();
}



void GUI::SetTimer(const uint32_t totalMs)
{
	uint32_t min = totalMs / 60000;
	uint32_t sec = (totalMs / 1000) % 60;
	uint32_t milliseconds = totalMs % 1000;

	wchar_t buffer[16];
	swprintf_s(buffer, L"%02u:%02u:%03u",
		min, sec, milliseconds);
	m_Elements.m_Timer.SetText(std::wstring(buffer));
}

void GUI::SetSpeed(const float speed)
{
	wchar_t buffer[16];
	swprintf_s(buffer, L"%.2f m/s", speed);
	m_Elements.m_Speed.SetText(buffer);
}

void GUI::Update()
{
    
}
 */