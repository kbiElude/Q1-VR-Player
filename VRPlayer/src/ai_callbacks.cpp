/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
 #include "Common/callbacks.h"
 #include "VRPlayer.h"

static VRPlayerUniquePtr g_vr_player_ptr;

void start_vrplayer_thread_func()
{
    g_vr_player_ptr = VRPlayer::create();

    if (g_vr_player_ptr == nullptr)
    {
        ::TerminateProcess(::GetCurrentProcess(),
                           EXIT_FAILURE);
    }
}

void on_api_interceptor_removed()
{
    /* VR player should have been deleted by this point */
    AI_ASSERT(g_vr_player_ptr == nullptr);
}

void on_api_interceptor_injected()
{
    /* This entrypoint is called from within DllMain() which is a pain and causes LibOVR to lock up
     * at initialization time.
     *
     * Therefore to continue we must spawn a new thread to initialize VR Player.
     */
    std::thread init_thread(start_vrplayer_thread_func);

    init_thread.detach();
}