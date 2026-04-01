/**
 * Input Viewer - Displays player inputs during races
 * 
 * Based on implementations from:
 * - mkw-sp (Copyright 2021-2023 Pablo Stebler, MIT License)
 * - Retro Rewind Team
 * - Insane Kart Wii
 */

#include <MarioKartWii/Kart/KartManager.hpp>
#include <MarioKartWii/Race/RaceInfo/RaceInfo.hpp>
#include <UI/CtrlRaceBase/InputViewer.hpp>
#include <Settings/Settings.hpp>

namespace Pulsar {
namespace UI {

const s8 CtrlRaceInputViewer::DPAD_HOLD_FOR_N_FRAMES = 10;

void CtrlRaceInputViewer::Init() {
    char name[32];
    const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
    const GameMode mode = scenario.settings.gamemode;
    
    // Check if brake drift is enabled
    bool isBrakedriftToggled = (scenario.settings.engineClass == CC_100);

    // Initialize D-pad panes
    for(int i = 0; i < (int)DpadState_Count; ++i) {
        DpadState state = static_cast<DpadState>(i);
        const char* stateName = CtrlRaceInputViewer::DpadStateToName(state);
        snprintf(name, 32, "Dpad%.*s", strlen(stateName), stateName);
        nw4r::lyt::Pane* pane = this->layout.GetPaneByName(name);
        this->SetPaneVisibility(name, state == DpadState_Off);
        this->m_dpadPanes[i] = pane;
        this->HudSlotColorEnable(name, true);
    }

    // Initialize accel panes
    for(int i = 0; i < (int)AccelState_Count; ++i) {
        AccelState state = static_cast<AccelState>(i);
        const char* stateName = CtrlRaceInputViewer::AccelStateToName(state);
        snprintf(name, 32, "Accel%.*s", strlen(stateName), stateName);
        nw4r::lyt::Pane* pane = this->layout.GetPaneByName(name);
        this->SetPaneVisibility(name, state == AccelState_Off);
        this->m_accelPanes[i] = pane;
        this->HudSlotColorEnable(name, true);
    }

    // Initialize trigger panes
    for(int i = 0; i < (int)Trigger_Count; ++i) {
        Trigger trigger = static_cast<Trigger>(i);
        const char* triggerName = CtrlRaceInputViewer::TriggerToName(trigger);
        for(int j = 0; j < (int)TriggerState_Count; ++j) {
            TriggerState state = static_cast<TriggerState>(j);
            const char* stateName = CtrlRaceInputViewer::TriggerStateToName(state);
            snprintf(name, 32, "Trigger%.*s%.*s", strlen(triggerName), triggerName, strlen(stateName), stateName);
            nw4r::lyt::Pane* pane = this->layout.GetPaneByName(name);
            this->SetPaneVisibility(name, state == TriggerState_Off);
            if(!isBrakedriftToggled && trigger == Trigger_BD) {
                this->SetPaneVisibility(name, false);
            }
            this->m_triggerPanes[i][j] = pane;
            this->HudSlotColorEnable(name, true);
        }
    }

    // Initialize stick pane
    this->m_stickPane = this->layout.GetPaneByName("Stick");
    this->m_stickOrigin = this->m_stickPane->trans;
    this->m_playerId = this->GetPlayerId();
    this->HudSlotColorEnable("Stick", true);
    this->HudSlotColorEnable("StickBackdrop", true);

    // Check if nunchuk layout
    u32 inputViewerSetting = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_INPUTVIEWER);
    if(inputViewerSetting == RACESETTING_INPUTVIEWER_NUNCHUK) {
        this->HudSlotColorEnable("ChukBg", true);
    }

    LayoutUIControl::Init();
}

void CtrlRaceInputViewer::OnUpdate() {
    this->UpdatePausePosition();

    const Kart::Pointers& pointers = Kart::Manager::sInstance->players[this->GetPlayerId()]->pointers;
    const u8 hudSlotId = this->GetPlayerId();

    if(pointers.values != nullptr && hudSlotId < 4) {
        const Input::RealControllerHolder* controller = SectionMgr::sInstance->pad.padInfos[hudSlotId].controllerHolder;
        if(controller != nullptr) {
            const Input::State* input = &controller->inputStates[0];

            // Process stick input
            Vec2 stick;
            stick.x = input->stickX / 7.0f;
            stick.z = input->stickY / 7.0f;

            // Process button inputs
            bool accel = input->buttonActions & 0x1;
            bool L = input->buttonActions & 0x8;
            bool R = input->buttonActions & 0x4;

            // Process D-pad
            DpadState dpadState = DpadState_Off;
            if(input->buttonActions & 0x200) dpadState = DpadState_Up;
            else if(input->buttonActions & 0x100) dpadState = DpadState_Down;
            else if(input->buttonActions & 0x400) dpadState = DpadState_Left;
            else if(input->buttonActions & 0x800) dpadState = DpadState_Right;

            setDpad(dpadState);
            setAccel(accel ? AccelState_Pressed : AccelState_Off);
            setTrigger(Trigger_L, L ? TriggerState_Pressed : TriggerState_Off);
            setTrigger(Trigger_R, R ? TriggerState_Pressed : TriggerState_Off);
            setStick(stick);

            // Check for brake drift
            const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
            const GameMode mode = scenario.settings.gamemode;
            bool isBrakedriftToggled = (scenario.settings.engineClass == CC_100);

            if(isBrakedriftToggled) {
                bool BD = input->buttonActions & 0x10;
                setTrigger(Trigger_BD, BD ? TriggerState_Pressed : TriggerState_Off);
            }
        }
    }
}

u32 CtrlRaceInputViewer::Count() {
    u32 inputViewerSetting = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_INPUTVIEWER);
    if(inputViewerSetting != RACESETTING_INPUTVIEWER_DISABLED) {
        const RacedataScenario& scenario = Racedata::sInstance->racesScenario;
        u32 localPlayerCount = scenario.localPlayerCount;
        const SectionId sectionId = SectionMgr::sInstance->curSection->sectionId;
        
        // Add ghost viewer if watching ghost
        if(sectionId >= SECTION_WATCH_GHOST_FROM_CHANNEL && sectionId <= SECTION_WATCH_GHOST_FROM_MENU) {
            localPlayerCount += 1;
        }
        
        // Handle spectator mode
        if(localPlayerCount == 0 && (scenario.settings.gametype & GAMETYPE_ONLINE_SPECTATOR)) {
            localPlayerCount = 1;
        }
        
        return localPlayerCount;
    }
    return 0;
}

void CtrlRaceInputViewer::Create(Page& page, u32 index, u32 count) {
    u8 variantId = (count == 3) ? 4 : count;
    for(int i = 0; i < count; ++i) {
        CtrlRaceInputViewer* inputViewer = new CtrlRaceInputViewer;
        page.AddControl(index + i, *inputViewer, 0);
        char variant[0x20];
        int pos = i;
        snprintf(variant, 0x20, "InputDisplay_%u_%u", variantId, pos);
        inputViewer->Load(variant, i);
    }
}

static CustomCtrlBuilder INPUTVIEWER(CtrlRaceInputViewer::Count, CtrlRaceInputViewer::Create);

void CtrlRaceInputViewer::Load(const char* variant, u8 id) {
    this->hudSlotId = id;
    ControlLoader loader(this);
    const char* groups[] = {nullptr, nullptr};
    
    u32 inputViewerSetting = Settings::Mgr::Get().GetSettingValue(Settings::SETTINGSTYPE_RACE, SETTINGRACE_RADIO_INPUTVIEWER);
    if(inputViewerSetting == RACESETTING_INPUTVIEWER_NUNCHUK) {
        loader.Load(UI::raceFolder, "PULInputViewerChuk", variant, groups);
    } else {
        loader.Load(UI::raceFolder, "PULInputViewer", variant, groups);
    }
}

void CtrlRaceInputViewer::setDpad(DpadState state) {
    if(state == m_dpadState) {
        return;
    }
    
    // Only hold for off press
    if(state == DpadState_Off && m_dpadTimer != 0 && --m_dpadTimer) {
        return;
    }
    
    m_dpadPanes[static_cast<u32>(m_dpadState)]->flag &= ~1;
    m_dpadPanes[static_cast<u32>(state)]->flag |= 1;
    m_dpadState = state;
    m_dpadTimer = DPAD_HOLD_FOR_N_FRAMES;
}

void CtrlRaceInputViewer::setAccel(AccelState state) {
    if(state == m_accelState) {
        return;
    }
    
    m_accelPanes[static_cast<u32>(m_accelState)]->flag &= ~1;
    m_accelPanes[static_cast<u32>(state)]->flag |= 1;
    m_accelState = state;
}

void CtrlRaceInputViewer::setTrigger(Trigger trigger, TriggerState state) {
    if(state == m_triggerStates[static_cast<u32>(trigger)]) {
        return;
    }
    
    m_triggerPanes[static_cast<u32>(trigger)][static_cast<u32>(m_triggerStates[static_cast<u32>(trigger)])]->flag &= ~1;
    m_triggerPanes[static_cast<u32>(trigger)][static_cast<u32>(state)]->flag |= 1;
    m_triggerStates[static_cast<u32>(trigger)] = state;
}

void CtrlRaceInputViewer::setStick(const Vec2& state) {
    const float scale = 10.0f;
    m_stickPane->trans.x = m_stickOrigin.x + scale * state.x * m_stickPane->scale.x * m_stickPane->size.x;
    m_stickPane->trans.y = m_stickOrigin.y + scale * state.z * m_stickPane->scale.z * m_stickPane->size.z;
    m_stickState = state;
}

}  // namespace UI
}  // namespace Pulsar
