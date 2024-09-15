/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_state.h"
#include "Launcher_ui.h"
#include <assert.h>
#include <Windows.h>

Launcher::UI::UI(State* in_state_ptr)
    :m_state_ptr(in_state_ptr)
{
    /* Stub */
}

Launcher::UI::~UI()
{
    /* Stub */
}

Launcher::UIUniquePtr Launcher::UI::create(State* in_state_ptr)
{
    Launcher::UIUniquePtr result_ptr;

    result_ptr.reset(new UI(in_state_ptr) );

    assert(result_ptr != nullptr);
    if (result_ptr != nullptr)
    {
        if (!result_ptr->init() )
        {
            result_ptr.reset();
        }
    }

    return result_ptr;
}

bool Launcher::UI::init()
{
    std::wstring glquake_file_path;
    std::wstring glquake_file_name;
    bool         result            = false;

    result = show_glquake_exe_path_dialog(&glquake_file_path,
                                          &glquake_file_name);

    m_state_ptr->set_glquake_exe_file_path(glquake_file_path);

    return result;
}

bool Launcher::UI::show_glquake_exe_path_dialog(std::wstring* out_file_path_ptr,
                                                std::wstring* out_file_name_ptr)
{
    OPENFILENAMEW open_file_name_descriptor           = {};
    bool          result                              = false;
    wchar_t       result_file_name         [MAX_PATH] = {};

    open_file_name_descriptor.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    open_file_name_descriptor.hwndOwner   = HWND_DESKTOP;
    open_file_name_descriptor.lStructSize = sizeof(open_file_name_descriptor);
    open_file_name_descriptor.lpstrFilter  = L"GLQuake Executable (glquake.exe)\0glquake.exe\0";
    open_file_name_descriptor.lpstrFile    = result_file_name;
    open_file_name_descriptor.nMaxFile     = sizeof(result_file_name);

    if (::GetOpenFileNameW(&open_file_name_descriptor) == 0)
    {
        ::MessageBox(HWND_DESKTOP,
                     "You must provide a path to glquake.exe in order to run the tool.",
                     "Error",
                     MB_OK | MB_ICONERROR);

        goto end;
    }

    *out_file_path_ptr = std::wstring(result_file_name,
                                      0,
                                      open_file_name_descriptor.nFileOffset);
    *out_file_name_ptr = std::wstring(result_file_name);

    result = true;
end:
    return result;
}