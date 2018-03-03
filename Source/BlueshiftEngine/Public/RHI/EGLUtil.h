#pragma once

#ifdef __XAMARIN__

struct EGLUtil
{
	int m_width;
	int m_height;
    int m_windowWidth;
    int m_windowHeight;

	bool swap() { return true; }
	int getWidth() { return m_width; }
	int getHeight() { return m_height; }
};

#endif
