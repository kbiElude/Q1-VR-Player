/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#ifndef LAUNCHER_STATE_H
#define LAUNCHER_STATE_H

#include <array>
#include <memory>
#include <string>

namespace Launcher
{
    class                          State;
    typedef std::unique_ptr<State> StateUniquePtr;

    enum class VRBackend : uint8_t;
    class State

    {
    public:
        /* Public funcs */
        static StateUniquePtr create();

        ~State();

        const VRBackend& get_active_vr_backend() const
        {
            return m_active_vr_backend;
        }

        const std::wstring* get_glquake_exe_file_path_ptr() const
        {
            return (m_glquake_exe_file_path.size() > 0) ? &m_glquake_exe_file_path
                                                        :  nullptr;
        }

        void set_active_vr_backend(const VRBackend& in_new_value)
        {
            m_active_vr_backend = in_new_value;
        }

        void set_glquake_exe_file_path(const std::wstring& in_new_value)
        {
            m_glquake_exe_file_path = in_new_value;
        }

    private:
        /* Private funcs */
        State();

        bool init();

        /* Private vars */
        VRBackend              m_active_vr_backend;
        std::wstring           m_glquake_exe_file_path;


    };
}
#endif /* LAUNCHER_STATE_H */