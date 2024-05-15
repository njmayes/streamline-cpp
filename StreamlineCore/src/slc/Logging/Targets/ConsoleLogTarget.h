#pragma once

#include "ILogTarget.h"

#include <map>

namespace slc {

	namespace ConsoleAttributes
	{
		using Attribute = unsigned int;

		enum : Attribute {
			BlackForeground = 1,
			RedForeground = 2,
			GreenForeground = 4,
			YellowForeground = 8,
			BlueForeground = 16,
			MagentaForeground = 32,
			CyanForeground = 64,
			WhiteForeground = 128,

			ForegroundMask = BlackForeground | RedForeground | GreenForeground | YellowForeground | BlueForeground | MagentaForeground | CyanForeground | WhiteForeground,

			BlackBackground = 256,
			RedBackground = 512,
			GreenBackground = 1024,
			YellowBackground = 2048,
			BlueBackground = 4096,
			MagentaBackground = 8192,
			CyanBackground = 16384,
			WhiteBackground = 32768,

			BackgroundMask = BlackBackground | RedBackground | GreenBackground | YellowBackground | BlueBackground | MagentaBackground | CyanBackground | WhiteBackground,

			Bold = 65536,
			Faint = 131072,
			Italic = 262144,
			Underline = 524288,
			SlowBlink = 1048576,
			FastBlink = 2097152,

			StyleMask = Bold | Faint | Italic | Underline | SlowBlink | FastBlink,

			DefaultForeground = WhiteForeground,
			DefaultBackground = BlackBackground,
			Default = DefaultForeground | DefaultBackground,
		};
	}

	class ConsoleLogTarget : public ILogTarget
	{
	public:
		ConsoleLogTarget(LogLevel level, std::map<LogLevel, ConsoleAttributes::Attribute> colours = {});

	private:
		void DoWriteTarget() override;
		void DoPreFlush() override;
		void DoFlush() override;

		void PopulateBuffer(std::span<MessageEntry> data) override;

		void WriteColourCode(LogLevel level);
		void WriteColourAttribute(ConsoleAttributes::Attribute colour);
		void WriteStyleAttribute(ConsoleAttributes::Attribute style);
		void WriteResetColourCode();

	private:
		std::map<LogLevel, ConsoleAttributes::Attribute> mColours;
		ConsoleAttributes::Attribute mCurrentAttribute{};
	};
}