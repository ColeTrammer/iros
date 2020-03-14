#include "window_wrapper.h"

WindowWrapper::WindowWrapper() : m_window(m_connection.create_window(200, 200, 80 * 8, 25 * 16)) {}

WindowWrapper::~WindowWrapper() {}