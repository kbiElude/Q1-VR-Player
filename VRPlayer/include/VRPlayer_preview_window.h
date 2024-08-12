/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#if !defined(VR_PLAYER_PREVIEW_WINDOW_H)
#define VR_PLAYER_PREVIEW_WINDOW_H

#include "VRPlayer_types.h"
#include <unordered_map>

/* Forward decls */
struct GLFWwindow;


class PreviewWindow : public IPreviewWindowCallback
{
public:
    /* Public funcs */
    ~PreviewWindow();

    static PreviewWindowUniquePtr create(VRPlayer*    in_vr_player_ptr,
                                         IVRPlayback* in_vr_playback_ptr,
                                         VRRenderer*  in_vr_renderer_ptr,
                                         Settings*    in_settings_ptr);

    void set_position(const std::array<uint32_t, 2>& in_x1y1,
                      const std::array<uint32_t, 2>& in_extents);

    // IPreviewWindowCallback interface impl.
    void on_frame_available(const Frame* in_frame_ptr) const final;

private:
    /* Private funcs */
    PreviewWindow(VRPlayer*    in_vr_player_ptr,
                  IVRPlayback* in_vr_playback_ptr,
                  VRRenderer*  in_vr_renderer_ptr);

    void execute();
    bool init   (Settings* in_settings_ptr);

    /* Private vars */
    std::array<uint32_t, 2> m_window_extents_incl_ui;
    std::array<uint32_t, 2> m_window_extents_wo_ui;
    std::array<uint32_t, 2> m_window_x1y1;

    mutable const Frame* m_current_frame_ptr;
    mutable uint32_t     m_frame_counter;

    uint32_t m_preview_fb_id;

    PreviewWindowUIUniquePtr m_ui_ptr;
    IVRPlayback*             m_vr_playback_ptr;
    VRPlayer*                m_vr_player_ptr;
    VRRenderer*              m_vr_renderer_ptr;
    GLFWwindow*              m_window_ptr;
    std::thread              m_worker_thread;
    volatile bool            m_worker_thread_must_die;
    HANDLE                   m_worked_thread_done_event;
    HANDLE                   m_worked_thread_wait_event;
};

#endif /* VR_PLAYER_PREVIEW_WINDOW_H */