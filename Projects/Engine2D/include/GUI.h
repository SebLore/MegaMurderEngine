#pragma once
#include "Engine2D.h"
//#include <Engine2D.h>

#include <Common/Scene/Sprite.h>
#include "SpriteText.h"

struct BarElement
{
    Sprite m_Background;
    Sprite m_Display;
    SpriteText m_ValueText;
    std::wstring m_StaticText;

    float m_Value;
    float m_MaxValue;
    float m_MaxWidth;
    float m_Depletion = 0; ///< value between 1 and 0 that handels how depleted a bar is
};



class GUI
{
public:
    struct Elements
    {
        Sprite			m_Crosshair;
        Sprite			m_WeaponDisplay;

        BarElement		m_HealthBar;
        BarElement		m_ComboBar;

        SpriteText		m_Timer;
        SpriteText		m_Speed;
    };

public:
    GUI() = default;
    ~GUI() = default;
    GUI(GUI&&) = default;
    GUI& operator=(GUI&&) = default;

    GUI(Engine2D& engine2D);

    /// @brief Initializes the gui object
    /// @param engine2D Engine2D to store inside of the gui object for later use
    void Initialize(Engine2D& engine2D);

    /// @brief Initializes the sprite object inside the GUI object
    /// @param texutreName Path to the sprite texture
    void InitializeCrosshair(const std::string& textureName);

    /// @brief Initializes the healthbar gui element
    /// @param background Path to the background texture
    /// @param health Path to the health texture
    /// @param healthValue Starting wstring used for spritefont
    void InitializeHealthBar(const std::string& background,
        const std::string& health,
        unsigned int startingHealth,
        unsigned int maxHealth);

    /// @brief Initializes the combobar gui element
        /// @param background Path to the background texture
        /// @param combo Path to the combo texture
        /// @param comboValue Starting wstring used for spritefont
    void InitializeComboBar(const std::string& background,
        const std::string& combo,
        const std::wstring& comboValue);


    void InitializeTimer(int posX, int posY);
    void InitializeSpeed(int posX, int posY);

    /// @brief Draws the crosshair at the center of the screen
    void DrawCrosshair();

    void DrawHealth();
    void DrawCombo();
    void DrawTimer();
    void DrawSpeed();

    // TODO: implement these function if the entire sptire system is changed
    void AddSprite(Sprite* sprite);
    void AddText(SpriteText* text);

    /// Setting the value for the different hud elements
    void SetHealthValue(float value);
    void SetTimer(const uint32_t milliseconds);
    void SetSpeed(const float speed); ///< in m/s currently, will change based on later implementations

    void Update(); ///< currently not implemented

    /* Functions for updating the combo
     * CUrrently just in limbo
     *
     * void SetComboValue(unsigned int value);
     * void UpdateCombo(float dt);
     * unsigned int GetComboValue() { return m_Elements.m_ComboBar.m_Value; }
     */

private:
    Engine2D* m_Engine2D = nullptr;
    Elements	m_Elements;


    // TODO: dynamic elements
    // using GuiElements = std::vector<std::unique_ptr<IGuiElement>>
    //GuiElements elements;
};

