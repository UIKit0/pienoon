// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "precompiled.h"
#include "gui_menu.h"
#include "config_generated.h"
#include "character_state_machine_def_generated.h"

namespace fpl {
namespace splat {

GuiMenu::GuiMenu() {}

static const char* TextureName(const ButtonTexture& button_texture) {
#ifdef PLATFORM_MOBILE
  if (button_texture.touch_screen() != nullptr)
    return button_texture.touch_screen()->c_str();
#endif
  return button_texture.standard()->c_str();
}

void GuiMenu::Setup(const UiGroup* menu_def, MaterialManager* matman) {
  ClearRecentSelections();
  if (menu_def == nullptr) {
    button_list_.resize(0);
    current_focus_ = ButtonId_Undefined;
    return;   // Nothing to set up.  Just clearing things out.
  }
  menu_def_ = menu_def;
  button_list_.resize(menu_def->button_list()->Length());
  current_focus_ = menu_def->starting_selection();
  // Empty the queue.

  for (size_t i = 0; i < menu_def->button_list()->Length(); i++) {
    const ButtonDef* button = menu_def->button_list()->Get(i);
    button_list_[i].set_up_material(matman->LoadMaterial(
        TextureName(*button->texture_normal())));
    button_list_[i].set_down_material(matman->LoadMaterial(
        TextureName(*button->texture_pressed())));

    Shader* shader = matman->LoadShader(
          button->shader()->c_str());
    if (shader == nullptr) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "Buttons used in menus must specify a shader!");
    }
    button_list_[i].set_shader(shader);
    button_list_[i].set_button_def(button);
    button_list_[i].set_is_active(button->starts_active());
    button_list_[i].set_is_highlighted(true);
  }
}


// Force the material manager to load all the textures and shaders
// used in the UI group.
void GuiMenu::LoadAssets(const UiGroup* menu_def, MaterialManager* matman) {
  for (size_t i = 0; i < menu_def->button_list()->Length(); i++) {
    const ButtonDef* button = menu_def->button_list()->Get(i);
    matman->LoadMaterial(TextureName(*button->texture_normal()));
    matman->LoadMaterial(TextureName(*button->texture_pressed()));

    Shader* shader = matman->LoadShader(
          button->shader()->c_str());
    if (shader == nullptr) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "Buttons used in menus must specify a shader!");
    }
  }
}

void GuiMenu::AdvanceFrame(WorldTime delta_time, InputSystem* input,
                           const vec2& window_size) {
  // Start every frame with a clean list of events.
  ClearRecentSelections();
  for (size_t i = 0; i < button_list_.size(); i++) {
    button_list_[i].AdvanceFrame(delta_time, input, window_size);
    button_list_[i].set_is_highlighted(
        current_focus_ == button_list_[i].GetId());

    if (button_list_[i].button().is_down()) {
    if ((button_list_[i].button_def()->event_trigger() ==
         ButtonEvent_ButtonHold && button_list_[i].button().is_down()) ||
        (button_list_[i].button_def()->event_trigger() ==
         ButtonEvent_ButtonPress && button_list_[i].button().went_down())) {
      unhandled_selections_.push(MenuSelection(button_list_[i].GetId(),
                                               kTouchController));
    }}
  }
}

// Utility function for finding indexes.
TouchscreenButton* GuiMenu::FindButtonById(ButtonId id) {
  for (size_t i = 0; i < button_list_.size(); i++) {
    if (button_list_[i].GetId() == id)
      return &button_list_[i];
  }
  return nullptr;
}

// Utility function for clearing out the queue, since the syntax is weird.
void GuiMenu::ClearRecentSelections() {
  std::queue<MenuSelection>().swap(unhandled_selections_);
}

MenuSelection GuiMenu::GetRecentSelection() {
  if (unhandled_selections_.empty()) {
    return MenuSelection(ButtonId_Undefined, kUndefinedController);
  } else {
    MenuSelection return_value = unhandled_selections_.front();
    unhandled_selections_.pop();
    return return_value;
  }
}

void GuiMenu::Render(Renderer* renderer) {
  // Render touch controls, as long as the touch-controller is active.
  for (size_t i = 0; i < button_list_.size(); i++) {
    button_list_[i].Render(*renderer);
  }
}

// Accepts logical inputs, and navigates based on it.
void GuiMenu::HandleControllerInput(uint32_t logical_input,
                                    ControllerId controller_id) {
  TouchscreenButton* current_focus_button_ = FindButtonById(current_focus_);
  if (!current_focus_button_) {
    // errors?
    return;
  }
  const ButtonDef* current_def = current_focus_button_->button_def();
  if (logical_input & LogicalInputs_Left) {
    UpdateFocus(current_def->nav_left());
  }
  if (logical_input & LogicalInputs_Right) {
    UpdateFocus(current_def->nav_right());
  }

  if (logical_input & LogicalInputs_ThrowPie) {
    unhandled_selections_.push(MenuSelection(current_focus_, controller_id));
  }
  if (logical_input & LogicalInputs_Deflect) {
    unhandled_selections_.push(MenuSelection(ButtonId_Cancel, controller_id));
  }
}

// This is an internal-facing function for moving the focus around.  It
// accepts an array of possible destinations as input, and moves to
// the first active ID it finds.  (otherwise it doesn't move.)
void GuiMenu::UpdateFocus(
    const flatbuffers::Vector<uint16_t>* destination_list) {
  for (size_t i = 0; i < destination_list->Length(); i++) {
    ButtonId destination_id =
        static_cast<ButtonId>(destination_list->Get(i));
    TouchscreenButton* destination =
        FindButtonById(destination_id);
    if (destination != nullptr && destination->is_active()) {
      SetFocus(destination_id);
      return;
    }
  }
  // if we didn't find an active button to move to, we just return and
  // leave everything unchanged.
}

ButtonId GuiMenu::GetFocus() const {
  return current_focus_;
}

void GuiMenu::SetFocus(ButtonId new_focus) {
  current_focus_  = new_focus;
}

// Returns a pointer to the button specified, if it's in the current
// menu.  Otherwise, returns a null pointer if it's not found.
TouchscreenButton* GuiMenu::GetButtonById(ButtonId id) {
  return FindButtonById(id);
}

}  // splat
}  // fpl

