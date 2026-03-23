#pragma once

#include <GL/glew.h>

// Safe wrapper around glGetString (can return nullptr)
const char* glutilGetStringSafe(GLenum pname);

// Print GL strings, version, profile and flags. Throws if requirements are not met.
// requireCoreProfile: 1 => CORE required, 0 => any profile accepted.
void glutilPrintContextInfoOrThrow(int minMajor, int minMinor, int requireCoreProfile);

// Enable debug output if supported and context is a debug context.
// Returns 1 if enabled, 0 otherwise.
int glutilTryEnableDebugOutput(GLDEBUGPROC callback, void* userParam);

// Clamp to [0,1]
float glutilClamp01(float x);
