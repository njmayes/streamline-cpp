#pragma once

#include <vector>

#include <slc/Common/Base.h>

namespace slc {

	template<std::integral T>
	struct Coordinate
	{
		static constexpr T NullCoord = Limits<T>::Max;
		T x = NullCoord, y = NullCoord;

		Coordinate() = default;
		constexpr Coordinate(T _x, T _y) : x(_x), y(_y) {}

		bool operator==(const Coordinate& other) const { return (x == other.x && y == other.y); }
		bool operator<(const Coordinate& other) const
		{
			if (y == other.y)
				return x < other.x;
			return y < other.y;
		}

		bool Valid() const { return x != NullCoord && y != NullCoord; }
	};
	
	using GridPosition = Coordinate<uint32_t>;

	template<typename T, std::integral TPos = uint32_t>
	class Grid
	{
	public:
		using Position = Coordinate<TPos>;

	private:
		using Bool = uint8_t;
		static constexpr Bool True = 1;
		static constexpr Bool False = 0;

		static constexpr bool IsBool = std::is_same_v<T, bool>;

		using Type = std::conditional_t<IsBool, Bool, T>;
		using TReturn = std::conditional_t<IsBool, Bool&, T&>;
		using TConstReturn = std::conditional_t<IsBool, bool, const T&>;

	public:
		Grid() = default;
		Grid(size_t width, size_t height)
			: mWidth(width), mHeight(height), mData(width* height)
		{}

		TReturn operator()(TPos x, TPos y) { return mData[x + (mWidth * y)]; }
		TConstReturn operator()(TPos x, TPos y) const
		{
			if constexpr (IsBool)
				return (bool)mData[x + (mWidth * y)];

			return mData[x + (mWidth * y)];
		}

		TReturn operator()(const Position& pos) { return mData[pos.x + (mWidth * pos.y)]; }
		TConstReturn operator()(const Position& pos) const
		{
			if constexpr (IsBool)
				return (bool)mData[pos.x + (mWidth * pos.y)];

			return mData[pos.x + (mWidth * pos.y)];
		}

		void Set(size_t index, const T& data)
		{
			if constexpr (IsBool)
				mData[index] = data ? True : False;
			else
				mData[index] = data;
		}

		size_t GetWidth() const { return mWidth; }
		size_t GetHeight() const { return mHeight; }

		void Resize(size_t width, size_t height) { mData.clear(); mData.resize(width * height); }
		void Reset() { mData.clear(); mData.resize(mWidth * mHeight); }

		T* Data() { return mData.data(); }

		auto begin() { return mData.begin(); }
		auto begin() const { return mData.cbegin(); }
		auto end() { return mData.end(); }
		auto end() const { return mData.cend(); }

	protected:
		size_t mWidth = 0, mHeight = 0;

		TReturn At(TPos x, TPos y) { return mData[x + (mWidth * y)]; }
		TConstReturn At(TPos x, TPos y) const
		{
			if constexpr (IsBool)
				return (bool)mData[x + (mWidth * y)];

			return mData[x + (mWidth * y)];
		}

		TReturn At(const Position& pos) { return mData[pos.x + (mWidth * pos.y)]; }
		TConstReturn At(const Position& pos) const
		{
			if constexpr (IsBool)
				return (bool)mData[pos.x + (mWidth * pos.y)];

			return mData[pos.x + (mWidth * pos.y)];
		}

	private:
		std::vector<Type> mData;
	};
}