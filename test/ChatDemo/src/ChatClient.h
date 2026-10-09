#pragma once

#include "SL/Core.h"
#include "SL/Net.h"
#include "SL/Gfx.h"


class ChatClient : public sl::GuiApplication
{
public:
	ChatClient( sl::Ref< sl::GuiApplicationSpecification > spec, sl::net::ClientContextOptions const& opts );
};