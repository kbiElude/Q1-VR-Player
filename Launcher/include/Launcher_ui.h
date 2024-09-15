/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#ifndef LAUNCHER_UI_H
#define LAUNCHER_UI_H

#include <memory>
#include <string>

namespace Launcher
{
    class                       State;
    class                       UI;
    typedef std::unique_ptr<UI> UIUniquePtr;

    class UI
    {
    public:
        /* Public funcs */
        static UIUniquePtr create(State* in_state_ptr);

        ~UI();

    private:
        /* Private funcs */
        UI(State* in_state_ptr);

        bool init                        ();
        bool show_glquake_exe_path_dialog(std::wstring* out_file_path_ptr,
                                          std::wstring* out_file_name_ptr);

        State* m_state_ptr;
    };
}

#endif /* LAUNCHER_VR_SUPPORT_H */