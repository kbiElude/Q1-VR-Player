/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#ifndef LAUNCHER_VR_SUPPORT_H
#define LAUNCHER_VR_SUPPORT_H

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace Launcher
{
    enum class                         VRBackend          : uint8_t;
    class                              VRSupport;
    typedef std::unique_ptr<VRSupport> VRSupportUniquePtr;

    class VRSupport
    {
    public:
        /* Public funcs */
        static VRSupportUniquePtr create();

        void enumerate_devices      (const VRBackend&                 in_backend,
                                     const std::vector<std::string>** out_device_name_vec_ptr_ptr);
        void get_eye_texture_extents(const VRBackend&                 in_backend,
                                     std::array<uint32_t, 2>*         out_u32vec2_ptr);
        void get_refresh_rate       (const VRBackend&                 in_backend,
                                     uint32_t*                        out_refresh_rate_ptr);
        bool is_vr_backend_supported(const VRBackend&                 in_backend);

        ~VRSupport();

    private:
        /* Private typedefs */
        struct VRBackendInfo
        {
            std::vector<std::string> device_name_vec;
            std::array<uint32_t, 2>  eye_texture_extents = {};
            bool                     is_supported        = false;
            uint32_t                 refresh_rate        = 0;
        };

        /* Private funcs */
        VRSupport();

        bool init       ();
        void init_libovr();

        /* Private vars */
        VRBackendInfo m_libovr_backend_info;
        VRBackendInfo m_openxr_backend_info;
    };
}

#endif /* LAUNCHER_VR_SUPPORT_H */