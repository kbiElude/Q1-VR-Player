/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VR_PLAYER_PREVIEW_WINDOW_UI_H)
#define VR_PLAYER_PREVIEW_WINDOW_UI_H

#include "VRPlayer_types.h"

/* Forward decls */
struct GLFWwindow;
struct ImGuiContext;

class PreviewWindowUI
{
public:
    /* Public funcs */
    static PreviewWindowUIUniquePtr create(Settings* in_settings_ptr);

    uint32_t get_height_px          () const;
    void     render                 (const uint32_t& in_start_y,
                                     const uint32_t& in_width);
    void     setup_for_bound_context(GLFWwindow*     in_glfw_preview_window_ptr);

    ~PreviewWindowUI();

private:
    /* Private funcs */
    PreviewWindowUI(Settings* in_settings_ptr);

    bool init();

    /* Private vars */
    Settings* const m_settings_ptr;

    ImGuiContext* m_imgui_context_ptr;
};

#endif /* VR_PLAYER_PREVIEW_WINDOW_UI_H */