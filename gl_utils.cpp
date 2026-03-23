#include "gl_utils.h"

#include <iostream>
#include <stdexcept>
#include <string>

const char* glutilGetStringSafe(GLenum pname) {
    const GLubyte* s = glGetString(pname);
    if (!s) return nullptr;
    return reinterpret_cast<const char*>(s);
}

static void glutilPrintString(const char* label, GLenum pname) {
    const char* s = glutilGetStringSafe(pname);
    std::cout << label << ": " << (s ? s : "<Unknown>") << "\n";
}

void glutilPrintContextInfoOrThrow(int minMajor, int minMinor, int requireCoreProfile) {
    // Strings
    glutilPrintString("GL_VENDOR", GL_VENDOR);
    glutilPrintString("GL_RENDERER", GL_RENDERER);
    glutilPrintString("GL_VERSION", GL_VERSION);
    glutilPrintString("GLSL", GL_SHADING_LANGUAGE_VERSION);

    // Numeric version
    GLint major = 0;
    GLint minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    std::cout << "GL_MAJOR_VERSION: " << major << "\n";
    std::cout << "GL_MINOR_VERSION: " << minor << "\n";

    // Verify minimum version
    if (major < minMajor || (major == minMajor && minor < minMinor)) {
        std::string msg = "OpenGL " + std::to_string(major) + "." + std::to_string(minor)
                        + " created; need at least " + std::to_string(minMajor) + "." + std::to_string(minMinor);
        throw std::runtime_error(msg);
    }

    // Profile
    GLint profile_mask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
    if (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) {
        std::cout << "Profile: CORE\n";
    } else if (profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
        std::cout << "Profile: COMPATIBILITY\n";
        if (requireCoreProfile) {
            throw std::runtime_error("Compatibility profile created; need CORE profile");
        }
    } else {
        throw std::runtime_error("GL_CONTEXT_PROFILE_MASK returned no known profile bit");
    }

    // Context flags
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    std::cout << "GL_CONTEXT_FLAGS: 0x" << std::hex << flags << std::dec << "\n";
    if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) std::cout << "  - FORWARD_COMPATIBLE\n";
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)              std::cout << "  - DEBUG\n";
    if (flags & GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT)      std::cout << "  - ROBUST_ACCESS\n";
    if (flags & GL_CONTEXT_FLAG_NO_ERROR_BIT)           std::cout << "  - NO_ERROR\n";
}

int glutilTryEnableDebugOutput(GLDEBUGPROC callback, void* userParam) {
    GLint ctx_flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &ctx_flags);

    int has_debug = 0;
    if (GLEW_KHR_debug || GLEW_ARB_debug_output) {
        has_debug = 1;
    }

    if (!has_debug) {
        std::cout << "GL_DEBUG not supported (no KHR_debug / ARB_debug_output).\n";
        return 0;
    }

    if (!(ctx_flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        std::cout << "GL_DEBUG not enabled (context is not a debug context).\n";
        return 0;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(callback, userParam);
    std::cout << "GL_DEBUG enabled.\n";
    return 1;
}

float glutilClamp01(float x) {
    if (x < 0.0f) return 0.0f;
    if (x > 1.0f) return 1.0f;
    return x;
}
