#pragma once

#include "PlatformGeneric.h"
#include <nvidia/nv_egl_util/nv_egl_util.h>

BE_NAMESPACE_BEGIN

class PlatformAndroid : public PlatformGeneric {
public:
    PlatformAndroid();
    
	virtual void			Init() override;
	virtual void			Shutdown() override;

	virtual void            SetMainWindowHandle(void *windowHandle) override;
	virtual void			Quit() override;
    virtual void			Log(const wchar_t *msg) override;
	virtual void			Error(const wchar_t *msg) override;

private:
//	NvEGLUtil *				window;
};

BE_NAMESPACE_END
