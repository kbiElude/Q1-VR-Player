/* Q1 VR Player (c) 2024 Dominik Witczak
 *
 * This code is licensed under version 3 of the GNU Affero General Public License (see LICENSE for details)
 */

#include "Launcher_dummy_gl_context.h"
#include <assert.h>

#include "glfw/glfw3.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "glfw/glfw3native.h"


Launcher::DummyGLContext::DummyGLContext()
    :m_glfw_window_ptr(nullptr)
{
    /* Stub */
}

Launcher::DummyGLContext::~DummyGLContext()
{
    if (m_glfw_window_ptr != nullptr)
    {
        glfwDestroyWindow(m_glfw_window_ptr);

        m_glfw_window_ptr = nullptr;
    }
}

Launcher::DummyGLContextUniquePtr Launcher::DummyGLContext::create()
{
    Launcher::DummyGLContextUniquePtr result_ptr;

    result_ptr.reset(new DummyGLContext() );

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

HDC Launcher::DummyGLContext::get_dc() const
{
    return ::GetDC(::glfwGetWin32Window(m_glfw_window_ptr) );
}

HGLRC Launcher::DummyGLContext::get_glrc() const
{
    return ::glfwGetWGLContext(m_glfw_window_ptr);
}

bool Launcher::DummyGLContext::init()
{
    bool result = false;

    if (!glfwInit() )
    {
        assert(false);

        goto end;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_VISIBLE,               GLFW_FALSE);
    glfwWindowHint(GLFW_DEPTH_BITS,            32);
    glfwWindowHint(GLFW_DOUBLEBUFFER,          1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);

    // Create window with graphics context
    m_glfw_window_ptr = glfwCreateWindow(1,
                                         1,
                                         "Dummy window",
                                         nullptr,  /* monitor */
                                         nullptr); /* share   */

    if (m_glfw_window_ptr == nullptr)
    {
        assert(false);

        goto end;
    }

    glfwMakeContextCurrent(m_glfw_window_ptr);

    result = true;
end:
    return result;
}