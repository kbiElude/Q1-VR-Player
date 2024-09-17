/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */
#ifndef LAUNCHER_DUMMY_GL_CONTEXT_H
#define LAUNCHER_DUMMY_GL_CONTEXT_H

#include <Windows.h>
#include <array>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct GLFWwindow;

namespace Launcher
{
    class                                   DummyGLContext;
    typedef std::unique_ptr<DummyGLContext> DummyGLContextUniquePtr;


    class DummyGLContext
    {
    public:
        /* Public funcs */

        /* Creates & binds a dummy GL context to the current thread. */
        static DummyGLContextUniquePtr create();

        HDC   get_dc  () const;
        HGLRC get_glrc() const;

        ~DummyGLContext();

    private:
        /* Private funcs */
        DummyGLContext();

        bool init();

        /* Private vars */
        GLFWwindow* m_glfw_window_ptr;
    };
}

#endif /* LAUNCHER_DUMMY_GL_CONTEXT_H */